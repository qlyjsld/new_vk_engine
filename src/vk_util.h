#pragma once
#include <iostream>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

#define VK_CHECK(x)                                                                      \
    do {                                                                                 \
        VkResult err = x;                                                                \
        if (err) {                                                                       \
            std::cerr << "vulkan error: " << err << std::endl;                           \
            abort();                                                                     \
        }                                                                                \
    } while (0)
