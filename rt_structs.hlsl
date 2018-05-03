#ifndef GPU
#pragma once
#endif

#ifndef GPU
#include "src/vec.hpp"
#include "src/math_util.hpp"

#include <array>

#define float3 fm::vec3
#define float2 fm::vec2
#define float4 fm::vec4
#define constant static constexpr
#define cbuffer struct
#define length(v) v.Length()
#define normalize(v) fm::Vec<float, 3>::Normalize(v);
#define dot(a, b) a.Dot(b)
#define clamp(a, b, c) fm::clamp(a, b, c)
#define REGISTER_B(i)

#define FUNC inline

using pc_type = float;
struct Pixel
{
	pc_type r;
	pc_type g;
	pc_type b;
	pc_type a;
};

struct Input
{
	float3 pos;
};
#else
#define constant static const
#define REGISTER_B(i) : register(b##i)
#define FUNC 
#endif

// Structs shared with CPU.
cbuffer RTProperties REGISTER_B(0)
{
	float2 canvas_size;
	float viewport_size;
	float epsilon;
	float3 camera_pos;
	float z_near;
	float3 sky_color;
	float gamma;
	float3 floor_color;
	int use_cpu;
	float exposure;
};
