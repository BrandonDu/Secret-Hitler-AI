#pragma once
#include <chrono>
#include <string>

void printProgressBar(const std::string&    prefix,
    int                   current,
    int                   total,
    int                   width,
    const std::chrono::steady_clock::time_point& startTime);