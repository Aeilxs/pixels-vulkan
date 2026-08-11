#pragma once

#include "renderer/vulkan/command_buffer.hpp"
#include "renderer/vulkan/command_pool.hpp"
#include "renderer/vulkan/frame_synchronization.hpp"
#include "renderer/vulkan/graphics_pipeline.hpp"

#include <chrono>
#include <cstdint>
#include <vulkan/vulkan.h>

namespace ps::renderer::vulkan {

class Device;
class PhysicalDevice;
class Swapchain;

/// @brief Coordinates Vulkan resources and commands required to render one frame.
///
/// The renderer owns the resources that belong to frame rendering itself: the
/// graphics pipeline, graphics command pool, primary command buffer and frame
/// synchronization primitives. It records rendering commands, submits them to
/// the graphics queue and presents the acquired swapchain image.
///
/// Bootstrap resources such as the Vulkan instance, surface, physical device,
/// logical device and swapchain remain separate objects. This keeps bootstrap
/// ownership explicit while centralizing per-frame Vulkan orchestration here.
///
/// @note The logical device and swapchain supplied at construction must outlive
/// this object.
class Renderer final {
   public:
    /// @brief Creates the resources required by the initial Vulkan renderer.
    /// @param physicalDevice Physical device providing the graphics queue family.
    /// @param device Logical device and graphics/presentation queues used for rendering.
    /// @param swapchain Swapchain whose images are rendered and presented.
    /// @throws std::runtime_error If an owned Vulkan resource cannot be created.
    Renderer(const PhysicalDevice& physicalDevice, const Device& device, const Swapchain& swapchain);

    /// @brief Waits for outstanding device work before owned renderer resources are destroyed.
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer&&) = delete;

    /// @brief Renders and presents one frame.
    ///
    /// Waits for the previous frame, acquires a swapchain image, records the
    /// primary command buffer, submits it to the graphics queue and presents
    /// the completed image.
    ///
    /// @throws std::runtime_error If a Vulkan operation fails or the swapchain
    /// becomes out of date. Swapchain recreation is intentionally deferred until
    /// the resize lifecycle is implemented.
    void drawFrame();

   private:
    void createVertexBuffer();
    void destroyVertexBuffer();
    void createIndexBuffer();
    void destroyIndexBuffer();

    VkBuffer vertexBuffer_ = VK_NULL_HANDLE;
    VkBuffer indexBuffer_ = VK_NULL_HANDLE;
    VkDeviceMemory indexBufferMemory_ = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory_ = VK_NULL_HANDLE;
    /// @brief Records all commands required to draw the current triangle frame.
    /// @param imageIndex Index of the swapchain image acquired for this frame.
    /// @throws std::runtime_error If command-buffer recording cannot begin or end.
    void recordCommandBuffer(std::uint32_t imageIndex, const std::chrono::steady_clock::duration& duration);

    VkPhysicalDevice physicalDevice_{VK_NULL_HANDLE};
    VkDevice device_{VK_NULL_HANDLE};
    VkQueue graphicsQueue_{VK_NULL_HANDLE};
    VkQueue presentQueue_{VK_NULL_HANDLE};

    const Swapchain& swapchain_;

    GraphicsPipeline graphicsPipeline_;
    CommandPool commandPool_;
    CommandBuffer commandBuffer_;
    FrameSynchronization synchronization_;

    std::chrono::steady_clock::time_point startTime_;
};

}  // namespace ps::renderer::vulkan
