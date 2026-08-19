#pragma once

#include "gfx/particles/particle.hpp"
#include "vulkan/buffer.hpp"
#include "vulkan/command_buffer.hpp"
#include "vulkan/command_pool.hpp"
#include "vulkan/frame_synchronization.hpp"
#include "vulkan/graphics_pipeline.hpp"

#include <chrono>
#include <cstdint>
#include <glm/mat4x4.hpp>
#include <span>
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
/// The renderer owns the resources that belong to frame rendering itself: the
/// graphics pipeline, graphics command pool, primary command buffer and frame
/// synchronization primitives. The current implementation also uploads and owns
/// the particle vertex buffer supplied at construction. It records rendering commands,
/// submits them to the graphics queue and presents the acquired swapchain image.
///
/// Bootstrap resources such as the Vulkan instance, surface, physical device,
/// logical device and swapchain remain separate objects. This keeps bootstrap
/// ownership explicit while centralizing per-frame Vulkan orchestration here.
///
/// @note The logical device and swapchain supplied at construction must outlive
/// this object.
class Renderer final {
   public:
    Renderer(
        const PhysicalDevice& physicalDevice,
        const Device& device,
        const Swapchain& swapchain,
        std::span<const ps::gfx::particles::Particle> particles
    );
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer&&) = delete;

    // Swapchain recreation is intentionally deferred; OUT_OF_DATE/SUBOPTIMAL
    // are reported as errors until the resize lifecycle is implemented.
    [[nodiscard]]
    FrameTimings drawFrame(const glm::mat4& viewProjection, std::span<const ps::gfx::particles::Particle> particles);

   private:
    void recordCommandBuffer(std::uint32_t imageIndex, const glm::mat4& viewProjection);

    VkDevice device_{VK_NULL_HANDLE};
    VkQueue graphicsQueue_{VK_NULL_HANDLE};
    VkQueue presentQueue_{VK_NULL_HANDLE};

    const Swapchain& swapchain_;

    GraphicsPipeline graphicsPipeline_;
    CommandPool commandPool_;
    CommandBuffer commandBuffer_;
    FrameSynchronization synchronization_;

    Buffer particleBuffer_;
    std::uint32_t particleCount_{0};
};

}  // namespace ps::vulkan
