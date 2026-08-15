#include "renderer/vertex.hpp"
#include "renderer/vulkan/device.hpp"
#include "renderer/vulkan/physical_device.hpp"
#include "renderer/vulkan/renderer.hpp"
#include "renderer/vulkan/swapchain.hpp"

#include <cstdint>
#include <cstring>
#include <glm/vec2.hpp>
#include <limits>
#include <stdexcept>
#include <string>

namespace ps::renderer::vulkan {
Renderer::Renderer(const PhysicalDevice& physicalDevice, const Device& device, const Swapchain& swapchain)
    : physicalDevice_{physicalDevice.nativeHandle()},
      device_{device.nativeHandle()},
      graphicsQueue_{device.graphicsQueue()},
      presentQueue_{device.presentQueue()},
      swapchain_{swapchain},
      graphicsPipeline_{device, swapchain},
      commandPool_{physicalDevice, device},
      commandBuffer_{device, commandPool_},
      synchronization_{device, swapchain.images().size()} {
    createVertexBuffer();
    createIndexBuffer();
}

Renderer::~Renderer() {
    if (device_ != VK_NULL_HANDLE) {
        // Member destructors run after this destructor body. Wait first so the
        // GPU no longer uses the pipeline, command buffer or synchronization
        // objects that are about to be destroyed.
        vkDeviceWaitIdle(device_);
    }
    destroyVertexBuffer();
    destroyIndexBuffer();
}

void Renderer::drawFrame(glm::mat4 const& viewProjection) {
    const VkFence inFlightFence = synchronization_.inFlightFence();

    VkResult result = vkWaitForFences(device_, 1, &inFlightFence, VK_TRUE, std::numeric_limits<std::uint64_t>::max());
    if (result != VK_SUCCESS) {
        throw std::runtime_error{std::string{"Failed to wait for Vulkan in-flight fence: VkResult "} + std::to_string(result)};
    }

    std::uint32_t imageIndex = 0;
    result = vkAcquireNextImageKHR(
        device_, swapchain_.nativeHandle(), std::numeric_limits<std::uint64_t>::max(), synchronization_.imageAvailable(), VK_NULL_HANDLE, &imageIndex
    );

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        throw std::runtime_error{"Vulkan swapchain is out of date; swapchain recreation is not implemented yet"};
    }

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error{std::string{"Failed to acquire Vulkan swapchain image: VkResult "} + std::to_string(result)};
    }

    result = vkResetCommandBuffer(commandBuffer_.nativeHandle(), 0);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{std::string{"Failed to reset Vulkan command buffer: VkResult "} + std::to_string(result)};
    }
    recordCommandBuffer(imageIndex, viewProjection);

    const VkSemaphore imageAvailable = synchronization_.imageAvailable();
    const VkSemaphore renderFinished = synchronization_.renderFinished(imageIndex);

    VkSemaphoreSubmitInfo waitSemaphoreInfo{};
    waitSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    waitSemaphoreInfo.semaphore = imageAvailable;
    waitSemaphoreInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkCommandBufferSubmitInfo commandBufferInfo{};
    commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    commandBufferInfo.commandBuffer = commandBuffer_.nativeHandle();

    VkSemaphoreSubmitInfo signalSemaphoreInfo{};
    signalSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    signalSemaphoreInfo.semaphore = renderFinished;
    signalSemaphoreInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

    VkSubmitInfo2 submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    submitInfo.waitSemaphoreInfoCount = 1;
    submitInfo.pWaitSemaphoreInfos = &waitSemaphoreInfo;
    submitInfo.commandBufferInfoCount = 1;
    submitInfo.pCommandBufferInfos = &commandBufferInfo;
    submitInfo.signalSemaphoreInfoCount = 1;
    submitInfo.pSignalSemaphoreInfos = &signalSemaphoreInfo;

    // Reset the fence only once the frame is fully recorded and is about to be
    // submitted. If recording throws, the still-signaled fence remains reusable.
    result = vkResetFences(device_, 1, &inFlightFence);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{std::string{"Failed to reset Vulkan in-flight fence: VkResult "} + std::to_string(result)};
    }

    result = vkQueueSubmit2(graphicsQueue_, 1, &submitInfo, inFlightFence);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{std::string{"Failed to submit Vulkan graphics commands: VkResult "} + std::to_string(result)};
    }

    const VkSwapchainKHR swapchainHandle = swapchain_.nativeHandle();

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderFinished;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchainHandle;
    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(presentQueue_, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        throw std::runtime_error{"Vulkan swapchain became out of date; swapchain recreation is not implemented yet"};
    }

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error{std::string{"Failed to present Vulkan swapchain image: VkResult "} + std::to_string(result)};
    }
}

void Renderer::createVertexBuffer() {
    Vertex vertices[4] = {
        {{-200.0F, -150.0F}, {1.0F, 0.0F, 0.0F}},
        {{200.0F, -150.0F}, {0.0F, 1.0F, 0.0F}},
        {{200.0F, 150.0F}, {0.0F, 0.0F, 1.0F}},
        {{-200.0F, 150.0F}, {1.0F, 1.0F, 0.0F}},
    };

    VkBufferCreateInfo vertexBufferCreateInfo{};
    vertexBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vertexBufferCreateInfo.size = sizeof(vertices);
    vertexBufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    vertexBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateBuffer(device_, &vertexBufferCreateInfo, nullptr, &vertexBuffer_);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{std::string{"Failed to create Vulkan vertex buffer: VkResult "} + std::to_string(result)};
    }

    VkMemoryRequirements memoryRequirements{};
    VkPhysicalDeviceMemoryProperties memoryProperties{};
    vkGetBufferMemoryRequirements(device_, vertexBuffer_, &memoryRequirements);
    vkGetPhysicalDeviceMemoryProperties(physicalDevice_, &memoryProperties);

    std::uint32_t memoryTypeIndex = 0;
    bool foundMemoryType = false;
    for (std::uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i) {
        bool compatibleWithBuffer = (memoryRequirements.memoryTypeBits & (1U << i)) != 0;
        bool isHostVisible = (memoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0;
        bool isHostCoherent = (memoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0;
        if (compatibleWithBuffer && isHostVisible && isHostCoherent) {
            foundMemoryType = true;
            memoryTypeIndex = i;
            break;
        }
    }

    if (!foundMemoryType) {
        throw std::runtime_error{"Failed to find suitable Vulkan memory type for vertex buffer"};
    }

    VkMemoryAllocateInfo memoryAllocateInfo{};
    memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocateInfo.allocationSize = memoryRequirements.size;
    memoryAllocateInfo.memoryTypeIndex = memoryTypeIndex;

    result = vkAllocateMemory(device_, &memoryAllocateInfo, nullptr, &vertexBufferMemory_);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{std::string{"Failed to allocate Vulkan memory for vertex buffer: VkResult "} + std::to_string(result)};
    }

    result = vkBindBufferMemory(device_, vertexBuffer_, vertexBufferMemory_, 0);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{std::string{"Failed to bind Vulkan memory to vertex buffer: VkResult "} + std::to_string(result)};
    }

    void* mappedMemory = nullptr;
    result = vkMapMemory(device_, vertexBufferMemory_, 0, sizeof(vertices), 0, &mappedMemory);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{std::string{"Failed to map Vulkan memory for vertex buffer: VkResult "} + std::to_string(result)};
    }

    std::memcpy(mappedMemory, vertices, sizeof(vertices));
    vkUnmapMemory(device_, vertexBufferMemory_);
}

void Renderer::destroyVertexBuffer() {
    if (vertexBuffer_ != VK_NULL_HANDLE) {
        vkDestroyBuffer(device_, vertexBuffer_, nullptr);
        vertexBuffer_ = VK_NULL_HANDLE;
    }

    if (vertexBufferMemory_ != VK_NULL_HANDLE) {
        vkFreeMemory(device_, vertexBufferMemory_, nullptr);
        vertexBufferMemory_ = VK_NULL_HANDLE;
    }
}

void Renderer::createIndexBuffer() {
    const std::uint16_t indices[6] = {0, 1, 3, /**/ 1, 2, 3};
    VkBufferCreateInfo vertexIndexBufferCreateInfo{};
    vertexIndexBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vertexIndexBufferCreateInfo.size = sizeof(indices);
    vertexIndexBufferCreateInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    vertexIndexBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateBuffer(device_, &vertexIndexBufferCreateInfo, nullptr, &indexBuffer_);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{std::string{"Failed to create Vulkan vertex index buffer: VkResult "} + std::to_string(result)};
    }
    VkMemoryRequirements memoryRequirements{};
    VkPhysicalDeviceMemoryProperties memoryProperties{};
    vkGetBufferMemoryRequirements(device_, indexBuffer_, &memoryRequirements);
    vkGetPhysicalDeviceMemoryProperties(physicalDevice_, &memoryProperties);

    std::uint32_t memoryTypeIndex = 0;
    bool foundMemoryType = false;
    for (std::uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i) {
        bool compatibleWithBuffer = (memoryRequirements.memoryTypeBits & (1U << i)) != 0;
        bool isHostVisible = (memoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0;
        bool isHostCoherent = (memoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0;
        if (compatibleWithBuffer && isHostVisible && isHostCoherent) {
            foundMemoryType = true;
            memoryTypeIndex = i;
            break;
        }
    }

    if (!foundMemoryType) {
        throw std::runtime_error{"Failed to find suitable Vulkan memory type for vertex index buffer"};
    }

    VkMemoryAllocateInfo memoryAllocateInfo{};
    memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocateInfo.allocationSize = memoryRequirements.size;
    memoryAllocateInfo.memoryTypeIndex = memoryTypeIndex;

    result = vkAllocateMemory(device_, &memoryAllocateInfo, nullptr, &indexBufferMemory_);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{std::string{"Failed to allocate Vulkan memory for vertex index buffer: VkResult "} + std::to_string(result)};
    }

    result = vkBindBufferMemory(device_, indexBuffer_, indexBufferMemory_, 0);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{std::string{"Failed to bind Vulkan memory to vertex index buffer: VkResult "} + std::to_string(result)};
    }

    void* mappedMemory = nullptr;
    result = vkMapMemory(device_, indexBufferMemory_, 0, sizeof(indices), 0, &mappedMemory);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{std::string{"Failed to map Vulkan memory for vertex index buffer: VkResult "} + std::to_string(result)};
    }

    std::memcpy(mappedMemory, indices, sizeof(indices));
    vkUnmapMemory(device_, indexBufferMemory_);
}

void Renderer::destroyIndexBuffer() {
    if (indexBuffer_ != VK_NULL_HANDLE) {
        vkDestroyBuffer(device_, indexBuffer_, nullptr);
        indexBuffer_ = VK_NULL_HANDLE;
    }

    if (indexBufferMemory_ != VK_NULL_HANDLE) {
        vkFreeMemory(device_, indexBufferMemory_, nullptr);
        indexBufferMemory_ = VK_NULL_HANDLE;
    }
}

void Renderer::recordCommandBuffer(std::uint32_t imageIndex, glm::mat4 const& viewProjection) {
    const VkCommandBuffer commandBuffer = commandBuffer_.nativeHandle();

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VkResult result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{std::string{"Failed to begin Vulkan command buffer: VkResult "} + std::to_string(result)};
    }

    const VkImage image = swapchain_.images().at(imageIndex);
    const VkImageView imageView = swapchain_.imageViews().at(imageIndex);
    const VkExtent2D extent = swapchain_.extent();

    // We clear every acquired image before drawing, so previous contents do not
    // need to be preserved. Using UNDEFINED here explicitly allows Vulkan to
    // discard them while transitioning to a color-attachment layout.
    VkImageMemoryBarrier2 toColorAttachment{};
    toColorAttachment.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    toColorAttachment.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
    toColorAttachment.srcAccessMask = VK_ACCESS_2_NONE;
    toColorAttachment.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    toColorAttachment.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    toColorAttachment.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    toColorAttachment.newLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
    toColorAttachment.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toColorAttachment.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toColorAttachment.image = image;
    toColorAttachment.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    toColorAttachment.subresourceRange.baseMipLevel = 0;
    toColorAttachment.subresourceRange.levelCount = 1;
    toColorAttachment.subresourceRange.baseArrayLayer = 0;
    toColorAttachment.subresourceRange.layerCount = 1;

    VkDependencyInfo toColorAttachmentDependency{};
    toColorAttachmentDependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    toColorAttachmentDependency.imageMemoryBarrierCount = 1;
    toColorAttachmentDependency.pImageMemoryBarriers = &toColorAttachment;

    vkCmdPipelineBarrier2(commandBuffer, &toColorAttachmentDependency);

    VkClearValue clearValue{};
    clearValue.color.float32[0] = 0.035F;
    clearValue.color.float32[1] = 0.040F;
    clearValue.color.float32[2] = 0.060F;
    clearValue.color.float32[3] = 1.0F;

    VkRenderingAttachmentInfo colorAttachment{};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.imageView = imageView;
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue = clearValue;

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea.offset = {0, 0};
    renderingInfo.renderArea.extent = extent;
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachment;

    VkDeviceSize vertexBufferOffset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer_, &vertexBufferOffset);
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer_, 0, VK_INDEX_TYPE_UINT16);

    /// Rendering start //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    vkCmdBeginRendering(commandBuffer, &renderingInfo);

    VkViewport viewport{};
    viewport.x = 0.0F;
    viewport.y = 0.0F;
    viewport.width = static_cast<float>(extent.width);
    viewport.height = static_cast<float>(extent.height);
    viewport.minDepth = 0.0F;
    viewport.maxDepth = 1.0F;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = extent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline_.nativeHandle());
    vkCmdPushConstants(commandBuffer, graphicsPipeline_.layout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &viewProjection);

    vkCmdDrawIndexed(commandBuffer, 6, 1, 0, 0, 0);
    vkCmdEndRendering(commandBuffer);
    /// Rendering end ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    VkImageMemoryBarrier2 toPresent{};
    toPresent.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    toPresent.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    toPresent.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    toPresent.dstStageMask = VK_PIPELINE_STAGE_2_NONE;
    toPresent.dstAccessMask = VK_ACCESS_2_NONE;
    toPresent.oldLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
    toPresent.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    toPresent.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toPresent.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toPresent.image = image;
    toPresent.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    toPresent.subresourceRange.baseMipLevel = 0;
    toPresent.subresourceRange.levelCount = 1;
    toPresent.subresourceRange.baseArrayLayer = 0;
    toPresent.subresourceRange.layerCount = 1;

    VkDependencyInfo toPresentDependency{};
    toPresentDependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    toPresentDependency.imageMemoryBarrierCount = 1;
    toPresentDependency.pImageMemoryBarriers = &toPresent;

    vkCmdPipelineBarrier2(commandBuffer, &toPresentDependency);

    result = vkEndCommandBuffer(commandBuffer);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{std::string{"Failed to end Vulkan command buffer: VkResult "} + std::to_string(result)};
    }
}

}  // namespace ps::renderer::vulkan
