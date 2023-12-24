#include "vk_mesh.h"

#include <iostream>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf.h>

using namespace tinygltf;

vertex_input_description vertex::get_vertex_input_description()
{
    vertex_input_description description;

    VkVertexInputBindingDescription main_binding = {};
    main_binding.binding = 0;
    main_binding.stride = sizeof(vertex);
    main_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    description.bindings.push_back(main_binding);

    VkVertexInputAttributeDescription pos_attr = {};
    pos_attr.location = 0;
    pos_attr.binding = 0;
    pos_attr.format = VK_FORMAT_R32G32B32_SFLOAT;
    pos_attr.offset = offsetof(vertex, pos);
    description.attributes.push_back(pos_attr);

    VkVertexInputAttributeDescription normal_attr = {};
    normal_attr.location = 1;
    normal_attr.binding = 0;
    normal_attr.format = VK_FORMAT_R32G32B32_SFLOAT;
    normal_attr.offset = offsetof(vertex, normal);
    description.attributes.push_back(normal_attr);

    VkVertexInputAttributeDescription color_attr = {};
    color_attr.location = 2;
    color_attr.binding = 0;
    color_attr.format = VK_FORMAT_R32G32B32_SFLOAT;
    color_attr.offset = offsetof(vertex, texcoord);
    description.attributes.push_back(color_attr);

    return description;
}

bool mesh::load_from_gltf(const char *filename)
{
    TinyGLTF loader;
    Model model;
    std::string err;
    std::string warn;
    bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, filename);

    if (!warn.empty()) {
        std::cerr << "warn: " << warn.c_str() << std::endl;
    }

    if (!err.empty()) {
        std::cerr << "err: " << err.c_str() << std::endl;
    }

    if (!ret) {
        std::cerr << "failed to parse gltf" << std::endl;
        return false;
    }

    /* POSITION */
    for (auto it = model.meshes[0].primitives[0].attributes.cbegin();
         it != model.meshes[0].primitives[0].attributes.cend(); ++it) {
        if (!strcmp(it->first.data(), "POSITION")) {
            auto accessor = model.accessors[it->second];
            auto bufferview = model.bufferViews[accessor.bufferView];
            auto buffer = model.buffers[bufferview.buffer];
            unsigned char *data =
                buffer.data.data() + bufferview.byteOffset + accessor.byteOffset;
            for (uint32_t i = 0; i < accessor.count; ++i) {
                vertex v;
                v.pos = glm::vec3(*(float *)data, *(float *)(data + sizeof(float)),
                                  *(float *)(data + 2 * sizeof(float)));

                vertices.push_back(v);
                data += bufferview.byteStride;
            }
        }
    }

    /* NORMAL */
    for (auto it = model.meshes[0].primitives[0].attributes.cbegin();
         it != model.meshes[0].primitives[0].attributes.cend(); ++it) {
        if (!strcmp(it->first.data(), "NORMAL")) {
            auto accessor = model.accessors[it->second];
            auto bufferview = model.bufferViews[accessor.bufferView];
            auto buffer = model.buffers[bufferview.buffer];
            unsigned char *data =
                buffer.data.data() + bufferview.byteOffset + accessor.byteOffset;
            for (uint32_t i = 0; i < accessor.count; ++i) {
                vertices[i].normal =
                    glm::vec3(*(float *)data, *(float *)(data + sizeof(float)),
                              *(float *)(data + 2 * sizeof(float)));
                data += bufferview.byteStride;
            }
        }
    }

    /* TEXCROOD */
    for (auto it = model.meshes[0].primitives[0].attributes.cbegin();
         it != model.meshes[0].primitives[0].attributes.cend(); ++it) {
        if (!strcmp(it->first.data(), "TEXCOORD_0")) {
            auto accessor = model.accessors[it->second];
            auto bufferview = model.bufferViews[accessor.bufferView];
            auto buffer = model.buffers[bufferview.buffer];
            unsigned char *data =
                buffer.data.data() + bufferview.byteOffset + accessor.byteOffset;
            for (uint32_t i = 0; i < accessor.count; ++i) {
                vertices[i].texcoord =
                    glm::vec2(*(float *)data, *(float *)(data + sizeof(float)));
                data += bufferview.byteStride;
            }
        }
    }

    /* INDICES */
    if (model.meshes[0].primitives[0].indices != -1) {
        auto accessor = model.accessors[model.meshes[0].primitives[0].indices];
        auto bufferview = model.bufferViews[accessor.bufferView];
        auto buffer = model.buffers[bufferview.buffer];
        unsigned char *data =
            buffer.data.data() + bufferview.byteOffset + accessor.byteOffset;
        for (uint32_t i = 0; i < accessor.count; ++i) {
            indices.push_back(*(uint16_t *)data);
            data += bufferview.byteStride + sizeof(uint16_t);
        }
    }

    /* TEXTURE */
    if (model.meshes[0].primitives[0].material != -1) {
        auto material = model.materials[model.meshes[0].primitives[0].material];
        auto base_color_texture = material.pbrMetallicRoughness.baseColorTexture;
        auto texture = model.textures[base_color_texture.index];
        auto img = model.images[texture.source];
        this->texture = img.image;
        texture_buffer.format = VK_FORMAT_R8G8B8A8_SRGB;
    }

    std::cout << filename << " loaded" << std::endl;

    return true;
}
