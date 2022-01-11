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

#ifndef FIREFLY_LC0_NETWORK_H
#define FIREFLY_LC0_NETWORK_H

#include <pch.h>
#include <external/LeelaUtils/encoder.h>


#define NETWORK_CLASSICAL_WITH_HEADFORMAT 3
#define NETWORK_SE_WITH_HEADFORMAT 4

#define NETWORK_INPUT_PLANES 112
#define POLICY_SIZE (8*8*80)

/*
 * https://lczero.org/dev/backend/nn/
 */
namespace lc0
{
    struct NetworkOutput
    {
        torch::Tensor moves_left;
        torch::Tensor value;
        torch::Tensor policy;
        void to(torch::Device&);
    };
    /*
     * maps input to batch[idx_in_batch]
     */
    inline void input_planes_to_tensor(InputPlanes const& input, torch::Tensor& batch, int idx_in_batch)
    {
        auto temp = *static_cast<float(*)[NETWORK_INPUT_PLANES][8][8]>(batch[idx_in_batch].data_ptr());

        for (int i = 0; i < NETWORK_INPUT_PLANES; i++) {
            for (int y = 0; y < 8; y++) {
                for (int x = 0; x < 8; x++) {
                    temp[i][y][x] = input[i].mask & get_bit(x,y) ? input[i].value : 0;
                }
            }
        }
    }

    struct SELayerImpl : public torch::nn::Module
    {
        SELayerImpl(int Filters, int SEChannels);

        torch::Tensor forward(torch::Tensor x);

        int SEChannels, Filters;
        torch::nn::Linear fc1, fc2;

    private:
        torch::nn::Sequential all_layers;
    };
    TORCH_MODULE(SELayer);

    struct ResLayerImpl : public torch::nn::Module
    {
        torch::nn::Conv2d conv1, conv2;
        SELayer se_layer1;


        ResLayerImpl(int Filters, int SEChannels);
        torch::Tensor forward(torch::Tensor x);

    private:
        torch::nn::Sequential all_layers;
    };
    TORCH_MODULE(ResLayer);


    struct LC0NetworkImpl : public torch::nn::Module
    {
        explicit LC0NetworkImpl(std::string const& weights_file,
                                torch::Device device = torch::Device(torch::DeviceType::CUDA),
                                bool use_fp16 = true);
        LC0NetworkImpl();
        // boards shape is {batch_size, 112, 8, 8}
        NetworkOutput forward(torch::Tensor boards);


        torch::Device device;
        torch::Dtype expected_dtype;

        // Threads using the Module
        std::atomic<int> n_user_threads = 0;

        pblczero::NetworkFormat::InputFormat input_format;
        torch::nn::Conv2d input_convolution;
        torch::nn::Sequential policy_head, value_head, moves_left_head, residual_tower;
    };

    TORCH_MODULE(LC0Network);
};
#endif //FIREFLY_LC0_NETWORK_H
