cmake_minimum_required (VERSION 3.22.1)
project (vk_engine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(vendor)
add_subdirectory(shader)

add_executable(vk_engine
    src/main.cpp
    src/vk_boiler.cpp
    src/vk_cmd.cpp
    src/vk_comp.cpp
    src/vk_engine.cpp
    src/vk_init.cpp
    src/vk_mesh.cpp
    src/vk_pipeline.cpp
    src/vk_util.cpp
)

target_link_libraries(vk_engine PRIVATE 
  volk 
  SDL3::SDL3 
  vk-bootstrap Vulkan::Headers
  GPUOpen::VulkanMemoryAllocator 
  tinygltf 
  imgui
  shader) #OpenVDB::openvdb

target_include_directories(vk_engine PRIVATE
	"${PROJECT_SOURCE_DIR}/vendor/imgui"
	"${PROJECT_SOURCE_DIR}/vendor/imgui/backends"
	"${PROJECT_SOURCE_DIR}/vendor/volk")
