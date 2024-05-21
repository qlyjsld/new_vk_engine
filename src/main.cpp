#include "vk_engine.h"

#include <cstring>

#include <imgui.h>
// #include <openvdb/openvdb.h>

#include "vk_boiler.h"
#include "vk_cmd.h"
#include "vk_comp.h"
#include "vk_pipeline.h"
#include "vk_type.h"

struct camera_data {
    alignas(16) glm::vec3 pos;
    alignas(16) glm::vec3 dir;
    alignas(16) glm::vec3 up;
    alignas(4) float fov;
};

struct cloud_data {
    alignas(16) glm::vec3 centre;
    alignas(4) float size;
    alignas(4) float height;
    alignas(4) int cloudtex_size;
    alignas(4) int weather_size;
    alignas(4) float type;
    alignas(4) float freq;
    alignas(4) float ambient;
    alignas(4) float sigma_a;
    alignas(4) float sigma_s;
    alignas(4) float step;
    alignas(4) int max_steps;
    alignas(4) float cutoff;
    alignas(4) float density;
    alignas(4) float lambda;
    alignas(4) float temperature;
    alignas(16) glm::vec3 color;
};

static float cloud_size = 256.f;
static uint32_t cloudtex_size = 128;
static uint32_t weather_size = 512;
static bool cloud = true;
static cloud_data cloud_data;

int main(int argc, char *argv[])
{
    vk_engine engine = {};
    engine.init();

    /* openvdb::initialize();
    openvdb::io::File file("./assets/wdas_cloud/wdas_cloud.vdb");
    file.open();
    file.close(); */

    engine.run();
    engine.cleanup();
    return 0;
}

void vk_engine::skybox_init()
{
    comp_allocator allocator(_device, _allocator);

    allocator.create_buffer(pad_uniform_buffer_size(sizeof(camera_data)),
                            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                            VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT, "camera");

    allocator.create_buffer(pad_uniform_buffer_size(sizeof(glm::vec2)),
                            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                            VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT, "extent");

    glm::vec2 extent = glm::vec2{
        _resolution.width,
        _resolution.height,
    };

    allocated_buffer buffer = allocator.get_buffer("extent");

    void *data;
    vmaMapMemory(_allocator, buffer.allocation, &data);
    std::memcpy(data, &extent, sizeof(glm::vec2));
    vmaUnmapMemory(_allocator, buffer.allocation);

    allocator.load_img("target", _target);

    std::vector<descriptor> descriptors = {
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, "target"},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, "extent"},
        // {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, "camera"},
    };

    cs skybox(allocator, descriptors, "../shaders/skybox.comp.spv",
              _min_buffer_alignment);

    PipelineBuilder pb = {};
    pb._shader_stage_infos.push_back(
        vk_boiler::shader_stage_create_info(VK_SHADER_STAGE_COMPUTE_BIT, skybox.module));

    std::vector<VkDescriptorSetLayout> layouts = {
        skybox.layout,
    };

    std::vector<VkPushConstantRange> push_constants = {};

    pb.build_comp(_device, layouts, push_constants, &skybox.pipeline_layout,
                  &skybox.pipeline);

    skybox.draw = [=](VkCommandBuffer cbuffer, cs *cs) {
        vkCmdBindPipeline(cbuffer, VK_PIPELINE_BIND_POINT_COMPUTE, cs->pipeline);

        /* camera_data camera_data;
        camera_data.pos = _vk_camera.get_pos();
        camera_data.dir = _vk_camera.get_dir();
        camera_data.up = _vk_camera.get_up();
        camera_data.fov = _vk_camera.get_fov();

        void *data;
        vmaMapMemory(_allocator, cs->allocator.get_buffer("camera").allocation, &data);
        std::memcpy(data, &camera_data, pad_uniform_buffer_size(sizeof(camera_data)));
        vmaUnmapMemory(cs->allocator.allocator,
                       cs->allocator.get_buffer("camera").allocation); */

        std::vector<uint32_t> doffsets = {
            0,
            // 0,
        };

        vkCmdBindDescriptorSets(cbuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                                cs->pipeline_layout, 0, 1, &cs->set, doffsets.size(),
                                doffsets.data());

        vkCmdDispatch(cbuffer, _resolution.width / 8, _resolution.height / 8, 1);
    };

    // css.push_back(skybox);
}

void vk_engine::cloudtex_init()
{
    /* to init a cs you need an allocator (custom struct) */
    comp_allocator allocator(_device, _allocator);

    allocator.create_img(VK_FORMAT_R16G16B16A16_SFLOAT,
                         VkExtent3D{cloudtex_size, cloudtex_size, cloudtex_size},
                         VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_USAGE_STORAGE_BIT, 0,
                         "cloudtex");

    allocator.create_buffer(pad_uniform_buffer_size(sizeof(float)),
                            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                            VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT, "size");

    allocated_buffer buffer = allocator.get_buffer("size");

    float dummy = (float)cloudtex_size;

    void *data;
    vmaMapMemory(_allocator, buffer.allocation, &data);
    std::memcpy(data, (float *)&dummy, sizeof(float));
    vmaUnmapMemory(_allocator, buffer.allocation);

    /* match set binding */
    std::vector<descriptor> descriptors = {
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, "cloudtex"},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, "extent"},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, "size"},
    };

    cs cloudtex(allocator, descriptors, "../shaders/cloudtex.comp.spv",
                _min_buffer_alignment);

    /* start building pipeline using info from struct cs and comp_allocator */
    PipelineBuilder pb = {};
    pb._shader_stage_infos.push_back(vk_boiler::shader_stage_create_info(
        VK_SHADER_STAGE_COMPUTE_BIT, cloudtex.module));

    VkPushConstantRange u_time = {};
    u_time.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    u_time.offset = 0;
    u_time.size = sizeof(float);

    std::vector<VkDescriptorSetLayout> layouts = {
        cloudtex.layout,
    };

    std::vector<VkPushConstantRange> push_constants = {
        u_time,
    };

    pb.build_comp(_device, layouts, push_constants, &cloudtex.pipeline_layout,
                  &cloudtex.pipeline);

    cloudtex.draw = [=](VkCommandBuffer cbuffer, cs *cs) {
        vk_cmd::vk_img_layout_transition(cbuffer, cs->allocator.get_img("cloudtex").img,
                                         VK_IMAGE_LAYOUT_UNDEFINED,
                                         VK_IMAGE_LAYOUT_GENERAL, _comp_index);

        vkCmdBindPipeline(cbuffer, VK_PIPELINE_BIND_POINT_COMPUTE, cs->pipeline);

        std::vector<uint32_t> doffsets = {
            0,
            0,
        };

        vkCmdBindDescriptorSets(cbuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                                cs->pipeline_layout, 0, 1, &cs->set, doffsets.size(),
                                doffsets.data());

        vkCmdDispatch(cbuffer, cloudtex_size / 8, cloudtex_size / 8, cloudtex_size / 8);
    };

    cs::cc_init(_comp_index, _device);
    cs::comp_immediate_submit(_device, _comp_queue, &cloudtex);
}

void vk_engine::weather_init()
{
    /* to init a cs you need an allocator (custom struct) */
    comp_allocator allocator(_device, _allocator);

    allocator.create_img(VK_FORMAT_R16_SFLOAT, VkExtent3D{weather_size, weather_size, 1},
                         VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_USAGE_STORAGE_BIT, 0,
                         "weather");

    /* match set binding */
    std::vector<descriptor> descriptors = {
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, "weather"},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, "extent"},
    };

    cs weather(allocator, descriptors, "../shaders/weather.comp.spv",
               _min_buffer_alignment);

    /* start building pipeline using info from struct cs and comp_allocator */
    PipelineBuilder pb = {};
    pb._shader_stage_infos.push_back(
        vk_boiler::shader_stage_create_info(VK_SHADER_STAGE_COMPUTE_BIT, weather.module));

    VkPushConstantRange u_time = {};
    u_time.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    u_time.offset = 0;
    u_time.size = sizeof(float);

    std::vector<VkDescriptorSetLayout> layouts = {
        weather.layout,
    };

    std::vector<VkPushConstantRange> push_constants = {
        u_time,
    };

    pb.build_comp(_device, layouts, push_constants, &weather.pipeline_layout,
                  &weather.pipeline);

    weather.draw = [=](VkCommandBuffer cbuffer, cs *cs) {
        vk_cmd::vk_img_layout_transition(cbuffer, cs->allocator.get_img("weather").img,
                                         VK_IMAGE_LAYOUT_UNDEFINED,
                                         VK_IMAGE_LAYOUT_GENERAL, _comp_index);

        vkCmdBindPipeline(cbuffer, VK_PIPELINE_BIND_POINT_COMPUTE, cs->pipeline);

        uint32_t doffset = 0;
        vkCmdBindDescriptorSets(cbuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                                cs->pipeline_layout, 0, 1, &cs->set, 1, &doffset);

        float u_time = 1.f;
        vkCmdPushConstants(cbuffer, cs->pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0,
                           sizeof(float), &u_time);

        vkCmdDispatch(cbuffer, weather_size / 8, weather_size / 8, 1);
    };

    cs::cc_init(_comp_index, _device);
    cs::comp_immediate_submit(_device, _comp_queue, &weather);
}

void vk_engine::sphere_init()
{
    comp_allocator allocator(_device, _allocator);

    std::vector<descriptor> descriptors = {
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, "target"},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, "extent"},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, "camera"},
    };

    cs sphere(allocator, descriptors, "../shaders/sphere.comp.spv",
              _min_buffer_alignment);

    PipelineBuilder pb = {};
    pb._shader_stage_infos.push_back(
        vk_boiler::shader_stage_create_info(VK_SHADER_STAGE_COMPUTE_BIT, sphere.module));

    std::vector<VkDescriptorSetLayout> layouts = {
        sphere.layout,
    };

    std::vector<VkPushConstantRange> push_constants = {};

    pb.build_comp(_device, layouts, push_constants, &sphere.pipeline_layout,
                  &sphere.pipeline);

    sphere.draw = [=](VkCommandBuffer cbuffer, cs *cs) {
        vkCmdBindPipeline(cbuffer, VK_PIPELINE_BIND_POINT_COMPUTE, cs->pipeline);

        std::vector<uint32_t> doffsets = {
            0,
            0,
        };

        vkCmdBindDescriptorSets(cbuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                                cs->pipeline_layout, 0, 1, &cs->set, doffsets.size(),
                                doffsets.data());

        vkCmdDispatch(cbuffer, _resolution.width / 8, _resolution.height / 8, 1);
    };

    // css.push_back(sphere);
}

void vk_engine::cloud_init()
{
    comp_allocator allocator(_device, _allocator);

    allocator.create_buffer(pad_uniform_buffer_size(sizeof(cloud_data)),
                            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                            VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT, "cloud");

    cloud_data.centre = glm::vec3(0.f, 400.f, 0.f);
    cloud_data.size = cloud_size;
    cloud_data.height = 128.f;
    cloud_data.cloudtex_size = cloudtex_size;
    cloud_data.weather_size = weather_size;
    cloud_data.type = .6f;
    cloud_data.freq = 1.f;
    cloud_data.ambient = 1.f;
    cloud_data.sigma_a = 0.f;
    cloud_data.sigma_s = .39f;
    cloud_data.step = .39f;
    cloud_data.max_steps = 128;
    cloud_data.cutoff = .289f;
    cloud_data.density = 1.f;
    cloud_data.lambda = 600.f;
    cloud_data.temperature = 3000.f;
    cloud_data.color = glm::vec3(1.f);

    std::vector<descriptor> descriptors = {
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, "target"},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, "cloudtex"},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, "weather"},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, "extent"},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, "camera"},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, "cloud"},
    };

    cs cloud(allocator, descriptors, "../shaders/cloud.comp.spv", _min_buffer_alignment);

    PipelineBuilder pb = {};
    pb._shader_stage_infos.push_back(
        vk_boiler::shader_stage_create_info(VK_SHADER_STAGE_COMPUTE_BIT, cloud.module));

    std::vector<VkDescriptorSetLayout> layouts = {
        cloud.layout,
    };

    std::vector<VkPushConstantRange> push_constants = {};

    pb.build_comp(_device, layouts, push_constants, &cloud.pipeline_layout,
                  &cloud.pipeline);

    cloud.draw = [=](VkCommandBuffer cbuffer, cs *cs) {
        vkCmdBindPipeline(cbuffer, VK_PIPELINE_BIND_POINT_COMPUTE, cs->pipeline);

        camera_data camera_data;
        camera_data.pos = _vk_camera.get_pos();
        camera_data.dir = _vk_camera.get_dir();
        camera_data.up = _vk_camera.get_up();
        camera_data.fov = _vk_camera.get_fov();

        void *data;
        vmaMapMemory(_allocator, cs->allocator.get_buffer("camera").allocation, &data);
        std::memcpy(data, &camera_data, pad_uniform_buffer_size(sizeof(camera_data)));
        vmaUnmapMemory(cs->allocator.allocator,
                       cs->allocator.get_buffer("camera").allocation);

        vmaMapMemory(_allocator, cs->allocator.get_buffer("cloud").allocation, &data);
        std::memcpy(data, &cloud_data, pad_uniform_buffer_size(sizeof(cloud_data)));
        vmaUnmapMemory(cs->allocator.allocator,
                       cs->allocator.get_buffer("cloud").allocation);

        std::vector<uint32_t> doffsets = {
            0,
            0,
            0,
        };

        vkCmdBindDescriptorSets(cbuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                                cs->pipeline_layout, 0, 1, &cs->set, doffsets.size(),
                                doffsets.data());

        vkCmdDispatch(cbuffer, _resolution.width / 8, _resolution.height / 8, 1);
    };

    css.push_back(cloud);
}

void vk_engine::draw_comp(frame *frame)
{
    ImGui::SetNextWindowSize(ImVec2{300, 290});
    ImGui::SetNextWindowPos(ImVec2{30, 30});
    ImGui::Begin("cloud", &cloud, 0);
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::SliderFloat("size", &cloud_data.size, 0.f, 6553.f);
    ImGui::SliderFloat("height", &cloud_data.height, 0.f, 650.f);
    ImGui::SliderFloat("type", &cloud_data.type, 0.f, 1.f);
    ImGui::SliderFloat("freq", &cloud_data.freq, 0.f, 3.f);
    ImGui::SliderFloat("ambient", &cloud_data.ambient, 0.f, 32.f);
    ImGui::SliderFloat("sigma_a", &cloud_data.sigma_a, 0.f, 1.f);
    ImGui::SliderFloat("sigma_s", &cloud_data.sigma_s, 0.f, 1.f);
    ImGui::SliderFloat("step", &cloud_data.step, .1f, 3.f);
    ImGui::SliderInt("max_steps", &cloud_data.max_steps, 0, 1024);
    ImGui::SliderFloat("cutoff", &cloud_data.cutoff, 0.f, 1.f);
    ImGui::SliderFloat("density", &cloud_data.density, 0.f, 32.f);
    // ImGui::SliderFloat("lambda", &cloud_data.lambda, 0.f, 1000.f);
    // ImGui::SliderFloat("temperature", &cloud_data.temperature, 0.f, 10000.f);
    // ImGui::ColorEdit3("color", (float *)&cloud_data.color);
    ImGui::End();

    vk_cmd::vk_img_layout_transition(frame->cbuffer, _target.img,
                                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                     VK_IMAGE_LAYOUT_GENERAL, _comp_index);

    for (cs &cs : css)
        cs.draw(frame->cbuffer, &cs);

    vk_cmd::vk_img_layout_transition(frame->cbuffer, _target.img, VK_IMAGE_LAYOUT_GENERAL,
                                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                     _comp_index);
}
