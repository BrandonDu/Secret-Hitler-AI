#include "LearnModels.hpp"
#include "GameState.hpp"
#include "Features.hpp"
#include "ProgressBar.hpp"

#include <fstream>
#include <iostream>
#include <cassert>

namespace secret_hitler
{

    std::vector<double> LR_w_vote_F, LR_w_vote_L;
    double LR_b_vote_F = 0.0, LR_b_vote_L = 0.0;

    std::vector<double> LR_w_enact_F, LR_w_enact_L;
    double LR_b_enact_F = 0.0, LR_b_enact_L = 0.0;

    void loadVoteWeights(const std::string &fF, const std::string &fL)
    {
        std::ifstream inF(fF), inL(fL);
        if (!inF || !inL)
        {
            std::cerr << "Warning: vote weight files not found, starting from zeros\n";
            return;
        }
        inF >> LR_b_vote_F;
        for (auto &w : LR_w_vote_F)
            inF >> w;
        inL >> LR_b_vote_L;
        for (auto &w : LR_w_vote_L)
            inL >> w;
    }

    void saveVoteWeights(
        const std::string &fF,
        const std::string &fL)
    {
        std::ofstream outF(fF), outL(fL);
        if (outF)
        {
            outF << LR_b_vote_F;
            for (auto &w : LR_w_vote_F)
                outF << ' ' << w;
            outF << '\n';
        }
        if (outL)
        {
            outL << LR_b_vote_L;
            for (auto &w : LR_w_vote_L)
                outL << ' ' << w;
            outL << '\n';
        }
    }

    void loadEnactWeights(const std::string &fF, const std::string &fL)
    {
        std::ifstream inF(fF), inL(fL);
        if (!inF || !inL)
        {
            std::cerr << "Warning: enact weight files not found, starting from zeros\n";
            return;
        }
        inF >> LR_b_enact_F;
        for (auto &w : LR_w_enact_F)
            inF >> w;
        inL >> LR_b_enact_L;
        for (auto &w : LR_w_enact_L)
            inL >> w;
    }

    void saveEnactWeights(
        const std::string &fF,
        const std::string &fL)
    {
        std::ofstream outF(fF), outL(fL);
        if (outF)
        {
            outF << LR_b_enact_F;
            for (auto &w : LR_w_enact_F)
                outF << ' ' << w;
            outF << '\n';
        }
        if (outL)
        {
            outL << LR_b_enact_L;
            for (auto &w : LR_w_enact_L)
                outL << ' ' << w;
            outL << '\n';
        }
    }

    void trainLogRegSGD(const std::vector<std::vector<double>> &X,
                        const std::vector<int> &Y,
                        std::vector<double> &w,
                        double &b,
                        double lr,
                        int epochs)
    {
        int N = (int)X.size(), d = (int)w.size();
        std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<int> dist(0, N - 1);

        auto startTime = std::chrono::steady_clock::now();

        int total = epochs * N, width = 50;
        for (int ep = 0; ep < epochs; ++ep)
            for (int it = 0; it < N; ++it)
            {
                int idx = dist(rng);
                double z = b;
                for (int j = 0; j < d; ++j)
                    z += w[j] * X[idx][j];
                double p = 1.0 / (1.0 + std::exp(-z)), err = p - Y[idx];
                for (int j = 0; j < d; ++j)
                    w[j] -= lr * err * X[idx][j];
                b -= lr * err;
                int step = ep * N + it + 1;
                printProgressBar("Training",
                                 step,
                                 total,
                                 width,
                                 startTime);
            }
        std::cout << "\n";
    }

    void trainEnactModels(
        const std::vector<std::vector<double>> &X_enact,
        const std::vector<int> &Y_enact,
        const std::vector<Role> &roles,
        double lr,
        int epochs)
    {
        assert(X_enact.size() == Y_enact.size());
        std::vector<std::vector<double>> X_F, X_L;
        std::vector<int> Y_F, Y_L;
        X_F.reserve(X_enact.size());
        Y_F.reserve(Y_enact.size());
        X_L.reserve(X_enact.size());
        Y_L.reserve(Y_enact.size());
        for (size_t i = 0; i < X_enact.size(); ++i)
        {
            if (roles[i] == Role::Fascist || roles[i] == Role::Hitler)
            {
                X_F.push_back(X_enact[i]);
                Y_F.push_back(Y_enact[i]);
            }
            else
            {
                X_L.push_back(X_enact[i]);
                Y_L.push_back(Y_enact[i]);
            }
        }

        if (!X_F.empty())
            trainLogRegSGD(X_F, Y_F, LR_w_enact_F, LR_b_enact_F, lr, epochs);
        if (!X_L.empty())
            trainLogRegSGD(X_L, Y_L, LR_w_enact_L, LR_b_enact_L, lr, epochs);
    }

}