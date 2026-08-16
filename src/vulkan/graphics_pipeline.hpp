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
    /// @brief Creates the graphics pipeline for the swapchain color format.
    /// @param device Logical device that owns the pipeline and pipeline layout.
    /// @param swapchain Swapchain providing the color attachment format.
    /// @throws std::runtime_error If shader loading or Vulkan pipeline creation fails.
    GraphicsPipeline(const Device& device, const Swapchain& swapchain);

    /// @brief Destroys the owned pipeline and pipeline layout.
    ~GraphicsPipeline();

    GraphicsPipeline(const GraphicsPipeline&) = delete;
    GraphicsPipeline& operator=(const GraphicsPipeline&) = delete;

    GraphicsPipeline(GraphicsPipeline&&) = delete;
    GraphicsPipeline& operator=(GraphicsPipeline&&) = delete;

    /// @brief Returns the native Vulkan graphics-pipeline handle.
    [[nodiscard("The Vulkan graphics pipeline handle must be used")]]
    VkPipeline nativeHandle() const noexcept;

    /// @brief Returns the pipeline layout used for push constants and resource bindings.
    [[nodiscard("The Vulkan graphics pipeline layout handle must be used")]]
    VkPipelineLayout layout() const noexcept;

   private:
    VkDevice device_{VK_NULL_HANDLE};
    VkPipelineLayout layout_{VK_NULL_HANDLE};
    VkPipeline handle_{VK_NULL_HANDLE};
};

}  // namespace ps::vulkan
