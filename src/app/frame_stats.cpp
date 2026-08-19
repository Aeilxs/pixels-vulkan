#include "app/frame_stats.hpp"

#include <chrono>

namespace ps::app {
namespace {
constexpr auto metricsRefreshInterval = std::chrono::milliseconds{500};
}

bool FrameStats::push(const FrameSample& sample) {
    const auto now = std::chrono::steady_clock::now();

    if (!windowStartTime_.has_value()) {
        // push() is called at the end of a frame. Anchor the first statistics
        // window to the beginning of that first measured frame.
        windowStartTime_ = now - sample.frameWallTime;
    }

    totalFrameWallTime_ += sample.frameWallTime;
    totalSimulationTime_ += sample.simulationTime;
    totalParticleUploadTime_ += sample.particleUploadTime;
    totalFenceWaitTime_ += sample.fenceWaitTime;
    ++frameCount_;

    const auto windowDuration = now - *windowStartTime_;
    if (windowDuration < metricsRefreshInterval) {
        return false;
    }

    updateMetrics(windowDuration);
    resetAccumulation();
    windowStartTime_ = now;
    return true;
}

const FrameMetrics& FrameStats::metrics() const noexcept {
    return metrics_;
}

void FrameStats::updateMetrics(std::chrono::steady_clock::duration windowDuration) noexcept {
    if (frameCount_ == 0) {
        return;
    }

    const double frameCount = static_cast<double>(frameCount_);

    metrics_.fps = frameCount / std::chrono::duration<double>(windowDuration).count();
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
