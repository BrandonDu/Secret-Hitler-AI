#pragma once

#include <vector>
#include <string>

namespace secret_hitler
{

struct TrainSample {
    std::vector<float> x;
    int y;
};

void trainingLoggerInit(int inputDim);
void trainingLoggerRecord(const std::vector<double>& phi, int y, int role, double headFlag);
void trainingLoggerFlush(const std::string& path);

}
