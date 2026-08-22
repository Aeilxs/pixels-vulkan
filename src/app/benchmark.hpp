#pragma once

#include "app/frame_stats.hpp"

#include <cstddef>
#include <filesystem>
#include <span>

namespace ps::app {

struct BenchmarkSample {
    double elapsedTimeSeconds{};
    std::size_t particleCount{};
    FrameMetrics metrics{};
};

void writeBenchmarkCsv(const std::filesystem::path& path, std::span<const BenchmarkSample> samples);

}  // namespace ps::app
