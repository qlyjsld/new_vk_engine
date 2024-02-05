#include "vk_engine.h"

#include <cstring>
#include <iostream>

#include "vk_cmd.h"
#include "vk_init.h"
#include "vk_type.h"

void vk_engine::load_meshes()
{
    std::vector<mesh> example = load_from_gltf("../assets/glTF-Sample-Assets/Models/"
                                               "Duck/glTF-Binary/Duck.glb",
                                               _nodes);

    _meshes.insert(_meshes.end(), example.begin(), example.end());

    _node_data_buffer = create_buffer(
        _nodes.size() * pad_uniform_buffer_size(sizeof(render_mat)),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT);

    _deletion_queue.push_back([=]() {
        vmaDestroyBuffer(_allocator, _node_data_buffer.buffer,
                         _node_data_buffer.allocation);
    });

    VkDescriptorBufferInfo desc_buffer_info = {};
    desc_buffer_info.buffer = _node_data_buffer.buffer;
    desc_buffer_info.offset = 0;
    desc_buffer_info.range = sizeof(render_mat);

    VkWriteDescriptorSet write_set = vk_init::vk_create_write_descriptor_set(
        &desc_buffer_info, _node_data_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);

    vkUpdateDescriptorSets(_device, 1, &write_set, 0, nullptr);
}

void vk_engine::upload_meshes(mesh *meshes, size_t size)
{
    for (uint32_t i = 0; i < size; ++i) {
        mesh *mesh = &meshes[i];
        allocated_buffer staging_buffer;

        /* create vertex buffer */
        staging_buffer = create_buffer(mesh->vertices.size() * sizeof(vertex),
                                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                       VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT);

        void *data;
        vmaMapMemory(_allocator, staging_buffer.allocation, &data);
        std::memcpy(data, mesh->vertices.data(), mesh->vertices.size() * sizeof(vertex));
        vmaUnmapMemory(_allocator, staging_buffer.allocation);

        mesh->vertex_buffer = create_buffer(
            mesh->vertices.size() * sizeof(vertex),
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 0);

        _deletion_queue.push_back([=]() {
            vmaDestroyBuffer(_allocator, _meshes[i].vertex_buffer.buffer,
                             _meshes[i].vertex_buffer.allocation);
        });

        immediate_submit([=](VkCommandBuffer cmd_buffer) {
            VkBufferCopy region = {};
            region.size = mesh->vertices.size() * sizeof(vertex);
            vkCmdCopyBuffer(cmd_buffer, staging_buffer.buffer, mesh->vertex_buffer.buffer,
                            1, &region);
        });

        vmaDestroyBuffer(_allocator, staging_buffer.buffer, staging_buffer.allocation);

        /* create index buffer */
        staging_buffer = create_buffer(mesh->indices.size() * sizeof(uint16_t),
                                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                       VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT);

        vmaMapMemory(_allocator, staging_buffer.allocation, &data);
        std::memcpy(data, mesh->indices.data(), mesh->indices.size() * sizeof(uint16_t));
        vmaUnmapMemory(_allocator, staging_buffer.allocation);

        mesh->index_buffer = create_buffer(
            mesh->indices.size() * sizeof(uint16_t),
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 0);

        _deletion_queue.push_back([=]() {
            vmaDestroyBuffer(_allocator, _meshes[i].index_buffer.buffer,
                             _meshes[i].index_buffer.allocation);
        });

        immediate_submit([=](VkCommandBuffer cmd_buffer) {
            VkBufferCopy region = {};
            region.size = mesh->indices.size() * sizeof(uint16_t);
            vkCmdCopyBuffer(cmd_buffer, staging_buffer.buffer, mesh->index_buffer.buffer,
                            1, &region);
        });

        vmaDestroyBuffer(_allocator, staging_buffer.buffer, staging_buffer.allocation);
    }
}

void vk_engine::upload_textures(mesh *meshes, size_t size)
{
    for (uint32_t i = 0; i < size; ++i) {
        mesh *mesh = &meshes[i];
        allocated_buffer staging_buffer;

        if (mesh->texture.size() != 0) {
            staging_buffer = create_buffer(mesh->texture.size() * sizeof(unsigned char),
                                           VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                           VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT);

            void *data;
            vmaMapMemory(_allocator, staging_buffer.allocation, &data);
            std::memcpy(data, mesh->texture.data(),
                        mesh->texture.size() * sizeof(unsigned char));
            vmaUnmapMemory(_allocator, staging_buffer.allocation);

            VkExtent3D extent = {};
            extent.width = mesh->texture_buffer.width;
            extent.height = mesh->texture_buffer.height;
            extent.depth = 1;

            mesh->texture_buffer = create_img(
                mesh->texture_buffer.format, extent, VK_IMAGE_ASPECT_COLOR_BIT,
                VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, 0);

            _deletion_queue.push_back([=]() {
                vkDestroyImageView(_device, _meshes[i].texture_buffer.img_view, nullptr);
                vmaDestroyImage(_allocator, _meshes[i].texture_buffer.img,
                                _meshes[i].texture_buffer.allocation);
            });

            immediate_submit([=](VkCommandBuffer cmd_buffer) {
                vk_cmd::vk_img_layout_transition(
                    cmd_buffer, mesh->texture_buffer.img, VK_IMAGE_LAYOUT_UNDEFINED,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, _transfer_queue_family_index);

                VkBufferImageCopy region = vk_init::vk_create_buffer_image_copy(extent);

                vkCmdCopyBufferToImage(cmd_buffer, staging_buffer.buffer,
                                       mesh->texture_buffer.img,
                                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

                vk_cmd::vk_img_layout_transition(cmd_buffer, mesh->texture_buffer.img,
                                                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                                 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                 _transfer_queue_family_index);
            });

            vmaDestroyBuffer(_allocator, staging_buffer.buffer,
                             staging_buffer.allocation);

            VkSamplerCreateInfo sampler_info = vk_init::vk_create_sampler_info();

            VkSampler sampler;

            VK_CHECK(vkCreateSampler(_device, &sampler_info, nullptr, &sampler));

            _deletion_queue.push_back(
                [=]() { vkDestroySampler(_device, sampler, nullptr); });

            VkDescriptorSetAllocateInfo desc_set_allocate_info =
                vk_init::vk_allocate_descriptor_set_info(_desc_pool, &_texture_layout);

            VK_CHECK(vkAllocateDescriptorSets(_device, &desc_set_allocate_info,
                                              &mesh->desc_set));

            VkDescriptorImageInfo desc_img_info = {};
            desc_img_info.sampler = sampler;
            desc_img_info.imageView = mesh->texture_buffer.img_view;
            desc_img_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            VkWriteDescriptorSet write_set = vk_init::vk_create_write_descriptor_set(
                &desc_img_info, mesh->desc_set,
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

            vkUpdateDescriptorSets(_device, 1, &write_set, 0, nullptr);
        }
    }
}
