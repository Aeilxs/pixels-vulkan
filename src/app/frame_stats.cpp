#include "app/frame_stats.hpp"

#include <chrono>

namespace ps::app {
namespace {
constexpr auto metricsRefreshInterval = std::chrono::milliseconds{500};
}

bool FrameStats::push(const FrameSample& sample) {
    totalFrameWallTime_ += sample.frameWallTime;
    totalSimulationTime_ += sample.simulationTime;
    totalParticleUploadTime_ += sample.particleUploadTime;
    totalFenceWaitTime_ += sample.fenceWaitTime;
    ++frameCount_;

    if (totalFrameWallTime_ < metricsRefreshInterval) {
        return false;
    }

    updateMetrics();
    resetAccumulation();
    return true;
}

const FrameMetrics& FrameStats::metrics() const noexcept {
    return metrics_;
}

void FrameStats::updateMetrics() noexcept {
    if (frameCount_ == 0) {
        return;
    }

    const double frameCount = static_cast<double>(frameCount_);
    const double totalFrameWallTimeSeconds = std::chrono::duration<double>(totalFrameWallTime_).count();

    metrics_.fps = frameCount / totalFrameWallTimeSeconds;
    metrics_.frameWallTimeMs = std::chrono::duration<double, std::milli>(totalFrameWallTime_).count() / frameCount;
    metrics_.simulationTimeMs = std::chrono::duration<double, std::milli>(totalSimulationTime_).count() / frameCount;
    metrics_.particleUploadTimeMs = std::chrono::duration<double, std::milli>(totalParticleUploadTime_).count() / frameCount;
    metrics_.fenceWaitTimeMs = std::chrono::duration<double, std::milli>(totalFenceWaitTime_).count() / frameCount;
}

void FrameStats::resetAccumulation() noexcept {
    totalFrameWallTime_ = {};
    totalSimulationTime_ = {};
    totalParticleUploadTime_ = {};
    totalFenceWaitTime_ = {};
    frameCount_ = 0;
}

}  // namespace ps::app
