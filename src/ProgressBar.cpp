#include "ProgressBar.hpp"
#include <iomanip>
#include <iostream>

#include <iostream>
#include <iomanip>
#include <locale>
#include <string>
#include <sstream>
#include <chrono>

void printProgressBar(const std::string &prefix,
                      int current,
                      int total,
                      int width,
                      const std::chrono::steady_clock::time_point &startTime)
{
    static size_t maxLen = 0;
    std::ostringstream oss;
    float ratio = float(current) / total;
    int filled = int(ratio * width);

    oss << "[" << prefix << "] [";
    for (int i = 0; i < width; ++i)
        oss << (i < filled ? '=' : ' ');
    oss << "] " << std::setw(3) << int(ratio * 100) << "%  ETA: ";

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count();
    int remaining = int((elapsed / ratio) - elapsed);
    oss << remaining << "s";

    std::string out = oss.str();
    maxLen = std::max(maxLen, out.size());

    std::cout << '\r'
              << out
              << std::string(maxLen - out.size(), ' ')
              << std::flush;
}
