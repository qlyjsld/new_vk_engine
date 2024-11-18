set(SDL_TEST_LIBRARY OFF CACHE INTERNAL "" FORCE)
set(SDL_STATIC ON CACHE INTERNAL "" FORCE)
set(SDL_SHARED OFF CACHE INTERNAL "" FORCE)
set(TINYGLTF_HEADER_ONLY ON CACHE INTERNAL "" FORCE)
set(TINYGLTF_INSTALL OFF CACHE INTERNAL "" FORCE)
set(TINYGLTF_BUILD_LOADER_EXAMPLE OFF CACHE INTERNAL "" FORCE)

add_subdirectory(glm)
add_subdirectory(vk-bootstrap)
add_subdirectory(SDL)
add_subdirectory(vma)
add_subdirectory(tinygltf)
add_subdirectory(volk)

add_library(imgui STATIC)
add_compile_definitions(IMGUI_IMPL_VULKAN_USE_VOLK)

target_sources(imgui PRIVATE
	"${PROJECT_SOURCE_DIR}/vendor/imgui/imgui.cpp"
	"${PROJECT_SOURCE_DIR}/vendor/imgui/imgui_demo.cpp"
	"${PROJECT_SOURCE_DIR}/vendor/imgui/imgui_draw.cpp"
	"${PROJECT_SOURCE_DIR}/vendor/imgui/imgui_widgets.cpp"
	"${PROJECT_SOURCE_DIR}/vendor/imgui/imgui_tables.cpp"
	"${PROJECT_SOURCE_DIR}/vendor/imgui/backends/imgui_impl_sdl3.cpp"
	"${PROJECT_SOURCE_DIR}/vendor/imgui/backends/imgui_impl_vulkan.cpp")

target_link_libraries(imgui volk SDL3::SDL3)

target_include_directories(imgui PRIVATE
	"${PROJECT_SOURCE_DIR}/vendor/imgui"
	"${PROJECT_SOURCE_DIR}/vendor/imgui/backends"
	"${PROJECT_SOURCE_DIR}/vendor/volk")
