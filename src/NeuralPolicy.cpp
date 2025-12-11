#include "NeuralPolicy.hpp"

#include <mutex>
#include <torch/script.h>

namespace secret_hitler {

namespace {

torch::jit::script::Module g_tmodel;
bool g_tloaded = false;
std::once_flag g_tonce;
int g_tinputDim = 0;

int roleToIndex(Role r) {
  switch (r) {
  case Role::Liberal:
    return 0;
  case Role::Fascist:
    return 1;
  case Role::Hitler:
    return 2;
  default:
    return 0;
  }
}

double forwardTransformer(const std::vector<double> &phi, Role r,
                          double headFlag) {
  if (!g_tloaded || g_tinputDim <= 0)
    return 0.5;

  std::vector<double> feat;
  feat.reserve(g_tinputDim);

  for (double v : phi)
    feat.push_back(v);

  int ridx = roleToIndex(r);
  feat.push_back(ridx == 0 ? 1.0 : 0.0);
  feat.push_back(ridx == 1 ? 1.0 : 0.0);
  feat.push_back(ridx == 2 ? 1.0 : 0.0);
  feat.push_back(headFlag);

  while ((int)feat.size() < g_tinputDim)
    feat.push_back(0.0);

  torch::Tensor x = torch::from_blob(const_cast<double *>(feat.data()),
                                     {1, 1, g_tinputDim}, torch::kDouble)
                        .clone();

  x = x.to(torch::kFloat);

  torch::Tensor out = g_tmodel.forward({x}).toTensor();
  // Model outputs single logit, need to squeeze all dimensions and apply
  // sigmoid
  out = out.squeeze();

  double logit = out.item<double>();
  // Apply sigmoid to convert logit to probability
  double p_yes = 1.0 / (1.0 + std::exp(-logit));
  return p_yes;
}

} // namespace

void initTransformerModel(const std::string &path, int inputDim) {
  std::call_once(g_tonce, [&]() {
    g_tmodel = torch::jit::load(path);
    g_tmodel.eval();
    g_tinputDim = inputDim;
    g_tloaded = true;
  });
}

double transformerVoteProb(const std::vector<double> &phi, Role r) {
  return forwardTransformer(phi, r, 1.0);
}

double transformerEnactProb(const std::vector<double> &phi, Role r) {
  return forwardTransformer(phi, r, 0.0);
}

} // namespace secret_hitler
