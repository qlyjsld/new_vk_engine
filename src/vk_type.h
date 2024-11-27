#pragma once

#include <functional>
#include <vector>
#include <volk.h>

#include <vk_mem_alloc.h>

#ifndef NDEBUG
#include <iostream>

#define VK_CHECK(x)                                                            \
    do {                                                                       \
        VkResult err = x;                                                      \
        if (err) {                                                             \
            std::cerr << "vulkan error: " << err << std::endl;                 \
            abort();                                                           \
        }                                                                      \
    } while (0)

#else
#define VK_CHECK(x) x
#endif

struct allocated_buffer {
    VkBuffer buffer;
    VmaAllocation allocation;
    VkDeviceSize size;
};

struct allocated_img {
    VkImage img;
    VkExtent3D extent;
    VkFormat format;
    VmaAllocation allocation;
    VkImageView img_view;
};

struct deletion_queue {
public:
    void push_back(std::function<void()> &&f) { fs.push_back(f); }

    void flush()
    {
        for (auto f = fs.rbegin(); f != fs.rend(); f++)
            (*f)();

        fs.clear();
    };

private:
    std::vector<std::function<void()>> fs;
};

inline deletion_queue deletion_queue;
