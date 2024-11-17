cmake_minimum_required(VERSION 3.15)
project(shader)

find_program(GLSLC "glslc" REQUIRED)

add_library(shader INTERFACE)
target_include_directories(shader INTERFACE include)

function(add_shader INPUT_FILE OUTPUT_FILE OPTION)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/include/shader/${OUTPUT_FILE}
            COMMAND ${GLSLC} src/${INPUT_FILE} -o include/shader/${OUTPUT_FILE} -mfmt=num --target-env=vulkan1.3 ${OPTION}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/${INPUT_FILE}
    )
    add_custom_target(shader_${OUTPUT_FILE} ALL DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/include/shader/${OUTPUT_FILE})
    add_dependencies(shader shader_${OUTPUT_FILE})
endfunction()

add_shader(.vert .vert.u32 "-O")
add_shader(.frag .frag.u32 "-O")
add_shader(cloud.comp cloud.comp.u32 "-O")
add_shader(cloudtex.comp cloudtex.comp.u32 "-O")
add_shader(perlin.comp perlin.comp.u32 "-O")
add_shader(perlinworley.comp perlinworley.comp.u32 "-O")
add_shader(skybox.comp skybox.comp.u32 "-O")
add_shader(sphere.comp sphere.comp.u32 "-O")
add_shader(vol.comp vol.comp.u32 "-O")
add_shader(weather.comp weather.comp.u32 "-O")
add_shader(worley.comp worley.comp.u32 "-O")