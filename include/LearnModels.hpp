#pragma once
#include "Types.hpp"
#include <vector>

namespace secret_hitler
{

    extern std::vector<double> LR_w_vote_F, LR_w_vote_L;
    extern double LR_b_vote_F, LR_b_vote_L;

    extern std::vector<double> LR_w_enact_F;
    extern double LR_b_enact_F;

    void loadVoteWeights(const std::string &fF, const std::string &fL);
    void saveVoteWeights(const std::string &fF, const std::string &fL);
    void loadEnactWeights(const std::string &fF, const std::string &fL);
    void saveEnactWeights(
        const std::string &fF,
        const std::string &fL);

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