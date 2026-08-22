#pragma once

#include "gfx/fonts/font_atlas.hpp"
#include "gfx/fonts/text_vertex.hpp"
#include "vulkan/buffer.hpp"
#include "vulkan/graphics_pipeline.hpp"
#include "vulkan/image.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <vulkan/vulkan.h>

namespace ps::vulkan {
class CommandPool;
class Device;
class PhysicalDevice;

/// @brief Concrete Vulkan feature used to render Pixel Storm's diagnostics text.
///
/// TextOverlay owns the GPU copy of one font atlas, its sampling descriptors, the
/// text graphics pipeline and a small host-visible vertex buffer. Text content is
/// rebuilt on the CPU and uploaded only after Renderer has waited for the frame fence.
class TextOverlay final {
   public:
    TextOverlay(
        const PhysicalDevice& physicalDevice,
        const Device& device,
        const CommandPool& commandPool,
        VkQueue graphicsQueue,
        VkFormat colorAttachmentFormat,
        ps::gfx::fonts::FontAtlas fontAtlas
    );
    ~TextOverlay();

    TextOverlay(const TextOverlay&) = delete;
    TextOverlay& operator=(const TextOverlay&) = delete;
    TextOverlay(TextOverlay&&) = delete;
    TextOverlay& operator=(TextOverlay&&) = delete;

    void setText(std::string_view text);
    void uploadIfDirty();
    void record(VkCommandBuffer commandBuffer, VkExtent2D framebufferExtent) const;

   private:
    static constexpr std::size_t maxGlyphCount = 4096;
    static constexpr std::size_t maxVertexCount = maxGlyphCount * 6;

    void uploadAtlas(const PhysicalDevice& physicalDevice, const Device& device, const CommandPool& commandPool);
    void createImageView();
    void createSampler();
    void createDescriptors();
    void createPipeline(const Device& device, VkFormat colorAttachmentFormat);
    void destroyVulkanObjects() noexcept;

    VkDevice device_{VK_NULL_HANDLE};
    VkQueue graphicsQueue_{VK_NULL_HANDLE};

    ps::gfx::fonts::FontAtlas fontAtlas_;
    Image atlasImage_;
    Buffer vertexBuffer_;

    VkImageView atlasImageView_{VK_NULL_HANDLE};
    VkSampler atlasSampler_{VK_NULL_HANDLE};
    VkDescriptorSetLayout descriptorSetLayout_{VK_NULL_HANDLE};
    VkDescriptorPool descriptorPool_{VK_NULL_HANDLE};
    VkDescriptorSet descriptorSet_{VK_NULL_HANDLE};
    std::optional<GraphicsPipeline> pipeline_;

    std::string text_;
    std::vector<ps::gfx::fonts::TextVertex> vertices_;
    std::uint32_t vertexCount_{0};
    bool dirty_{false};
};

}  // namespace ps::vulkan
