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
#define cross(a, b) a.Cross(b)
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
	ARRAY(float3, bbox, 2);
	float left_first;
	float count;

	float num_indices;
	float bib_start;
	float2 padding;
};

static const float inf = 9999999;
static const float PI = 3.14159265f;
static const float num_indices = 90;

#ifndef GPU // properties
static float2 canvas_size(600, 600);
static float viewport_size = 1;
static float epsilon = 0.08;
static float3 camera_pos(0, 0, -3);
static float z_near = 1;
static float3 sky_color(0, 0, 0);
static float3 floor_color(1, 1, 1);
static float gamma = 1;
static float exposure = 1;
#endif

#ifdef GPU
const StructuredBuffer<BVHNode> bvh_nodes : register(t5);
const StructuredBuffer<Vertex> vertices : register(t3);
const ByteAddressBuffer indices : register(t4);
#endif

cbuffer RTMaterials REGISTER_B(1)
{
	ARRAY(Material, materials, 3);
};

struct Sphere
{
	float3 center;
	float radius;
	float3 color;
	float intensity;
};

struct Light
{
	int type;
	float intensity;
	float3 position;
	float3 direction;
};

#ifndef GPU
#undef int
#undef uint
#undef ARRAY()
#endif