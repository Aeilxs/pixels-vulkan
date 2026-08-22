#include "vulkan/text_overlay.hpp"

#include "gfx/fonts/text_geometry.hpp"
#include "vulkan/command_buffer.hpp"
#include "vulkan/command_pool.hpp"
#include "vulkan/device.hpp"
#include "vulkan/physical_device.hpp"

#include <cstddef>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace ps::vulkan {
namespace {
constexpr VkFormat atlasFormat = VK_FORMAT_R8_UNORM;
constexpr float overlayOriginX = 24.0F;
constexpr float overlayOriginY = 24.0F;

struct TextPushConstants {
    float framebufferWidth{};
    float framebufferHeight{};
};

[[nodiscard]]
GraphicsPipelineConfig makeTextPipelineConfig(VkDescriptorSetLayout descriptorSetLayout) {
    GraphicsPipelineConfig config{};
    config.vertexShader = "text.vert.spv";
    config.fragmentShader = "text.frag.spv";
    config.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkVertexInputBindingDescription binding{};
    binding.binding = 0;
    binding.stride = sizeof(ps::gfx::fonts::TextVertex);
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    config.vertexBindings.push_back(binding);

    VkVertexInputAttributeDescription positionAttribute{};
    positionAttribute.location = 0;
    positionAttribute.binding = 0;
    positionAttribute.format = VK_FORMAT_R32G32_SFLOAT;
    positionAttribute.offset = offsetof(ps::gfx::fonts::TextVertex, x);
    config.vertexAttributes.push_back(positionAttribute);

    VkVertexInputAttributeDescription uvAttribute{};
    uvAttribute.location = 1;
    uvAttribute.binding = 0;
    uvAttribute.format = VK_FORMAT_R32G32_SFLOAT;
    uvAttribute.offset = offsetof(ps::gfx::fonts::TextVertex, u);
    config.vertexAttributes.push_back(uvAttribute);

    config.descriptorSetLayouts.push_back(descriptorSetLayout);

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(TextPushConstants);
    config.pushConstantRanges.push_back(pushConstantRange);

    return config;
}

void checkResult(VkResult result, const char* operation) {
    if (result != VK_SUCCESS) {
        throw std::runtime_error{std::string{operation} + ": VkResult " + std::to_string(result)};
    }
}

}  // namespace

TextOverlay::TextOverlay(
    const PhysicalDevice& physicalDevice,
    const Device& device,
    const CommandPool& commandPool,
    VkQueue graphicsQueue,
    VkFormat colorAttachmentFormat,
    ps::gfx::fonts::FontAtlas fontAtlas
)
    : device_{device.nativeHandle()},
      graphicsQueue_{graphicsQueue},
      fontAtlas_{std::move(fontAtlas)},
      atlasImage_{
          physicalDevice,
          device,
          VkExtent2D{fontAtlas_.width(), fontAtlas_.height()},
          atlasFormat,
          VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      },
      vertexBuffer_{
          physicalDevice,
          device,
          sizeof(ps::gfx::fonts::TextVertex) * maxVertexCount,
          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      } {
    VkFormatProperties formatProperties{};
    vkGetPhysicalDeviceFormatProperties(physicalDevice.nativeHandle(), atlasFormat, &formatProperties);
    constexpr VkFormatFeatureFlags requiredFormatFeatures =
        VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_TRANSFER_DST_BIT;
    if ((formatProperties.optimalTilingFeatures & requiredFormatFeatures) != requiredFormatFeatures) {
        throw std::runtime_error{"VK_FORMAT_R8_UNORM does not support the required sampled/transfer-destination usage."};
    }

    uploadAtlas(physicalDevice, device, commandPool);

    try {
        createImageView();
        createSampler();
        createDescriptors();
        createPipeline(device, colorAttachmentFormat);
    } catch (...) {
        destroyVulkanObjects();
        throw;
    }
}

TextOverlay::~TextOverlay() {
    destroyVulkanObjects();
}

void TextOverlay::setText(std::string_view text) {
    if (text == text_) {
        return;
    }

    text_.assign(text);
    vertices_ = ps::gfx::fonts::buildTextVertices(text_, fontAtlas_, {overlayOriginX, overlayOriginY});

    if (vertices_.size() > maxVertexCount) {
        throw std::length_error{"Text overlay exceeds its fixed vertex-buffer capacity."};
    }

    vertexCount_ = static_cast<std::uint32_t>(vertices_.size());
    dirty_ = true;
}

void TextOverlay::uploadIfDirty() {
    if (!dirty_) {
        return;
    }

    if (!vertices_.empty()) {
        vertexBuffer_.write(vertices_.data(), sizeof(ps::gfx::fonts::TextVertex) * vertices_.size());
    }

    dirty_ = false;
}

void TextOverlay::record(VkCommandBuffer commandBuffer, VkExtent2D framebufferExtent) const {
    if (vertexCount_ == 0 || !pipeline_.has_value()) {
        return;
    }

    const VkBuffer vertexBuffer = vertexBuffer_.nativeHandle();
    constexpr VkDeviceSize vertexBufferOffset = 0;

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_->nativeHandle());
    vkCmdBindDescriptorSets(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline_->layout(),
        0,
        1,
        &descriptorSet_,
        0,
        nullptr
    );
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, &vertexBufferOffset);

    const TextPushConstants pushConstants{
        .framebufferWidth = static_cast<float>(framebufferExtent.width),
        .framebufferHeight = static_cast<float>(framebufferExtent.height),
    };
    vkCmdPushConstants(
        commandBuffer,
        pipeline_->layout(),
        VK_SHADER_STAGE_VERTEX_BIT,
        0,
        sizeof(pushConstants),
        &pushConstants
    );

    vkCmdDraw(commandBuffer, vertexCount_, 1, 0, 0);
}

void TextOverlay::uploadAtlas(const PhysicalDevice& physicalDevice, const Device& device, const CommandPool& commandPool) {
    const VkDeviceSize atlasByteSize = static_cast<VkDeviceSize>(fontAtlas_.pixels().size());
    Buffer stagingBuffer{
        physicalDevice,
        device,
        atlasByteSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    };
    stagingBuffer.write(fontAtlas_.pixels().data(), atlasByteSize);

    CommandBuffer uploadCommandBuffer{device, commandPool};
    const VkCommandBuffer commandBuffer = uploadCommandBuffer.nativeHandle();

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    checkResult(vkBeginCommandBuffer(commandBuffer, &beginInfo), "Failed to begin font-atlas upload command buffer");

    VkImageMemoryBarrier2 toTransferDestination{};
    toTransferDestination.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    toTransferDestination.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
    toTransferDestination.srcAccessMask = VK_ACCESS_2_NONE;
    toTransferDestination.dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
    toTransferDestination.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    toTransferDestination.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    toTransferDestination.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    toTransferDestination.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toTransferDestination.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toTransferDestination.image = atlasImage_.nativeHandle();
    toTransferDestination.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    toTransferDestination.subresourceRange.baseMipLevel = 0;
    toTransferDestination.subresourceRange.levelCount = 1;
    toTransferDestination.subresourceRange.baseArrayLayer = 0;
    toTransferDestination.subresourceRange.layerCount = 1;

    VkDependencyInfo toTransferDestinationDependency{};
    toTransferDestinationDependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    toTransferDestinationDependency.imageMemoryBarrierCount = 1;
    toTransferDestinationDependency.pImageMemoryBarriers = &toTransferDestination;
    vkCmdPipelineBarrier2(commandBuffer, &toTransferDestinationDependency);

    VkBufferImageCopy copyRegion{};
    copyRegion.bufferOffset = 0;
    copyRegion.bufferRowLength = 0;
    copyRegion.bufferImageHeight = 0;
    copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.imageSubresource.mipLevel = 0;
    copyRegion.imageSubresource.baseArrayLayer = 0;
    copyRegion.imageSubresource.layerCount = 1;
    copyRegion.imageOffset = {0, 0, 0};
    copyRegion.imageExtent = {fontAtlas_.width(), fontAtlas_.height(), 1};

    vkCmdCopyBufferToImage(
        commandBuffer,
        stagingBuffer.nativeHandle(),
        atlasImage_.nativeHandle(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &copyRegion
    );

    VkImageMemoryBarrier2 toShaderRead{};
    toShaderRead.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    toShaderRead.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
    toShaderRead.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    toShaderRead.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
    toShaderRead.dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
    toShaderRead.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    toShaderRead.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    toShaderRead.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toShaderRead.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toShaderRead.image = atlasImage_.nativeHandle();
    toShaderRead.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    toShaderRead.subresourceRange.baseMipLevel = 0;
    toShaderRead.subresourceRange.levelCount = 1;
    toShaderRead.subresourceRange.baseArrayLayer = 0;
    toShaderRead.subresourceRange.layerCount = 1;

    VkDependencyInfo toShaderReadDependency{};
    toShaderReadDependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    toShaderReadDependency.imageMemoryBarrierCount = 1;
    toShaderReadDependency.pImageMemoryBarriers = &toShaderRead;
    vkCmdPipelineBarrier2(commandBuffer, &toShaderReadDependency);

    checkResult(vkEndCommandBuffer(commandBuffer), "Failed to end font-atlas upload command buffer");

    VkFenceCreateInfo fenceCreateInfo{};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    VkFence uploadFence = VK_NULL_HANDLE;
    checkResult(vkCreateFence(device_, &fenceCreateInfo, nullptr, &uploadFence), "Failed to create font-atlas upload fence");

    try {
        VkCommandBufferSubmitInfo commandBufferInfo{};
        commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        commandBufferInfo.commandBuffer = commandBuffer;

        VkSubmitInfo2 submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
        submitInfo.commandBufferInfoCount = 1;
        submitInfo.pCommandBufferInfos = &commandBufferInfo;

        checkResult(vkQueueSubmit2(graphicsQueue_, 1, &submitInfo, uploadFence), "Failed to submit font-atlas upload");
        checkResult(
            vkWaitForFences(device_, 1, &uploadFence, VK_TRUE, std::numeric_limits<std::uint64_t>::max()),
            "Failed to wait for font-atlas upload"
        );
    } catch (...) {
        vkQueueWaitIdle(graphicsQueue_);
        vkDestroyFence(device_, uploadFence, nullptr);
        throw;
    }

    vkDestroyFence(device_, uploadFence, nullptr);
}

void TextOverlay::createImageView() {
    VkImageViewCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = atlasImage_.nativeHandle();
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = atlasImage_.format();
    createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;

    checkResult(vkCreateImageView(device_, &createInfo, nullptr, &atlasImageView_), "Failed to create font-atlas image view");
}

void TextOverlay::createSampler() {
    VkSamplerCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    createInfo.magFilter = VK_FILTER_LINEAR;
    createInfo.minFilter = VK_FILTER_LINEAR;
    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    createInfo.mipLodBias = 0.0F;
    createInfo.anisotropyEnable = VK_FALSE;
    createInfo.maxAnisotropy = 1.0F;
    createInfo.compareEnable = VK_FALSE;
    createInfo.minLod = 0.0F;
    createInfo.maxLod = 0.0F;
    createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    createInfo.unnormalizedCoordinates = VK_FALSE;

    checkResult(vkCreateSampler(device_, &createInfo, nullptr, &atlasSampler_), "Failed to create font-atlas sampler");
}

void TextOverlay::createDescriptors() {
    VkDescriptorSetLayoutBinding atlasBinding{};
    atlasBinding.binding = 0;
    atlasBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    atlasBinding.descriptorCount = 1;
    atlasBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutCreateInfo{};
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCreateInfo.bindingCount = 1;
    layoutCreateInfo.pBindings = &atlasBinding;
    checkResult(
        vkCreateDescriptorSetLayout(device_, &layoutCreateInfo, nullptr, &descriptorSetLayout_),
        "Failed to create font descriptor-set layout"
    );

    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = 1;

    VkDescriptorPoolCreateInfo poolCreateInfo{};
    poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolCreateInfo.maxSets = 1;
    poolCreateInfo.poolSizeCount = 1;
    poolCreateInfo.pPoolSizes = &poolSize;
    checkResult(vkCreateDescriptorPool(device_, &poolCreateInfo, nullptr, &descriptorPool_), "Failed to create font descriptor pool");

    VkDescriptorSetAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.descriptorPool = descriptorPool_;
    allocateInfo.descriptorSetCount = 1;
    allocateInfo.pSetLayouts = &descriptorSetLayout_;
    checkResult(vkAllocateDescriptorSets(device_, &allocateInfo, &descriptorSet_), "Failed to allocate font descriptor set");

    VkDescriptorImageInfo imageInfo{};
    imageInfo.sampler = atlasSampler_;
    imageInfo.imageView = atlasImageView_;
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = descriptorSet_;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(device_, 1, &descriptorWrite, 0, nullptr);
}

void TextOverlay::createPipeline(const Device& device, VkFormat colorAttachmentFormat) {
    pipeline_.emplace(device, colorAttachmentFormat, makeTextPipelineConfig(descriptorSetLayout_));
}

void TextOverlay::destroyVulkanObjects() noexcept {
    pipeline_.reset();

    if (descriptorPool_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device_, descriptorPool_, nullptr);
        descriptorPool_ = VK_NULL_HANDLE;
        descriptorSet_ = VK_NULL_HANDLE;
    }

    if (descriptorSetLayout_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device_, descriptorSetLayout_, nullptr);
        descriptorSetLayout_ = VK_NULL_HANDLE;
    }

    if (atlasSampler_ != VK_NULL_HANDLE) {
        vkDestroySampler(device_, atlasSampler_, nullptr);
        atlasSampler_ = VK_NULL_HANDLE;
    }

    if (atlasImageView_ != VK_NULL_HANDLE) {
        vkDestroyImageView(device_, atlasImageView_, nullptr);
        atlasImageView_ = VK_NULL_HANDLE;
    }
}

}  // namespace ps::vulkan
