#pragma once

#include <torch/script.h>
#include <vector>
#include <string>

namespace secret_hitler {

class TransformerPolicy {
public:
    TransformerPolicy(const std::string& path, int inputDim);

    float eval(const std::vector<double>& phi, int role, double headFlag);

private:
    torch::jit::script::Module module;
    int input_dim;
};

}  // namespace secret_hitler
