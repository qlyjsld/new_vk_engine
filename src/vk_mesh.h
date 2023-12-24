#pragma once

#include <vector>
#include <vulkan/vulkan_core.h>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include "vk_type.h"

struct vertex_input_description {
    std::vector<VkVertexInputBindingDescription> bindings;
    std::vector<VkVertexInputAttributeDescription> attributes;
};

struct vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 texcoord;

    static vertex_input_description get_vertex_input_description();
};

struct mesh {
    mesh(const char *filename) { load_from_gltf(filename); }

    std::vector<vertex> vertices;
    std::vector<uint16_t> indices;
    allocated_buffer vertex_buffer;
    allocated_buffer index_buffer;

    std::vector<unsigned char> texture;
    allocated_img texture_buffer;

    bool load_from_gltf(const char *filename);
};

struct mesh_push_constants {
    glm::vec4 data;
    glm::mat4 render_matrix;
};

struct render_mat {
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 model;
};

struct material {
    VkPipeline pipeline;
    VkPipelineLayout pipeline_layout;
};