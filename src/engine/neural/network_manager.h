/*
    Firefly Chess Engine
    Copyright (C) 2022  Ognyan Mirev

    This program is free software: you can redistribute it and/or modify
            it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
            but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/


#ifndef FIREFLY_NETWORK_MANAGER_H
#define FIREFLY_NETWORK_MANAGER_H

#include <vector>
#include <thread>
#include <csignal>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <random>

#include <engine/neural/lc0_network.h>
#include <engine/mcts/node.h>

#include <c10/cuda/CUDAStream.h>

#include <cxxopts.hpp>

/*
 * 0. Accepts nodes from any number of threads.
 *
 * 1. A thread pops nodes from the input queue, builds and preprocesses batches and
 * passes them off to the NN processing queue
 *
 * 2. Any number of backends can process nodes simultaneously,
 * they get batches from the NN processing queue on a first come first serve basis
 *
 * 3. The network results are pushed onto yet another queue for postprocessing, a thread reads from this queue and
 * populates the relevant nodes with the computed policy/value data.
 */
template<typename Network=lc0::LC0Network>
struct network_manager
{
     memory* memory_ = nullptr;
    /*
       min_batch_size may be ignored if there is one or more tree traversal thread waiting for a node to be evaluated
    */
    network_manager(cxxopts::ParseResult& options) :
    max_batch_size(options["max_batch_size"].as<int>()),
#ifndef SYNCHRONOUS_INFERENCE
    min_batch_size(min_batch_size),
    max_input_queue_size(max_input_queue_size),
    max_nn_input_queue_size(max_nn_input_batch_queue_size),
#endif
    softmax_temperature(options["softmax_temperature"].as<float>()),
    cpu_device(torch::DeviceType::CPU)
    {
        softmax_temperature_reciprocal = 1/softmax_temperature;

        auto device = options["device"].as<string>();
        auto weights_file = options["n"].as<string>();

        if (device == "auto")
            autodetect_backends(weights_file);
        else if (device == "cuda")
        {
            if (!torch::cuda::is_available())
                throw std::invalid_argument("CUDA is not available.");

            for (int i = 0; i < torch::cuda::device_count(); i++)
                add_backend(Network(weights_file, torch::Device(torch::kCUDA, i), true));
        }
        else if (device == "cpu")
        {
            auto n_threads = options["cpu_inference_threads"].as<int>();

            if (n_threads <= 0)
            {
                auto n_threads = std::thread::hardware_concurrency();

                if (n_threads == 0)
                {
                    std::cout << "info Could not detect the number of supported concurrent threads, defaulting to 2." << std::endl;
                    torch::set_num_threads(2);
                }
                else {
                    torch::set_num_threads(std::max(1u, n_threads/2));
                }
            }
            else torch::set_num_threads(n_threads);

            add_backend(Network(weights_file, cpu_device, false));
        }
        else
        {
            std::stringstream ss(device);
            string device_index;

            while (std::getline(ss, device_index, ','))
            {
                auto i = stoi(device_index);

                add_backend(Network(weights_file, torch::Device(torch::kCUDA, i), true));
            }
        }

    }

    ~network_manager()
    {
#ifndef SYNCHRONOUS_INFERENCE
        halt();
#endif
    }

#ifndef SYNCHRONOUS_INFERENCE
    void add_to_eval_queue(mcts::node *node_to_eval)
    {
        while (input_queue.size() >= max_input_queue_size)
            this_thread::sleep_for(1ms);

        std::lock_guard lock(input_queue_lock);
        input_queue.push(node_to_eval);
        cv_input_node.notify_one();
    }


    void halt()
    {
        if (active)
        {
            active = false;
            input_processing_t.join();
            output_processing_t.join();
            for (auto& i : backend_threads)
                i.join();
            backend_threads.clear();
        }
    }

    void run_async()
    {
        if (backends.empty())
            throw std::logic_error("network_manager::run_async called before any backends were added.");
        input_processing_t  = thread(&network_manager<Network>::input_processing, this);
        output_processing_t = thread(&network_manager<Network>::output_processing, this);

        for (int i = 0; i < backends.size(); i++) {
            backend_threads.emplace_back(std::thread(&network_manager<Network>::inference, this, i));
        }
    }

    /*
    void pause()
    {

        paused_lock.lock();
        paused = true;
        while(working_threads);

        //nn_output_batches_lock.lock();
        //nn_input_batches_lock.lock();
        //output_queue_lock.lock();
    }

    void resume()
    {
        paused = false;
        paused_lock.unlock();
        //nn_output_batches_lock.unlock();
        //nn_input_batches_lock.unlock();
        //output_queue_lock.unlock();
        resume_time = std::chrono::high_resolution_clock::now();
        nodes_processed = 0;
        //paused_cv.notify_all();

    }
    */
    void wait_until_pipeline_empty()
    {
        input_queue_lock.lock();
        threads_waiting_for_output++;
        cv_input_node.notify_all();
        input_queue_lock.unlock();

        while(!input_queue.empty()) this_thread::sleep_for(1ms);
        while(!output_queue.empty()) this_thread::sleep_for(1ms);

        threads_waiting_for_output--;
    }

#endif


    uint64_t time_spent_waiting = 0;

    // If during tree traversal, a node that is enqueued but not yet processed is encountered, call this function to wait on it
    void wait_for_node_evaluation(const mcts::node* node_to_wait)
    {
        //auto start = chrono::high_resolution_clock::now();

#ifndef SYNCHRONOUS_INFERENCE
        input_queue_lock.lock();
        cv_input_node.notify_all();
        input_queue_lock.unlock();
#endif

        std::unique_lock lock(node_processed_lock);

        if (!node_to_wait->evaluated)
        {
#ifndef SYNCHRONOUS_INFERENCE
            threads_waiting_for_output++;
#endif
            cv_node_processed.wait(lock, [&node_to_wait](){
                return node_to_wait->evaluated;
            });
#ifndef SYNCHRONOUS_INFERENCE
            threads_waiting_for_output--;
#endif
        }



        //time_spent_waiting += chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now() - start).count();
    }

    void add_backend(Network&& backend)
    {
        backend->forward(torch::randn({32,112,8,8}, torch::TensorOptions()
        .dtype(backend->expected_dtype).device(backend->device)));

        std::cout << "info [netmgr] Adding backend " << backends.size() << std::endl;
        backends.emplace_back(backend);

    }

    size_t autodetect_backends(std::string const& weights_file)
    {
        if (torch::cuda::is_available())
            for (int i = 0; i < torch::cuda::device_count(); i++)
                backends.emplace_back(Network(weights_file, torch::Device(torch::kCUDA, i), true));
        else
            add_backend(Network(weights_file, torch::Device(torch::DeviceType::CPU), false));
        return 1;
    }

#ifndef SYNCHRONOUS_INFERENCE
    size_t nodes_in_pipeline() const
    {
        return input_queue.size() + output_queue.size();
    }
#endif

    float get_nps()
    {
        return 1000 * float(nodes_processed) /
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() -
        this->resume_time).count();
    }

    void print_pipeline_information(auto& oss)
    {
#ifndef SYNCHRONOUS_INFERENCE
        oss << "NPS: " << get_nps() << "  |  input queue size: " << input_queue.size() <<
        "  |  NN input queue size: " << nn_input_batches.size() <<
        "  |  NN output queue size: " << nn_output_batches.size() <<
        "  |  output queue size: " << output_queue.size() <<
        "  |  processed: " << nodes_processed << endl;
#else
        oss << "info NPS: " << get_nps() << "  |  processed: " << nodes_processed;
#endif
    }

    void reset_nps()
    {
        resume_time = std::chrono::high_resolution_clock::now();
        nodes_processed = 0;
    }


#ifdef SYNCHRONOUS_INFERENCE
    /*
     * Equivalent to input_processing -> inference -> output_processing, except it does everything on the calling thread
     */
    void blocking_inference(std::vector<mcts::node*> const& batch)
    {
        torch::InferenceMode inference_mode;
        PositionHistory history;

        auto temp_batch_data = (float(*)[NETWORK_INPUT_PLANES][8][8])memory_->get_batch_memory();

        int policy_indices[1024];
        int index_in_batch = 0;

        for (auto node : batch)
        {
            for (; node && history.size() != 8; node = node->parent)
                history.push_back(node->board);

            reverse(history.begin(), history.end());

            int transform;
            auto planes = lczero_modified::EncodePositionForNN(backends[0]->input_format, history, 8,
                                                               lczero_modified::FillEmptyHistory::NO, &transform);


            for (int i = 0; i < NETWORK_INPUT_PLANES; i++) {
                for (int y = 0; y < 8; y++) {
                    for (int x = 0; x < 8; x++) {
                        temp_batch_data[index_in_batch][i][y][x] = planes[i].mask & get_bit(x,y) ? planes[i].value : 0;
                    }
                }
            }

            index_in_batch++;
            history.clear();
        }

        auto net = get_backend();


        if (net->device.is_cuda())
            c10::cuda::setCurrentCUDAStream(c10::cuda::getStreamFromPool(false, net->device.index()));



        auto batch_tensor = torch::from_blob(temp_batch_data, {index_in_batch, NETWORK_INPUT_PLANES, 8, 8}, torch::kFloat32)
                .to(net->device, net->expected_dtype);

        auto net_results = net->forward(batch_tensor);


        auto values_tensor = net_results.value.to(cpu_device, torch::Dtype::Float);
        auto policy_tensor = net_results.policy.to(cpu_device, torch::Dtype::Float);
        auto moves_left_tensor = net_results.moves_left.to(cpu_device, torch::Dtype::Float);


        net->n_user_threads--;


        auto values = static_cast<float(*)[3]>(values_tensor.data_ptr());
        auto moves_left = static_cast<float*>(moves_left_tensor.data_ptr());



        for (size_t i = 0; i < batch.size(); i++)
        {
            if (batch[i]->evaluated) continue;

            // Q_ = L - W    To account for the perspective shift
            float P_sum = 0, P_max = -std::numeric_limits<float>::infinity();

            auto Q_ = values[i][2] - values[i][0];


            batch[i]->Q_ = Q_;
            batch[i]->visits_pending = 0;
            batch[i]->visit_count = 1;

            if (batch[i]->parent)
                batch[i]->parent->update_value(-Q_);


            int ml_temp = moves_left[i]; //net_results.moves_left[i].item<float>();

            batch[i]->moves_left = ml_temp > 255 ? 255 : ml_temp;

            //batch[i]->update_value(values[i][2] - values[i][0]);

            //region Gather policy values


            // Get node policy indices
            if (batch[i]->board.flipped) {
                for (int move_idx = 0; move_idx < batch[i]->edge_count; move_idx++)//(auto &child: *batch[i])
                    policy_indices[move_idx] = batch[i]->begin()[move_idx].move.to_flipped_policy_index();
            }
            else
            {
                for (int move_idx = 0; move_idx < batch[i]->edge_count; move_idx++)//(auto &child: *batch[i])
                    policy_indices[move_idx] = batch[i]->begin()[move_idx].move.to_policy_index();
            }

            // Create indices tensor and copy it to the relevant device
            auto indices_tensor = torch::from_blob(policy_indices, {batch[i]->edge_count},
                                                   c10::TensorOptions()
                                                           .dtype(torch::kInt)).to(policy_tensor.device());

            // Gather the policy values
            auto gathered_p = torch::index_select(policy_tensor[i], 0, indices_tensor);

            auto pol_max = torch::max(gathered_p);

            gathered_p = torch::exp((gathered_p - pol_max) * softmax_temperature_reciprocal);

            auto pol_sum = torch::sum(gathered_p);

            // Avoid any .item() calls as they appear to be very slow
            //auto divisor = (pol_sum > 0) * pol_sum + (pol_sum <= 0) * 1;

            float pol_sum_f = *(float*)pol_sum.data_ptr();

            if (pol_sum_f > 0) gathered_p *= (1/ pol_sum);
            //gathered_p *= (1/divisor);


            float* priors = static_cast<float*>(gathered_p.data_ptr());


            for (int move_idx = 0; move_idx < batch[i]->edge_count; move_idx++)
                batch[i]->get_edges()[move_idx].set_prior(priors[move_idx]);

            batch[i]->sort_edges_by_priors();

            std::lock_guard lock(node_processed_lock);
            batch[i]->evaluated = true;
            batch[i]->unlock();


            cv_node_processed.notify_all();
        }

        nodes_processed += batch.size();

        memory_->release_batch_memory((float*)temp_batch_data);

    }
#endif


    int get_max_batch_size() const
    {
        return max_batch_size;
    }
private:


    Network& get_backend()
    {
        if (backends.size() == 1)
            return backends[0];

        static std::mutex sel_lock;
        std::lock_guard l_(sel_lock);
        int best = 0;
        int best_util = backends[0]->n_user_threads;

        for (int i = 1; i < backends.size(); i++)
        {
            if (backends[i]->n_user_threads < best_util)
            {
                best_util = backends[i]->n_user_threads;
                best = i;
            }
        }

        backends[best]->n_user_threads++;
        return backends[best];
    }

#ifndef SYNCHRONOUS_INFERENCE
    void input_processing()
    {
        torch::InferenceMode inference_mode;

        torch::Tensor batch_tensor;
        std::vector<mcts::node*> temp_batch;
        temp_batch.reserve(max_batch_size);
        PositionHistory history;

        auto temp_batch_data = static_cast<float(*)[112][8][8]>((void*)new float[NETWORK_INPUT_PLANES * max_batch_size * 8 * 8]);
        //batch_tensor = torch::from_blob(temp_batch_data, {max_batch_size, 112, 8, 8}, torch::kFloat32);

        int index_in_batch = 0;

        std::unique_lock queue_lock(input_queue_lock);
        queue_lock.unlock();
        while (active)
        {
            queue_lock.lock();

            cv_input_node.wait(queue_lock, [&]()
            {
                return this->input_queue.size() >= min_batch_size ||
                        // If there are threads waiting NN results, disregard min_batch_size to prevent deadlocks.
                        (threads_waiting_for_output && !input_queue.empty()) ||
                        // If the NN input queue is empty, disregard batch size to slightly improve throughput.
                        (!input_queue.empty() && nn_input_batches.empty());
            });


            // Gather batch from queue, also copy the nodes directly to the output_queue
            int batch_size = min(max_batch_size, input_queue.size());

            output_queue_lock.lock();
            for (int i = 0; i < batch_size; i++)
            {
                auto node = input_queue.front();
                input_queue.pop();

                temp_batch.push_back(node);
                output_queue.push(node);
            }

            output_queue_lock.unlock();
            queue_lock.unlock();


            //cout << "Collected batch of size: " << batch_size << endl;

            //batch_tensor = torch::zeros({batch_size, 112,8,8});

            // Preprocess nodes for the NN
            for (auto& node : temp_batch)
            {
                for (; node && history.size() != 8; node = node->parent)
                    history.push_back(node->board);

                reverse(history.begin(), history.end());

                int transform;
                auto planes = lczero_modified::EncodePositionForNN(backends[0]->input_format, history, 8,
                                                                   lczero_modified::FillEmptyHistory::NO, &transform);


                for (int i = 0; i < NETWORK_INPUT_PLANES; i++) {
                    for (int y = 0; y < 8; y++) {
                        for (int x = 0; x < 8; x++) {
                            temp_batch_data[index_in_batch][i][y][x] = planes[i].mask & get_bit(x,y) ? planes[i].value : 0;
                        }
                    }
                }

                index_in_batch++;

                //lc0::input_planes_to_tensor(planes, batch_tensor, index_in_batch++);

                history.clear();
            }

            data_copy_lock.lock();
            batch_tensor = torch::from_blob(temp_batch_data, {index_in_batch, NETWORK_INPUT_PLANES, 8, 8}, torch::kFloat32)
            .to(backends[0]->device, backends[0]->expected_dtype);
            data_copy_lock.unlock();
            //batch_tensor = batch_tensor.to(backends[0]->device, backends[0]->expected_dtype);

            // Add nodes to NN queue
            nn_input_batches_lock.lock();
            nn_input_batches.push(batch_tensor);
            cv_nn_input.notify_one();
            nn_input_batches_lock.unlock();

            index_in_batch = 0;
            temp_batch.clear();
        }

        delete[] temp_batch_data;
    }

    void inference(int backend_idx)
    {
        torch::InferenceMode inference_mode;

        //torch::NoGradGuard guard;
        Network& network = backends[backend_idx];


        std::unique_lock nn_input_lock(nn_input_batches_lock);
        nn_input_lock.unlock();
        while (active) {

            nn_input_lock.lock();

            cv_nn_input.wait(nn_input_lock, [&](){return !nn_input_batches.empty();});


            auto batch = nn_input_batches.front();
            nn_input_batches.pop();

            if (!nn_input_batches.empty())
            {
                static vector<torch::Tensor> vec;
                vec = {batch};

                do
                {
                    vec.push_back(nn_input_batches.front());
                    nn_input_batches.pop();
                }
                while (!nn_input_batches.empty());

                batch = torch::cat(vec);
                vec.clear();
            }

            nn_input_lock.unlock();


            //auto result = network->forward(batch.to(network->device, network->expected_dtype));
            auto result = network->forward(batch);

            nn_output_batches_lock.lock();
            nn_output_batches.push(result);
            cv_nn_output.notify_one();
            nn_output_batches_lock.unlock();


            //cout << "Backend " << backend_idx << " processed batch of size " << batch.size(0) << endl;
        }
    }

    void output_processing()
    {
        torch::InferenceMode inference_mode;

        //torch::NoGradGuard grad_guard;
        vector<mcts::node*> batch;
        batch.reserve(max_batch_size);
        float intermediate[1024];

        int* policy_indices = (int*)(intermediate);



        std::unique_lock waiting_for_nn_outputs_lock(nn_output_batches_lock);
        waiting_for_nn_outputs_lock.unlock();

        while (active)
        {
            waiting_for_nn_outputs_lock.lock();
            //region Sync

            //{std::lock_guard (this->paused_lock);}


            cv_nn_output.wait(waiting_for_nn_outputs_lock, [&](){return !nn_output_batches.empty();});


            auto net_results = nn_output_batches.front();
            nn_output_batches.pop();


            waiting_for_nn_outputs_lock.unlock();

            //endregion


            //region Process Batch


            size_t batch_size = net_results.value.size(0);

            output_queue_lock.lock();
            for (size_t i = 0; i < batch_size; i++)
            {
                batch.push_back(output_queue.front());
                output_queue.pop();
            }
            output_queue_lock.unlock();

            data_copy_lock.lock();
            auto values_tensor = net_results.value.to(cpu_device, torch::Dtype::Float);
            auto policy_tensor = net_results.policy.to(cpu_device, torch::Dtype::Float);
            auto moves_left_tensor = net_results.moves_left.to(cpu_device, torch::Dtype::Float);
            data_copy_lock.unlock();


            auto values = static_cast<float(*)[3]>(values_tensor.data_ptr());
            auto moves_left = static_cast<float*>(moves_left_tensor.data_ptr());



            for (size_t i = 0; i < batch.size(); i++)
            {
                if (batch[i]->evaluated) continue;

                // Q_ = L - W    To account for the perspective shift
                float P_sum = 0, P_max = -std::numeric_limits<float>::infinity();

                auto Q_ = values[i][2] - values[i][0];


                batch[i]->Q_ = Q_;
                batch[i]->visits_pending = 0;
                batch[i]->visit_count = 1;

                if (batch[i]->parent)
                    batch[i]->parent->update_value(-Q_);


                int ml_temp = moves_left[i]; //net_results.moves_left[i].item<float>();

                batch[i]->moves_left = ml_temp > 255 ? 255 : ml_temp;

                //batch[i]->update_value(values[i][2] - values[i][0]);

                //region Gather policy values

                // Store all policy indices in a temp container, send the indices to the device,
                // gather the appropriate values and copy them over to a specified location in memory (var. intermediate)
                // This is done since most of the policy tensor is unnecessary.



                // Get node policy indices
                if (batch[i]->board.flipped) {
                    for (int move_idx = 0; move_idx < batch[i]->edge_count; move_idx++)//(auto &child: *batch[i])
                        policy_indices[move_idx] = batch[i]->begin()[move_idx].move.to_flipped_policy_index();
                }
                else
                {
                    for (int move_idx = 0; move_idx < batch[i]->edge_count; move_idx++)//(auto &child: *batch[i])
                        policy_indices[move_idx] = batch[i]->begin()[move_idx].move.to_policy_index();
                }

                // Create indices tensor and copy it to the relevant device
                auto indices_tensor = torch::from_blob(policy_indices, {batch[i]->edge_count},
                                                       c10::TensorOptions()
                                                       .dtype(torch::kInt)).to(policy_tensor.device());

                // Gather the policy values
                auto gathered_p = torch::index_select(policy_tensor[i], 0, indices_tensor);

                auto pol_max = torch::max(gathered_p);

                gathered_p = torch::exp((gathered_p - pol_max) * softmax_temperature_reciprocal);

                //auto pol_sum = torch::sum(gathered_p).item<float>();

                //if (pol_sum > 0) gathered_p *= (1/pol_sum);


                auto pol_sum = torch::sum(gathered_p);

                auto divisor = (pol_sum > 0) * pol_sum + (pol_sum <= 0) * 1;

                gathered_p *= (1/divisor);


                // Create tensor that points to local memory.
                //auto intermediate_tensor = torch::from_blob(intermediate, {gathered_p.size(0)}, torch::TensorOptions(torch::kFloat32));

                // Copy into tensor, this is a bit of a hack
                //intermediate_tensor.copy_(gathered_p);


                /*
                float* policies = policy_tensor[i].data_ptr<float>();
                for (int move_idx = 0; move_idx < batch[i]->edge_count; move_idx++)
                {
                    auto& child = batch[i]->get_edges()[move_idx];

                    auto policy_index = batch[i]->board.flipped ?
                                        child.move.to_flipped_policy_index() :
                                        child.move.to_policy_index();

                    //auto p = net_results.policy[i][policy_index].template item<float>();
                    auto p = policies[policy_index];
                    //auto p = policy[i][policy_index];

                    //if (p != gathered_p[move_idx].item<float>())
                    if (p != intermediate[move_idx])
                    {
                        cout << p << " != " << gathered_p[move_idx].item<float>() << endl;
                    }

                    intermediate[move_idx] = p;
                    if (p > P_max) P_max = p;
                }

                //endregion

                // TODO: This can be sped up with AVX (https://github.com/reyoung/avx_mathfun)
                //region Normalize
                for (int move_idx = 0; move_idx < batch[i]->edge_count; move_idx++)
                {
                    float p = std::exp((intermediate[move_idx] - P_max) / softmax_temperature);

                    P_sum += p;
                    intermediate[move_idx] = p;
                }
                const float scale = P_sum > 0.0f ? 1.0f / P_sum : 1.0f;
                //endregion
                 */

                float* intermediate = static_cast<float*>(gathered_p.data_ptr());


                for (int move_idx = 0; move_idx < batch[i]->edge_count; move_idx++)
                    batch[i]->get_edges()[move_idx].set_prior(intermediate[move_idx]);



                batch[i]->sort_edges_by_priors();

                std::lock_guard lock(node_processed_lock);
                batch[i]->evaluated = true;
                batch[i]->unlock();


                cv_node_processed.notify_all();
            }

            nodes_processed += batch_size;
            batch.clear();

            //working_threads--;
            //endregion
        }
    }
#endif

    float softmax_temperature, softmax_temperature_reciprocal;
    size_t max_batch_size, min_batch_size, max_input_queue_size, max_nn_input_queue_size;

#ifndef SYNCHRONOUS_INFERENCE
    bool active = true, paused = false;
    std::atomic<int> threads_waiting_for_output = 0;
    std::thread input_processing_t, output_processing_t;
    std::vector<std::thread> backend_threads;
#endif

    std::vector<Network> backends;

    torch::Device cpu_device;

    std::condition_variable cv_node_processed;
    std::mutex node_processed_lock;

    std::chrono::time_point<std::chrono::system_clock> resume_time;

#ifndef SYNCHRONOUS_INFERENCE
    std::condition_variable cv_input_node,
                            cv_nn_input,
                            cv_nn_output,
                            cv_paused;


    std::mutex  input_queue_lock,
                output_queue_lock,
                nn_input_batches_lock,
                nn_output_batches_lock,
                data_copy_lock;


    std::queue<torch::Tensor> nn_input_batches;
    std::queue<lc0::NetworkOutput> nn_output_batches;
    std::queue<mcts::node*> output_queue;
    std::queue<mcts::node*> input_queue;
#endif

public:
    std::atomic<uint64_t> nodes_processed = 0;
};

#endif //FIREFLY_NETWORK_MANAGER_H
