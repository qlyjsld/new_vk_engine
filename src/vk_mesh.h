#pragma once

#include "vk_type.h"
#include <glm/vec3.hpp>
#include <vector>
#include <vulkan/vulkan_core.h>

struct vertex_input_description {
    std::vector<VkVertexInputBindingDescription> bindings;
    std::vector<VkVertexInputAttributeDescription> attributes;
};

struct vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec3 color;

    static vertex_input_description get_vertex_input_description();
};

struct mesh {
    mesh(const char *filename) { load_from_gltf(filename); }

    std::vector<vertex> vertices;
    std::vector<uint16_t> indices;
    allocated_buffer vertex_buffer;
    allocated_buffer index_buffer;

    bool load_from_gltf(const char *filename);
};
