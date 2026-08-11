#include "renderer/vulkan/device.hpp"
#include "renderer/vulkan/frame_synchronization.hpp"

#include <cstddef>
#include <stdexcept>
#include <string>

namespace ps::renderer::vulkan {
namespace {

[[nodiscard("The created Vulkan semaphore must be used")]]
VkSemaphore createSemaphore(VkDevice device) {
    VkSemaphoreCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkSemaphore semaphore = VK_NULL_HANDLE;
    const VkResult result = vkCreateSemaphore(device, &createInfo, nullptr, &semaphore);
    if (result != VK_SUCCESS) {
        throw std::runtime_error{std::string{"Failed to create Vulkan semaphore: VkResult "} + std::to_string(result)};
    }

    return semaphore;
}

}  // namespace

FrameSynchronization::FrameSynchronization(const Device& device, std::size_t swapchainImageCount) : device_{device.nativeHandle()} {
    if (swapchainImageCount == 0) {
        throw std::runtime_error{"Cannot create Vulkan frame synchronization without swapchain images"};
    }

    try {
        imageAvailable_ = createSemaphore(device_);

        renderFinished_.reserve(swapchainImageCount);
        for (std::size_t index = 0; index < swapchainImageCount; ++index) {
            renderFinished_.push_back(createSemaphore(device_));
        }

        VkFenceCreateInfo fenceCreateInfo{};
        fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        const VkResult result = vkCreateFence(device_, &fenceCreateInfo, nullptr, &inFlightFence_);
        if (result != VK_SUCCESS) {
            throw std::runtime_error{std::string{"Failed to create Vulkan in-flight fence: VkResult "} + std::to_string(result)};
        }
    } catch (...) {
        if (inFlightFence_ != VK_NULL_HANDLE) {
            vkDestroyFence(device_, inFlightFence_, nullptr);
            inFlightFence_ = VK_NULL_HANDLE;
        }

        for (const VkSemaphore semaphore : renderFinished_) {
            vkDestroySemaphore(device_, semaphore, nullptr);
        }
        renderFinished_.clear();

        if (imageAvailable_ != VK_NULL_HANDLE) {
            vkDestroySemaphore(device_, imageAvailable_, nullptr);
            imageAvailable_ = VK_NULL_HANDLE;
        }

        throw;
    }
}

FrameSynchronization::~FrameSynchronization() {
    if (inFlightFence_ != VK_NULL_HANDLE) {
        vkDestroyFence(device_, inFlightFence_, nullptr);
    }

    for (const VkSemaphore semaphore : renderFinished_) {
        vkDestroySemaphore(device_, semaphore, nullptr);
    }

    if (imageAvailable_ != VK_NULL_HANDLE) {
        vkDestroySemaphore(device_, imageAvailable_, nullptr);
    }
}

VkSemaphore FrameSynchronization::imageAvailable() const noexcept {
    return imageAvailable_;
}

VkSemaphore FrameSynchronization::renderFinished(std::size_t imageIndex) const {
    return renderFinished_.at(imageIndex);
}

VkFence FrameSynchronization::inFlightFence() const noexcept {
    return inFlightFence_;
}

}  // namespace ps::renderer::vulkan
