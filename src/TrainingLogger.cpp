#include "TrainingLogger.hpp"
#include <fstream>
#include <iostream>
#ifdef _OPENMP
#include <omp.h>
#endif

namespace secret_hitler
{

static std::vector<TrainSample> g_samples;
static int g_inputDim = 0;

void trainingLoggerInit(int inputDim)
{
    g_inputDim = inputDim;
    g_samples.clear();
    std::cerr << "[TrainingLoggerInit] inputDim = " << g_inputDim << std::endl;
}

void trainingLoggerRecord(const std::vector<double>& phi, int y, int role, double headFlag)
{
    if (g_inputDim <= 0)
    {
        std::cerr << "[TrainingLoggerRecord] ERROR: g_inputDim <= 0 ("
                  << g_inputDim << "). Did you call trainingLoggerInit()?\n";
        return;
    }

    // For sanity, check how big phi is compared to g_inputDim.
    int baseDim = static_cast<int>(phi.size()) + 4; // +3 role one-hot, +1 headFlag
    if (baseDim > g_inputDim)
    {
        std::cerr << "[TrainingLoggerRecord] WARNING: phi.size() + 4 = "
                  << baseDim << " > g_inputDim = " << g_inputDim
                  << " (features will be truncated when written)\n";
    }

    TrainSample s;
    s.x.reserve(g_inputDim);

    // Copy manual features
    for (double v : phi)
        s.x.push_back(static_cast<float>(v));

    // One-hot encode role. Adjust mapping if your Role enum uses different ints.
    s.x.push_back(role == 0 ? 1.f : 0.f);
    s.x.push_back(role == 1 ? 1.f : 0.f);
    s.x.push_back(role == 2 ? 1.f : 0.f);

    // Head flag (1.0 for vote, 0.0 for enact in your selfPlayGen)
    s.x.push_back(static_cast<float>(headFlag));

    // Pad to g_inputDim
    while ((int)s.x.size() < g_inputDim)
        s.x.push_back(0.f);

    s.y = y;

    g_samples.push_back(std::move(s));

    // Debug: print occasionally.
    int N = static_cast<int>(g_samples.size());
    if (N <= 10 || (N % 1000 == 0))
    {
        std::cerr << "[TrainingLoggerRecord] sample #" << N
                  << " (phi_dim=" << phi.size()
                  << ", y=" << y
                  << ", role=" << role
                  << ", headFlag=" << headFlag << ")\n";
    }
}

void trainingLoggerFlush(const std::string& path)
{
    int N = static_cast<int>(g_samples.size());
    std::cerr << "[TrainingLoggerFlush] writing " << N
              << " samples with inputDim = " << g_inputDim
              << " to file: " << path
#ifdef _OPENMP
              << " (thread " << omp_get_thread_num() << ")"
#endif
              << std::endl;

    std::ofstream out(path, std::ios::binary);
    if (!out)
    {
        std::cerr << "[TrainingLoggerFlush] ERROR: could not open file: "
                  << path << std::endl;
        return;
    }

    // Write header
    out.write(reinterpret_cast<const char*>(&N), sizeof(int));
    out.write(reinterpret_cast<const char*>(&g_inputDim), sizeof(int));

    // Write each sample: g_inputDim floats + int label
    for (auto& s : g_samples)
    {
        out.write(reinterpret_cast<const char*>(s.x.data()),
                  g_inputDim * sizeof(float));
        out.write(reinterpret_cast<const char*>(&s.y), sizeof(int));
    }

    std::cerr << "[TrainingLoggerFlush] done.\n";
}

} // namespace secret_hitler
