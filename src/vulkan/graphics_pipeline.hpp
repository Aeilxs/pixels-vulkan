#pragma once

#include <vulkan/vulkan.h>

namespace ps::vulkan {

class Device;
class Swapchain;

/// @brief Owns the graphics pipeline used by the current renderer.
///
/// The pipeline uses Vulkan 1.3 dynamic rendering and renders Particle records as
/// point-list primitives. Position and color are read from the particle vertex buffer;
/// viewport and scissor state remain dynamic so they can follow the swapchain extent.
/// No descriptor sets are used yet.
///
/// @note The logical Vulkan device used to create this pipeline must outlive it.
class GraphicsPipeline final {
   public:
    GraphicsPipeline(const Device& device, const Swapchain& swapchain);

    ~GraphicsPipeline();

    GraphicsPipeline(const GraphicsPipeline&) = delete;
    GraphicsPipeline& operator=(const GraphicsPipeline&) = delete;

    GraphicsPipeline(GraphicsPipeline&&) = delete;
    GraphicsPipeline& operator=(GraphicsPipeline&&) = delete;

    [[nodiscard("The Vulkan graphics pipeline handle must be used")]]
    VkPipeline nativeHandle() const noexcept;

    [[nodiscard("The Vulkan graphics pipeline layout handle must be used")]]
    VkPipelineLayout layout() const noexcept;

   private:
    VkDevice device_{VK_NULL_HANDLE};
    VkPipelineLayout layout_{VK_NULL_HANDLE};
    VkPipeline handle_{VK_NULL_HANDLE};
};

}  // namespace ps::vulkan
