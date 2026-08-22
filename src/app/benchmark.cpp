#include "app/benchmark.hpp"

#include <fstream>
#include <iomanip>
#include <stdexcept>

namespace ps::app {

void writeBenchmarkCsv(const std::filesystem::path& path, std::span<const BenchmarkSample> samples) {
    std::ofstream output{path};
    if (!output) {
        throw std::runtime_error("Failed to open benchmark output file: " + path.string());
    }

    output << "elapsed_s,particles,fps,frame_wall_ms,simulation_ms,particle_upload_ms,fence_wait_ms\n";
    output << std::fixed << std::setprecision(6);

    for (const BenchmarkSample& sample : samples) {
        const FrameMetrics& metrics = sample.metrics;
        output << sample.elapsedTimeSeconds << ',' << sample.particleCount << ',' << metrics.fps << ',' << metrics.frameWallTimeMs << ','
               << metrics.simulationTimeMs << ',' << metrics.particleUploadTimeMs << ',' << metrics.fenceWaitTimeMs << '\n';
    }

    if (!output) {
        throw std::runtime_error("Failed to write benchmark output file: " + path.string());
    }
}

}  // namespace ps::app
