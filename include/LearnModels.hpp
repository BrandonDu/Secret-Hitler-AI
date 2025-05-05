#pragma once
#include "Types.hpp"
#include <vector>

namespace secret_hitler
{

    extern std::vector<double> LR_w_yes;
    extern double LR_b_yes;

    extern std::vector<double> LR_w_enact_F;
    extern std::vector<double> LR_w_enact_L;
    extern double LR_b_enact_F;
    extern double LR_b_enact_L;

    void loadWeights(const std::string &f);
    void saveWeights(const std::string &f);
    void loadEnactWeights(const std::string &fF, const std::string &fL);
    void saveEnactWeights(
        const std::string &fF, double bF, const std::vector<double> &wF,
        const std::string &fL, double bL, const std::vector<double> &wL);

    void trainLogRegSGD(const std::vector<std::vector<double>> &X,
                        const std::vector<int> &Y,
                        std::vector<double> &w,
                        double &b,
                        double lr,
                        int epochs);

    void trainEnactModels(
        const std::vector<std::vector<double>> &X_enact,
        const std::vector<int> &Y_enact,
        const std::vector<Role> &roles,
        double lr,
        int epochs);
}