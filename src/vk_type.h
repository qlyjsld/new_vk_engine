#pragma once

#include <vulkan/vulkan.h>

#include <vk_mem_alloc.h>

#define VK_CHECK(x)                                                                      \
    do {                                                                                 \
        VkResult err = x;                                                                \
        if (err) {                                                                       \
            std::cerr << "vulkan error: " << err << std::endl;                           \
            abort();                                                                     \
        }                                                                                \
    } while (0)

struct allocated_buffer {
    VkBuffer buffer;
    VmaAllocation allocation;
};

struct allocated_img {
    uint32_t width;
    uint32_t height;
    VkImage img;
    VmaAllocation allocation;
    VkImageView img_view;
    VkFormat format;
};
