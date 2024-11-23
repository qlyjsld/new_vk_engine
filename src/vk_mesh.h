#pragma once

#include <string>
#include <vector>
#include <volk.h>

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
    VkDescriptorSet texture_set;
};

struct material {
    VkPipeline pipeline;
    VkPipelineLayout pipeline_layout;
};

struct node {
    std::string name;
    int mesh_id;
    glm::mat4 transform_mat;
    std::vector<int> children;
    // material material;
};

std::vector<mesh> load_from_gltf(const char *filename, std::vector<node> &nodes);