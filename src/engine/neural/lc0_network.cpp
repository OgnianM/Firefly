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

#include <engine/neural/lc0_network.h>
#include <external/LeelaUtils/network_legacy.h>
#include <iostream>



namespace T = torch;
namespace nn = torch::nn;
using namespace lc0;
using namespace std;


void lc0::NetworkOutput::to(T::Device& device)
{
    this->policy = this->policy.to(device);
    this->value = this->value.to(device);
    this->moves_left = this->moves_left.to(device);
}

nn::Conv2d make_conv2d(int in_channels, int out_channels, int kernel_size=3, int stride=1)
{
    return nn::Conv2d(nn::Conv2dOptions(in_channels, out_channels, kernel_size)
    .stride(stride)
    .bias(true)
    .padding(torch::kSame));
}

SELayerImpl::SELayerImpl(int Filters, int SEChannels) :
        SEChannels(SEChannels),
        Filters(Filters),
        fc1(Filters, SEChannels),
        fc2(SEChannels, Filters*2)
{
    all_layers->push_back(nn::AvgPool2d(8));
    all_layers->push_back(nn::Flatten(nn::FlattenOptions().start_dim(1)));
    all_layers->push_back(fc1);
    all_layers->push_back(nn::ReLU());
    all_layers->push_back(fc2);


    this->register_module("fc1", fc1);
    this->register_module("fc2", fc2);
}

torch::Tensor SELayerImpl::forward(torch::Tensor x)
{
    /*
    auto y = torch::avg_pool2d(x, 8);

    y = T::flatten(y, 1);
    y = torch::relu(fc1(y));
    y = fc2(y);

    auto zw = torch::split(y, Filters, 1);

    auto Z = torch::sigmoid(zw[0]);
    auto B = zw[1];
    B = T::reshape(B, {-1, Filters, 1, 1});

    return T::einsum("bd, bdij -> bdij", {Z, x}) + B;
    */
    auto zw = torch::split(all_layers->forward(x), Filters, 1);


    return T::einsum("bd, bdij -> bdij", {torch::sigmoid(zw[0]), x}) +
            T::reshape(zw[1], {-1, Filters, 1, 1});
}


ResLayerImpl::ResLayerImpl(int Filters, int SEChannels) :
        conv1(make_conv2d(Filters, Filters)),
        conv2(make_conv2d(Filters, Filters)),
        se_layer1(Filters, SEChannels)
{

    all_layers->push_back(conv1);
    all_layers->push_back(nn::ReLU());
    all_layers->push_back(conv2);
    all_layers->push_back(se_layer1);


    this->register_module("conv1", conv1);
    this->register_module("conv2", conv2);
    this->register_module("se_layer1", se_layer1);
}

torch::Tensor ResLayerImpl::forward(torch::Tensor x)
{
    /*
    auto y = T::relu(conv1(x));
    y = conv2(y);
    y = se_layer1(y);

    return T::relu(x + y);
    */
    return T::relu(x + all_layers->forward(x));
}

LC0NetworkImpl::LC0NetworkImpl() : input_convolution(nullptr), device(torch::DeviceType::CPU){

}

LC0NetworkImpl::LC0NetworkImpl(std::string const& weights_file, torch::Device device, bool use_fp16) : input_convolution(nullptr),
device(device)
{
    int Filters, SEChannels, Blocks;
    auto NetworkDtype = use_fp16 ? T::Dtype::Half : T::Dtype::Float;
    expected_dtype = NetworkDtype;

    pblczero::Net weights;

    std::ifstream weights_stream(weights_file, std::ios::binary);
    weights.ParseFromIstream(&weights_stream);

    //auto weights = lczero::LoadWeightsFromFile(weights_file);


    if (weights.format().network_format().network() !=
    pblczero::NetworkFormat_NetworkStructure_NETWORK_SE_WITH_HEADFORMAT)
        throw std::invalid_argument("Unsupported network format: " +
        std::to_string(weights.format().network_format().network()));

    input_format = weights.format().network_format().input();


    lczero::LegacyWeights adapter(weights.weights());



    Blocks = adapter.residual.size();
    Filters = adapter.input.weights.size() / (NETWORK_INPUT_PLANES * 3 * 3);
    SEChannels = adapter.residual[0].se.b1.size();


    auto read_convlayer = [NetworkDtype, device](vector<float> const& input_weights,
            vector<float> const& input_biases, int in_c, int out_c, nn::Conv2d& convlayer)
    {
        auto kernel_size_f = sqrt(input_weights.size() / (in_c*out_c));

        int kernel_size = floor(kernel_size_f);

        if (kernel_size != kernel_size_f)
            throw std::logic_error("Weights size mismatch.");

        if ((in_c * out_c * kernel_size * kernel_size) != input_weights.size())
        {
            throw std::logic_error("Weights size mismatch: " +
            to_string(in_c * out_c * kernel_size * kernel_size) + " != " +
            to_string(input_weights.size()));
        }

        if (device.is_cpu())
        {
            convlayer->weight = T::from_blob((void *) input_weights.data(), {out_c, in_c, kernel_size, kernel_size},
                                             torch::TensorOptions().dtype(T::Dtype::Float)).to(NetworkDtype).clone();

            convlayer->bias = T::from_blob((void *) input_biases.data(), out_c,
                                           torch::TensorOptions().dtype(T::Dtype::Float)).to(NetworkDtype).clone();
        }
        else {
            convlayer->weight = T::from_blob((void *) input_weights.data(), {out_c, in_c, kernel_size, kernel_size},
                                             torch::TensorOptions().dtype(T::Dtype::Float)).to(device, NetworkDtype);

            convlayer->bias = T::from_blob((void *) input_biases.data(), out_c,
                                           torch::TensorOptions().dtype(T::Dtype::Float)).to(device, NetworkDtype);
        }
    };
    auto read_fc = [NetworkDtype, device](vector<float> const& input_weights,
                                  vector<float> const& input_biases, int in_c, int out_c, nn::Linear& fc)
    {
        if (input_biases.size() != out_c)
            throw std::invalid_argument(
                    "Input bias size (" +
                    to_string(input_biases.size()) +
                    ") != out_c ("  +
                    to_string(out_c) + ")");

        if (device.is_cpu())
        {
            fc->weight = T::from_blob((void *) input_weights.data(), {out_c, in_c},
                                      T::TensorOptions().dtype(T::Dtype::Float)).to(NetworkDtype).clone();

            fc->bias = T::from_blob((void *) input_biases.data(), out_c,
                                    T::TensorOptions().dtype(T::Dtype::Float)).to(NetworkDtype).clone();
        }
        else {
            fc->weight = T::from_blob((void *) input_weights.data(), {out_c, in_c},
                                      T::TensorOptions().dtype(T::Dtype::Float)).to(device, NetworkDtype);

            fc->bias = T::from_blob((void *) input_biases.data(), out_c,
                                    T::TensorOptions().dtype(T::Dtype::Float)).to(device, NetworkDtype);
        }
    };


    input_convolution = make_conv2d(NETWORK_INPUT_PLANES, Filters);
    read_convlayer(adapter.input.weights, adapter.input.biases, NETWORK_INPUT_PLANES, Filters, input_convolution);

    for (int i = 0; i < Blocks; i++)
    {
        ResLayer layer(Filters,SEChannels);

        read_convlayer(adapter.residual[i].conv1.weights, adapter.residual[i].conv1.biases, Filters,Filters, layer->conv1);
        read_convlayer(adapter.residual[i].conv2.weights, adapter.residual[i].conv2.biases, Filters,Filters, layer->conv2);

        read_fc(adapter.residual[i].se.w1, adapter.residual[i].se.b1, Filters, SEChannels, layer->se_layer1->fc1);
        read_fc(adapter.residual[i].se.w2, adapter.residual[i].se.b2, SEChannels, Filters*2, layer->se_layer1->fc2);

        residual_tower->push_back(layer);
    }



    auto policy_conv1 = make_conv2d(Filters, Filters);
    auto policy_conv2 = make_conv2d(Filters, 80);

    read_convlayer(adapter.policy1.weights, adapter.policy1.biases, Filters, Filters, policy_conv1);
    read_convlayer(adapter.policy.weights, adapter.policy.biases, Filters,80, policy_conv2);

    policy_head->push_back(policy_conv1);
    policy_head->push_back(nn::ReLU());
    policy_head->push_back(policy_conv2);
    policy_head->push_back(nn::Flatten(nn::FlattenOptions().start_dim(1)));

    auto value_conv1 = make_conv2d(Filters, 32, 1);
    read_convlayer(adapter.value.weights, adapter.value.biases, Filters,32,value_conv1);

    nn::Linear value_fc1(nn::LinearOptions(32*8*8,128).bias(true));
    nn::Linear value_fc2(nn::LinearOptions(128,3).bias(true)); // WDL

    read_fc(adapter.ip1_val_w, adapter.ip1_val_b, 32*8*8, 128, value_fc1);
    read_fc(adapter.ip2_val_w, adapter.ip2_val_b, 128, 3, value_fc2);

    value_head->push_back(value_conv1);
    value_head->push_back(nn::Flatten(nn::FlattenOptions().start_dim(1)));
    value_head->push_back(nn::ReLU());
    value_head->push_back(value_fc1);
    value_head->push_back(nn::ReLU());
    value_head->push_back(value_fc2);
    value_head->push_back(nn::Softmax(nn::SoftmaxOptions(1)));



    int MLHChannels = adapter.moves_left.biases.size(),
        FCSize = adapter.ip1_mov_b.size();

    if (MLHChannels) {
        auto mlh_conv1 = make_conv2d(Filters, MLHChannels);

        read_convlayer(adapter.moves_left.weights, adapter.moves_left.biases, Filters, MLHChannels, mlh_conv1);

        nn::Linear mlh_fc1(MLHChannels * 8 * 8, FCSize);
        nn::Linear mlh_fc2(FCSize, 1);

        read_fc(adapter.ip1_mov_w, adapter.ip1_mov_b, MLHChannels * 8 * 8, FCSize, mlh_fc1);
        read_fc(adapter.ip2_mov_w, adapter.ip2_mov_b, FCSize, 1, mlh_fc2);

        moves_left_head->push_back(mlh_conv1);
        moves_left_head->push_back(nn::Flatten(nn::FlattenOptions().start_dim(1)));
        moves_left_head->push_back(nn::ReLU());
        moves_left_head->push_back(mlh_fc1);
        moves_left_head->push_back(nn::ReLU());
        moves_left_head->push_back(mlh_fc2);
        moves_left_head->push_back(nn::ReLU());

    }
    else
    {
        moves_left_head->push_back(nn::ReLU());
    }



    this->register_module("input_convolution", input_convolution);
    this->register_module("residual_tower", residual_tower);
    this->register_module("value_head", value_head);
    this->register_module("policy_head", policy_head);
    this->register_module("moves_left_head", moves_left_head);
}

NetworkOutput LC0NetworkImpl::forward(torch::Tensor input_planes)
{
    auto x = T::relu(input_convolution(input_planes));

    x = residual_tower->forward(x);


    NetworkOutput output;
    output.policy = policy_head->forward(x);
    output.value = value_head->forward(x);
    output.moves_left = moves_left_head->forward(x);

    return output;
}
