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
#define int std::int32_t
#define uint std::uint32_t
#define ARRAY(type, name, num) std::array<type, num> name
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
#define ARRAY(type, name, num) type name[num]
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
	float3 padding;
	ARRAY(float, randoms, 500);
};

struct Triangle
{
	float3 a;
	float material_idx;
	float3 b;
	float u;
	float3 c;
	float v;
	float3 normal;
	float padding3;
};

struct Vertex
{
	float3 position;
	int material_idx;
	float3 normal;
	float padding0;
	float2 uv;
	float2 padding1;
};

struct Index
{
	int index;
};

struct Material
{
	float3 color;
	float metal;
	float3 padding;
	float specular;
};

struct BBox
{
	float top;
	float bottom;
	float left;
	float right;
	float front;
	float back;
};

struct BVHNode
{
	BBox bbox;
	int left_first;
	int count;

	int count_triangles;
	int bib_start;
};
//const StructuredBuffer<BVHNode> bvh_nodes : register(t3);

static const float num_indices = 90;
#ifdef GPU
const StructuredBuffer<Vertex> vertices : register(t3);
const ByteAddressBuffer indices : register(t4);
#endif

cbuffer RTMaterials REGISTER_B(1)
{
	ARRAY(Material, materials, 3);
};

#ifndef GPU
#undef int
#undef uint
#undef ARRAY()
#endif