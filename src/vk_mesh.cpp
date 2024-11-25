#include "vk_mesh.h"

#include <iostream>
#include <vector>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/mat4x4.hpp>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf.h>

#include "vk_boiler.h"
#include "vk_cmd.h"
#include "vk_engine.h"
#include "vk_type.h"

using namespace tinygltf;

struct buffer_view {
    unsigned char *data;
    uint32_t stride;
    uint32_t count;
};

struct texture_view {
    uint32_t width;
    uint32_t height;
    std::vector<unsigned char> img;
};

buffer_view retreive_buffer(Model *model, Primitive *primitive,
                            uint32_t accessor_index = -1,
                            const char *attr = nullptr);

texture_view retreive_texture(Model *model, uint32_t material_index = -1);

std::vector<mesh> load_from_gltf(const char *filename,
                                 std::vector<node> &nodes);

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
                            uint32_t accessor_index, const char *attr)
{
    buffer_view buffer_view;
    Accessor *accessor;
    if (attr != nullptr) {
        auto attribute = primitive->attributes.find(attr);
        accessor = &model->accessors[attribute->second];
    } else
        accessor = &model->accessors[accessor_index];
    auto *bufferview = &model->bufferViews[accessor->bufferView];
    auto *buffer = &model->buffers[bufferview->buffer];
    buffer_view.data =
        buffer->data.data() + bufferview->byteOffset + accessor->byteOffset;

    auto componentType = accessor->componentType;
    switch (componentType) {
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
        buffer_view.stride = sizeof(uint8_t);
        break;
    case TINYGLTF_COMPONENT_TYPE_BYTE:
        buffer_view.stride = sizeof(int8_t);
        break;
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
        buffer_view.stride = sizeof(uint16_t);
        break;
    case TINYGLTF_COMPONENT_TYPE_SHORT:
        buffer_view.stride = sizeof(int16_t);
        break;
    case TINYGLTF_COMPONENT_TYPE_INT:
        buffer_view.stride = sizeof(int32_t);
        break;
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
        buffer_view.stride = sizeof(uint32_t);
        break;
    case TINYGLTF_COMPONENT_TYPE_FLOAT:
        buffer_view.stride = sizeof(float);
        break;
    case TINYGLTF_COMPONENT_TYPE_DOUBLE:
        buffer_view.stride = sizeof(double);
        break;
    default:
        break;
    }

    auto type = accessor->type;
    switch (type) {
    case TINYGLTF_TYPE_SCALAR:
        buffer_view.stride *= 1;
        break;
    case TINYGLTF_TYPE_VEC2:
        buffer_view.stride *= 2;
        break;
    case TINYGLTF_TYPE_VEC3:
        buffer_view.stride *= 3;
        break;
    case TINYGLTF_TYPE_VEC4:
        buffer_view.stride *= 4;
        break;
    case TINYGLTF_TYPE_MAT2:
        buffer_view.stride *= 4;
        break;
    case TINYGLTF_TYPE_MAT3:
        buffer_view.stride *= 9;
        break;
    case TINYGLTF_TYPE_MAT4:
        buffer_view.stride *= 16;
        break;
    }

    if (bufferview->byteStride != 0)
        buffer_view.stride = bufferview->byteStride;

    buffer_view.count = accessor->count;
    return buffer_view;
}

texture_view retreive_texture(Model *model, uint32_t material_index)
{
    texture_view texture_view;
    auto *material = &model->materials[material_index];
    auto *base_color_texture = &material->pbrMetallicRoughness.baseColorTexture;
    if (base_color_texture->index != -1) {
        auto *texture = &model->textures[base_color_texture->index];
        auto *img = &model->images[texture->source];
        texture_view.width = img->width;
        texture_view.height = img->height;
        texture_view.img = img->image;
    }
    return texture_view;
}

std::vector<mesh> load_from_gltf(const char *filename, std::vector<node> &nodes)
{
    TinyGLTF loader;
    Model model;
    std::string err;
    std::string warn;
    std::vector<mesh> meshes;
    uint32_t mesh_base = 0;
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

    for (auto n = model.nodes.cbegin(); n != model.nodes.cend(); ++n) {
        node node;
        node.name = n->name;
        node.mesh_id = mesh_base + n->mesh;
        node.children = n->children;

        glm::mat4 t = glm::translate(glm::mat4(1.f), glm::vec3(0.f));
        glm::mat4 r = glm::translate(glm::mat4(1.f), glm::vec3(0.f));
        glm::mat4 s = glm::scale(t, glm::vec3(1.f));

        if (n->translation.size() != 0)
            t = glm::translate(glm::mat4(1.f),
                               glm::vec3(n->translation[0], n->translation[1],
                                         n->translation[2]));

        if (n->rotation.size() != 0)
            r = glm::toMat4(glm::quat(n->rotation[3], n->rotation[0],
                                      n->rotation[1], n->rotation[2]));

        if (n->scale.size() != 0)
            s = glm::scale(glm::mat4(1.f),
                           glm::vec3(n->scale[0], n->scale[1], n->scale[2]));

        node.transform_mat = s * r * t;

        if (n->matrix.size() != 0) {
            node.transform_mat = glm::mat4(
                n->matrix[0], n->matrix[1], n->matrix[2], n->matrix[3],
                n->matrix[4], n->matrix[5], n->matrix[6], n->matrix[7],
                n->matrix[8], n->matrix[9], n->matrix[10], n->matrix[11],
                n->matrix[12], n->matrix[13], n->matrix[14], n->matrix[15]);
        }

        nodes.push_back(node);
    }

    for (auto m = model.meshes.cbegin(); m != model.meshes.cend(); ++m) {
        mesh mesh;
        auto primitive = m->primitives[0];

        /* POSITION */
        buffer_view pos = retreive_buffer(&model, &primitive, -1, "POSITION");
        mesh.vertices.resize(pos.count);
        unsigned char *data = pos.data;
        for (uint32_t i = 0; i < pos.count; ++i) {
            mesh.vertices[i].pos =
                glm::vec3(*(float *)data, *(float *)(data + sizeof(float)),
                          *(float *)(data + 2 * sizeof(float)));
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
        buffer_view texcrood =
            retreive_buffer(&model, &primitive, -1, "TEXCOORD_0");
        data = texcrood.data;
        for (uint32_t i = 0; i < texcrood.count; ++i) {
            mesh.vertices[i].texcoord =
                glm::vec2(*(float *)data, *(float *)(data + sizeof(float)));
            data += texcrood.stride;
        }

        /* INDEX */
        if (primitive.indices != -1) {
            buffer_view index =
                retreive_buffer(&model, &primitive, primitive.indices);
            data = index.data;
            for (uint32_t i = 0; i < index.count; ++i) {
                mesh.indices.push_back(*(uint16_t *)data);
                data += index.stride;
            }
        }

        // /* TEXTURE */
        if (primitive.material != -1) {
            texture_view texture_view =
                retreive_texture(&model, primitive.material);
            mesh.texture = texture_view.img;
            mesh.texture_buffer.extent.width = texture_view.width;
            mesh.texture_buffer.extent.height = texture_view.height;
            mesh.texture_buffer.format = VK_FORMAT_R8G8B8A8_SRGB;
        }

        meshes.push_back(mesh);
        ++mesh_base;
    }

    std::cout << filename << " loaded" << std::endl;

    return meshes;
}

void vk_engine::load_meshes()
{
    std::vector<mesh> example = load_from_gltf(
        "./assets/glTF-Sample-Assets/Models/Duck/glTF-Binary/Duck.glb", _nodes);

    _meshes.insert(_meshes.end(), example.begin(), example.end());

    create_buffer(_nodes.size() * pad_uniform_buffer_size(sizeof(render_mat)),
                  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                  VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT,
                  &_render_mat_buffer);

    VkDescriptorBufferInfo descriptor_buffer_info = {};
    descriptor_buffer_info.buffer = _render_mat_buffer.buffer;
    descriptor_buffer_info.offset = 0;
    descriptor_buffer_info.range = sizeof(render_mat);

    VkWriteDescriptorSet write_set = vk_boiler::write_descriptor_set(
        &descriptor_buffer_info, _render_mat_set, 0,
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);

    vkUpdateDescriptorSets(_device, 1, &write_set, 0, nullptr);
}

void vk_engine::upload_meshes(mesh *meshes, size_t size)
{
    for (uint32_t i = 0; i < size; ++i) {
        mesh *mesh = &meshes[i];
        allocated_buffer staging_buffer;

        /* create vertex buffer */
        create_buffer(mesh->vertices.size() * sizeof(vertex),
                      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                      VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT,
                      &staging_buffer);

        void *data;
        vmaMapMemory(_allocator, staging_buffer.allocation, &data);
        std::memcpy(data, mesh->vertices.data(),
                    mesh->vertices.size() * sizeof(vertex));
        vmaUnmapMemory(_allocator, staging_buffer.allocation);

        create_buffer(mesh->vertices.size() * sizeof(vertex),
                      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                          VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                      0, &mesh->vertex_buffer);

        deletion_queue.push_back([=]() {
            vmaDestroyBuffer(_allocator, _meshes[i].vertex_buffer.buffer,
                             _meshes[i].vertex_buffer.allocation);
        });

        immediate_submit([=](VkCommandBuffer cbuffer) {
            VkBufferCopy region = {};
            region.size = mesh->vertices.size() * sizeof(vertex);
            vkCmdCopyBuffer(cbuffer, staging_buffer.buffer,
                            mesh->vertex_buffer.buffer, 1, &region);
        });

        vmaDestroyBuffer(_allocator, staging_buffer.buffer,
                         staging_buffer.allocation);

        /* create index buffer */
        create_buffer(mesh->indices.size() * sizeof(uint16_t),
                      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                      VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT,
                      &staging_buffer);

        vmaMapMemory(_allocator, staging_buffer.allocation, &data);
        std::memcpy(data, mesh->indices.data(),
                    mesh->indices.size() * sizeof(uint16_t));
        vmaUnmapMemory(_allocator, staging_buffer.allocation);

        create_buffer(mesh->indices.size() * sizeof(uint16_t),
                      VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                          VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                      0, &mesh->index_buffer);

        immediate_submit([=](VkCommandBuffer cbuffer) {
            VkBufferCopy region = {};
            region.size = mesh->indices.size() * sizeof(uint16_t);
            vkCmdCopyBuffer(cbuffer, staging_buffer.buffer,
                            mesh->index_buffer.buffer, 1, &region);
        });

        vmaDestroyBuffer(_allocator, staging_buffer.buffer,
                         staging_buffer.allocation);
    }
}

void vk_engine::upload_textures(mesh *meshes, size_t size)
{
    for (uint32_t i = 0; i < size; ++i) {
        mesh *mesh = &meshes[i];
        allocated_buffer staging_buffer;

        if (mesh->texture.size() != 0) {
            create_buffer(mesh->texture.size() * sizeof(unsigned char),
                          VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                          VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT,
                          &staging_buffer);

            void *data;
            vmaMapMemory(_allocator, staging_buffer.allocation, &data);
            std::memcpy(data, mesh->texture.data(),
                        mesh->texture.size() * sizeof(unsigned char));
            vmaUnmapMemory(_allocator, staging_buffer.allocation);

            VkExtent3D extent = {};
            extent.width = mesh->texture_buffer.extent.width;
            extent.height = mesh->texture_buffer.extent.height;
            extent.depth = 1;

            create_img(
                mesh->texture_buffer.format, extent, VK_IMAGE_ASPECT_COLOR_BIT,
                VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, 0,
                &mesh->texture_buffer);

            immediate_submit([=](VkCommandBuffer cbuffer) {
                vk_cmd::vk_img_layout_transition(
                    cbuffer, mesh->texture_buffer.img,
                    VK_IMAGE_LAYOUT_UNDEFINED,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, _transfer_index);

                VkBufferImageCopy region = vk_boiler::buffer_img_copy(extent);

                vkCmdCopyBufferToImage(
                    cbuffer, staging_buffer.buffer, mesh->texture_buffer.img,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

                vk_cmd::vk_img_layout_transition(
                    cbuffer, mesh->texture_buffer.img,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, _transfer_index);
            });

            vmaDestroyBuffer(_allocator, staging_buffer.buffer,
                             staging_buffer.allocation);

            VkDescriptorSetAllocateInfo descriptor_set_allocate_info =
                vk_boiler::descriptor_set_allocate_info(_descriptor_pool,
                                                        &_texture_layout);

            VK_CHECK(vkAllocateDescriptorSets(
                _device, &descriptor_set_allocate_info, &mesh->texture_set));

            VkDescriptorImageInfo descriptor_img_info = {};
            descriptor_img_info.sampler = _sampler;
            descriptor_img_info.imageView = mesh->texture_buffer.img_view;
            descriptor_img_info.imageLayout =
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            VkWriteDescriptorSet write_set = vk_boiler::write_descriptor_set(
                &descriptor_img_info, mesh->texture_set, 0,
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

            vkUpdateDescriptorSets(_device, 1, &write_set, 0, nullptr);
        }
    }
}
