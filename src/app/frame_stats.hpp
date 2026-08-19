#pragma once

#include <chrono>
#include <cstddef>
#include <optional>

namespace ps::app {

struct FrameSample {
    std::chrono::steady_clock::duration frameWallTime{};
    std::chrono::steady_clock::duration simulationTime{};
    std::chrono::steady_clock::duration particleUploadTime{};
    std::chrono::steady_clock::duration fenceWaitTime{};
};

struct FrameMetrics {
    double fps = 0.0;
    double frameWallTimeMs = 0.0;
    double simulationTimeMs = 0.0;
    double particleUploadTimeMs = 0.0;
    double fenceWaitTimeMs = 0.0;
};

class FrameStats {
   public:
    /// @brief Returns true if the metrics have been updated and are ready to be queried.
    bool push(const FrameSample& sample);

    [[nodiscard]]
    const FrameMetrics& metrics() const noexcept;

   private:
    void updateMetrics(std::chrono::steady_clock::duration windowDuration) noexcept;
    void resetAccumulation() noexcept;

    FrameMetrics metrics_{};
    std::optional<std::chrono::steady_clock::time_point> windowStartTime_{};

    std::chrono::steady_clock::duration totalFrameWallTime_{};
    std::chrono::steady_clock::duration totalSimulationTime_{};
    std::chrono::steady_clock::duration totalParticleUploadTime_{};
    std::chrono::steady_clock::duration totalFenceWaitTime_{};
    std::size_t frameCount_{0};
};

}  // namespace ps::app
