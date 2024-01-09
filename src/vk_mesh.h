#pragma once

#include <string>
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
    std::vector<vertex> vertices;
    allocated_buffer vertex_buffer;

    std::vector<uint16_t> indices;
    allocated_buffer index_buffer;

    std::vector<unsigned char> texture;
    allocated_img texture_buffer;
};

struct mesh_push_constants {
    glm::vec4 data;
    glm::mat4 render_mat;
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

struct node {
    std::string name;
    uint32_t mesh_id;
    glm::mat4 transform_mat;
    std::vector<int> children;
    // material material;
};

std::vector<mesh> load_from_gltf(const char *filename, std::vector<node> &nodes);