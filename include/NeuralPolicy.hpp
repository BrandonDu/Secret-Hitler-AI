#pragma once

#include <vector>
#include <string>
#include "Types.hpp"

namespace secret_hitler
{

void initTransformerModel(const std::string& path, int inputDim);

double transformerVoteProb(const std::vector<double>& phi, Role r);
double transformerEnactProb(const std::vector<double>& phi, Role r);

}
