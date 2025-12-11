#include "TransformerPolicy.hpp"
#include <torch/torch.h>
#include <torch/script.h>

namespace secret_hitler {

TransformerPolicy::TransformerPolicy(const std::string& path, int inputDim)
    : input_dim(inputDim) {
    module = torch::jit::load(path);
    module.eval();
    torch::NoGradGuard guard;
    torch::set_num_threads(1);
}

float TransformerPolicy::eval(const std::vector<double>& phi, int role, double headFlag) {
    std::vector<float> x_vec;
    x_vec.reserve(input_dim);

    for (double v : phi) x_vec.push_back(static_cast<float>(v));

    x_vec.push_back(role == 0 ? 1.f : 0.f);
    x_vec.push_back(role == 1 ? 1.f : 0.f);
    x_vec.push_back(role == 2 ? 1.f : 0.f);

    x_vec.push_back(static_cast<float>(headFlag));

    while (static_cast<int>(x_vec.size()) < input_dim)
        x_vec.push_back(0.f);

    torch::Tensor x = torch::from_blob(
        x_vec.data(),
        {1, input_dim},
        torch::TensorOptions().dtype(torch::kFloat32)
    ).clone();

    std::vector<torch::jit::IValue> inputs;
    inputs.push_back(x);

    auto out = module.forward(inputs).toTensor();
    auto s = out.squeeze();
    return s.item<float>();
}

}  // namespace secret_hitler
