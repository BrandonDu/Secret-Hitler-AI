#pragma once

#include <array>



namespace secret_hitler
{
    struct Assign {
        int hitler, fascist;
    };

    struct BeliefState
    {
        inline static const std::array<Assign,20> assignments = {{
            {0,1},{0,2},{0,3},{0,4},
            {1,0},{1,2},{1,3},{1,4},
            {2,0},{2,1},{2,3},{2,4},
            {3,0},{3,1},{3,2},{3,4},
            {4,0},{4,1},{4,2},{4,3}
        }};
        std::array<double,20> P;
        
        std::array<double,5> marginalHitler() const {
        std::array<double,5> pH{};  
        for (int k = 0; k < 20; ++k) {
            auto &A = assignments[k];
            pH[A.hitler] += P[k];
        }
        return pH;
        }

        std::array<double,5> marginalFascist() const {
        std::array<double,5> pF{};
        for (int k = 0; k < 20; ++k) {
            auto &A = assignments[k];
            pF[A.fascist] += P[k];
        }
        return pF;
        }
    };

}