#pragma once

#include <string>
#include <vector>
#include <vulkan/vulkan.h>

namespace ps::vulkan {

class Device;

struct GraphicsPipelineConfig {
    std::string vertexShader;
    std::string fragmentShader;
    VkPrimitiveTopology topology{VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST};

    std::vector<VkVertexInputBindingDescription> vertexBindings;
    std::vector<VkVertexInputAttributeDescription> vertexAttributes;
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
    std::vector<VkPushConstantRange> pushConstantRanges;
};

/// @brief Owns one Vulkan graphics pipeline and its pipeline layout.
///
/// Only the states that currently differ between Pixel Storm's particle and text
/// pipelines are configurable. Rasterization, blending, multisampling and dynamic
/// viewport/scissor state intentionally remain shared until another real use case
/// requires exposing them.
class GraphicsPipeline final {
   public:
    GraphicsPipeline(const Device& device, VkFormat colorAttachmentFormat, const GraphicsPipelineConfig& config);
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
