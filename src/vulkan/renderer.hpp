#pragma once

#include "gfx/fonts/font_atlas.hpp"
#include "vulkan/buffer.hpp"
#include "vulkan/command_buffer.hpp"
#include "vulkan/command_pool.hpp"
#include "vulkan/frame_synchronization.hpp"
#include "vulkan/graphics_pipeline.hpp"
#include "vulkan/text_overlay.hpp"

#include <chrono>
#include <cstdint>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <span>
#include <string_view>
#include <vulkan/vulkan.h>

namespace ps::vulkan {

class Device;
class PhysicalDevice;
class Swapchain;

struct FrameTimings {
    std::chrono::steady_clock::duration particleUploadTime{};
    std::chrono::steady_clock::duration fenceWaitTime{};
};

/// @brief Coordinates Vulkan resources and commands required to render one frame.
///
/// Renderer keeps frame orchestration explicit: wait, host uploads, image acquire,
/// command recording, submission and presentation. Particle rendering and the
/// diagnostics text overlay use separate graphics pipelines inside one dynamic-
/// rendering pass.
class Renderer final {
   public:
    Renderer(
        const PhysicalDevice& physicalDevice,
        const Device& device,
        const Swapchain& swapchain,
        std::span<const glm::vec2> particlePositions,
        std::span<const glm::vec4> particleColors,
        ps::gfx::fonts::FontAtlas fontAtlas
    );
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer&&) = delete;

    void setOverlayText(std::string_view text);

    // Swapchain recreation is intentionally deferred; OUT_OF_DATE/SUBOPTIMAL
    // are reported as errors until the resize lifecycle is implemented.
    [[nodiscard]]
    FrameTimings drawFrame(const glm::mat4& viewProjection, std::span<const glm::vec2> particlePositions);

   private:
    void recordCommandBuffer(std::uint32_t imageIndex, const glm::mat4& viewProjection);

    VkDevice device_{VK_NULL_HANDLE};
    VkQueue graphicsQueue_{VK_NULL_HANDLE};
    VkQueue presentQueue_{VK_NULL_HANDLE};

    const Swapchain& swapchain_;

    GraphicsPipeline particlePipeline_;
    CommandPool commandPool_;
    CommandBuffer commandBuffer_;
    FrameSynchronization synchronization_;

    Buffer particlePositionBuffer_;
    Buffer particleColorBuffer_;
    TextOverlay textOverlay_;
    std::uint32_t particleCount_{0};
};

}  // namespace ps::vulkan
