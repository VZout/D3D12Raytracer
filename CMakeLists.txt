cmake_minimum_required(VERSION 3.9)

project(ModernDirect3D12)

if (MSVC)
	add_compile_options(/W4)
else()
	add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_definitions(-DENABLE_IMGUI)

SET(D3D12_LIBS d3d12.lib dxgi.lib d3dcompiler.lib)

set(HEADERS
	src/main.cpp
	src/window.hpp
	src/window.cpp
	src/viewer.hpp
	src/viewer.cpp
	src/d3d12_viewer.hpp
	src/d3d12_viewer.cpp
	src/d3dx12.hpp
	src/ray_tracer.hpp
	src/ray_tracer.cpp
	src/d3d12_ray_tracer.hpp
	src/d3d12_ray_tracer.cpp
	src/texture.hpp
	src/vec.hpp
	src/vec.cpp
	src/math_util.hpp
	src/math_util.cpp
	)

set(IMGUI_SOURCES
	src/imgui/imconfig.h
	src/imgui/imgui.cpp
	src/imgui/imgui.h
	src/imgui/imgui_draw.cpp
	src/imgui/imgui_impl_dx12.cpp
	src/imgui/imgui_impl_dx12.h
	src/imgui/imgui_internal.h
	)

file(GLOB PS_SHADERS "${CMAKE_CURRENT_SOURCE_DIR}/../*.vs")
file(GLOB VS_SHADERS "${CMAKE_CURRENT_SOURCE_DIR}/../*.ps")
file(GLOB UTIL_SHADERS "${CMAKE_CURRENT_SOURCE_DIR}/../*.hlsl")
#set_source_files_properties(${PS_SHADERS} PROPERTIES VS_SHADER_TYPE Pixel VS_SHADER_MODEL 5.0 VS_SHADER_ENTRYPOINT main)
#set_source_files_properties(${VS_SHADERS} PROPERTIES VS_SHADER_TYPE Vertex VS_SHADER_MODEL 5.0 VS_SHADER_ENTRYPOINT main)

add_executable(game WIN32 ${HEADERS} ${IMGUI_SOURCES} ${PS_SHADERS} ${VS_SHADERS} ${UTIL_SHADERS})
target_link_libraries(game ${D3D12_LIBS})
set_target_properties(game PROPERTIES CXX_STANDARD 17)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT game)
set_target_properties(game PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/../")
