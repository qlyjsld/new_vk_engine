#include "vk_mesh.h"

#include <glm/ext/matrix_transform.hpp>
#include <iostream>
#include <vector>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf.h>

using namespace tinygltf;

struct buffer_view {
    unsigned char *data;
    uint32_t stride;
    uint32_t count;
};

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

buffer_view retreive_buffer(Model *model, Primitive *primitive,
                            uint32_t accessor_index = -1, const char *attr = nullptr)
{
    buffer_view buffer_view;
    Accessor accessor;
    if (attr != nullptr) {
        auto attribute = primitive->attributes.find(attr);
        accessor = model->accessors[attribute->second];
    } else
        accessor = model->accessors[accessor_index];
    auto bufferview = model->bufferViews[accessor.bufferView];
    auto buffer = model->buffers[bufferview.buffer];
    buffer_view.data = buffer.data.data() + bufferview.byteOffset + accessor.byteOffset;
    buffer_view.stride = bufferview.byteStride;
    buffer_view.count = accessor.count;
    return buffer_view;
}

std::vector<mesh> load_from_gltf(const char *filename, std::vector<node> &nodes)
{
    TinyGLTF loader;
    Model model;
    std::string err;
    std::string warn;
    std::vector<mesh> meshes;
    bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, filename);

    if (!warn.empty()) {
        std::cerr << "warn: " << warn.c_str() << std::endl;
    }

    if (!err.empty()) {
        std::cerr << "err: " << err.c_str() << std::endl;
    }

    if (!ret) {
        std::cerr << "failed to parse gltf" << std::endl;
        return meshes;
    }

    for (auto m = model.meshes.cbegin(); m != model.meshes.cend(); ++m) {
        mesh mesh;
        auto primitive = m->primitives[0];

        /* POSITION */
        buffer_view pos = retreive_buffer(&model, &primitive, -1, "POSITION");
        unsigned char *data = pos.data;
        for (uint32_t i = 0; i < pos.count; ++i) {
            vertex v;
            v.pos = glm::vec3(*(float *)data, *(float *)(data + sizeof(float)),
                              *(float *)(data + 2 * sizeof(float)));

            mesh.vertices.push_back(v);
            data += pos.stride;
        }

        /* NORMAL */
        buffer_view normal = retreive_buffer(&model, &primitive, -1, "NORMAL");
        data = normal.data;
        for (uint32_t i = 0; i < normal.count; ++i) {
            mesh.vertices[i].normal =
                glm::vec3(*(float *)data, *(float *)(data + sizeof(float)),
                          *(float *)(data + 2 * sizeof(float)));
            data += normal.stride;
        }

        /* TEXCROOD */
        buffer_view texcrood = retreive_buffer(&model, &primitive, -1, "TEXCOORD_0");
        data = texcrood.data;
        for (uint32_t i = 0; i < texcrood.count; ++i) {
            mesh.vertices[i].texcoord =
                glm::vec2(*(float *)data, *(float *)(data + sizeof(float)));
            data += texcrood.stride;
        }

        /* INDEX */
        if (primitive.indices != -1) {
            buffer_view index = retreive_buffer(&model, &primitive, primitive.indices);
            data = index.data;

            for (uint32_t i = 0; i < index.count; ++i) {
                mesh.indices.push_back(*(uint16_t *)data);
                data += index.stride + sizeof(uint16_t);
            }
        }

        // /* TEXTURE */
        if (primitive.material != -1) {
            auto material = model.materials[primitive.material];
            auto base_color_texture = material.pbrMetallicRoughness.baseColorTexture;
            auto texture = model.textures[base_color_texture.index];
            auto img = model.images[texture.source];
            mesh.texture = img.image;
            mesh.texture_buffer.format = VK_FORMAT_R8G8B8A8_SRGB;
        }

        meshes.push_back(mesh);
    }

    std::cout << filename << " loaded" << std::endl;

    return meshes;
}
