#include "ProgressBar.hpp"
#include <iomanip>
#include <iostream>

#include <iostream>
#include <iomanip>
#include <locale>
#include <string>
#include <sstream>
#include <chrono>



void printProgressBar(const std::string&    prefix,
    int                   current,
    int                   total,
    int                   width,
    const std::chrono::steady_clock::time_point& startTime)
{
    if (total <= 0) total = 1;
    double units   = double(current) / total * width;
    int    full    = int(std::floor(units));         
    bool   half    = (units - full) >= 0.5;        


    auto now       = std::chrono::steady_clock::now();
    auto elapsed_s = std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count();
    int  remaining = int(elapsed_s * (total - current) / std::max(1, current));

    std::cout << '\r'
              << "[" << prefix << "] [";
    for (int i = 0; i < width; ++i)
    {
        if (i < full) 
            std::cout << '=';
        else if (i == full && half)
            std::cout << '-';
        else
            std::cout << ' ';
    }
    std::cout << "] "
              << std::setw(3) << int(double(current)/total * 100) << "%  "
              << "ETA: " << remaining << "s"
              << std::flush;
}