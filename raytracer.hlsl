#ifndef GPU
#pragma once
#endif

#include "structs.hlsl"
#ifdef GPU
#include "intersects.hlsl"
#include "random.hlsl"
#include "util.hlsl"
#endif

//#define REFLECTIONS
//#define USE_BVH
#define REFLECTION_RECURSION 0

#ifdef GPU
Texture2D in_texture : register(t0);
SamplerState s0 : register(s0);

struct VS_OUTPUT
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD;
};
#endif

// ##################################
#ifdef GPU

constant int num_spheres = 1;
static Sphere spheres[num_spheres];

constant int num_lights = 2;
static Light lights[num_lights];

Intersection ClosestIntersection(float3 origin, float3 direction, float min_t, float max_t);
float3 ReflectRay(float3 v1, float3 v2);

#define AMBIENT 0
#define POINT 1

FUNC float3 ComputeLighting(float3 pont, float3 N, float3 V, Material material, float max_t)
{
	const int num_samples = 50;
	float intensity = 0;
	const float length_n = length(N);
	const float length_v = length(V);

	[unroll(num_spheres)]
	for (int i = 0; i < num_spheres; i++)
	{
		//Light light = lights[i];
		Sphere slight = spheres[i];
		/*if (light.type == AMBIENT)
		{
			intensity += light.intensity;
			continue;
		}*/

		for (int p = 0; p < num_samples; p++)
		{
			float3 vec_l;
			//if (light.type == POINT)
			{
				//vec_l = slight.center - pont;
				vec_l = RandomSpherePoint(slight, random(float2(pont.x + pont.z, pont.y + pont.z) * p) % 1, random(float2(N.x + N.z, N.y + N.z) / (p+1)) % 1) - pont;
				max_t = 1.0f;
			}
			/*else // Light.DIRECTIONAL
			{
				vec_l = light.position;
			}*/

			// Shadow check
			const Intersection intersect = ClosestIntersection(pont, vec_l, epsilon, max_t);
			if (intersect.closest_t != inf) { // if triangle found
				continue;
			}

			// Diffuse
			const float n_dot_l = dot(N, vec_l);
			if (n_dot_l > 0)
			{
				intensity += slight.intensity * n_dot_l / (length_n * length(vec_l));
				//intensity += light.intensity * n_dot_l;
			}

			if (material.specular != -1)
			{
				const float3 vec_r = ReflectRay(vec_l, N);
				//const float3 vec_r = mul(2.0*dot(N, vec_l), N) - vec_l;
				const float r_dot_v = dot(vec_r, V);
				if (r_dot_v > 0)
				{
					intensity += slight.intensity * pow(r_dot_v / (length(vec_r) * length_v), material.specular);
				}
			}
		}
	}

	intensity = intensity / num_samples;
	return float3(intensity, intensity, intensity);
}

FUNC Intersection ClosestIntersection(float3 origin, float3 direction, float min_t, float max_t)
{
	float closest_t = inf;
	Triangle closest_triangle;

	int bvh_num_indices = -1;
	int indices_start = -1;
	
#ifdef USE_BVH
	int left_child = -1;

	// Intersect root node.
	if (IntersectBVH(origin, direction, bvh_nodes[0]))
	{
		left_child = bvh_nodes[0].left_first;
	}
	else // no bvh intersections found
	{
		Intersection retval;
		retval.closest = closest_triangle;
		retval.closest_t = closest_t;
		return retval;
	}

	// Recursivily intersect childeren
	while(bvh_num_indices == -1) // while no end node has been found and it didn't completely miss.
	{
		const BVHNode left_node = bvh_nodes[left_child];
		const BVHNode right_node = bvh_nodes[left_child + 1];

		const bool left = IntersectBVH(origin, direction, left_node);
		const bool right = IntersectBVH(origin, direction, right_node);

		if (left)
		{
			if (left_node.num_indices == -1) // if is not a end node
			{
				left_child = left_node.left_first;
			}
			else { // is a end node.
				bvh_num_indices = left_node.num_indices;
				indices_start = left_node.bib_start;
			}
		}
		else if(right)
		{
			if (right_node.num_indices == -1) // if is not a end node
			{
				left_child = right_node.left_first;
			}
			else { // is a end node.
				bvh_num_indices = right_node.num_indices;
				indices_start = right_node.bib_start;
			}
		}
		else {
			// Becasue hlsl is garbage?
			/*closest_triangle.a = float3(0, 0, 0);
			closest_triangle.b = float3(0, 0, 0);
			closest_triangle.c = float3(0, 0, 0);
			closest_triangle.normal = float3(0, 0, 0);
			closest_triangle.material_idx = 0;
			closest_triangle.u = 0;
			closest_triangle.v = 0;
			closest_triangle.padding3 = 0;

			Intersection intersection;
			intersection.closest = closest_triangle;
			intersection.closest_t = closest_t;
			return intersection;*/
			break; // missed
		}

		// hit multiple bvh's. Using both in this case.
		if (left && right)
		{
			if (left_node.bib_start < right_node.bib_start) {
				indices_start = left_node.bib_start;
				bvh_num_indices = right_node.num_indices;
			}
			else
			{
				indices_start = right_node.bib_start;
				bvh_num_indices = left_node.num_indices;
			}
		}
	}
#else
	bvh_num_indices = num_indices;
	indices_start = 0;
#endif

	for (int i = indices_start; i < bvh_num_indices; i += 3)
	{
		const uint3 tri_vertices = Load3x16BitIndices(i*2);
		const Vertex v0 = vertices[tri_vertices.x];
		const Vertex v1 = vertices[tri_vertices.y];
		const Vertex v2 = vertices[tri_vertices.z];

		Triangle tri;
		tri.a = v0.position;
		tri.b = v1.position;
		tri.c = v2.position;
		tri.normal = normalize(v0.normal + v1.normal + v2.normal);
		tri.material_idx = v1.material_idx;
		

		const float2 ts = IntersectRayTriangle(origin, direction, tri);
		if (ts[0] < closest_t && ts[0] > min_t && ts[0] < max_t)
		{
			closest_t = ts[0];
			closest_triangle = tri;
		}
		if (ts[1] < closest_t && ts[1] > min_t && ts[1] < max_t)
		{
			closest_t = ts[1];
			closest_triangle = tri;
		}
	}

	Intersection retval;
	retval.closest = closest_triangle;
	retval.closest_t = closest_t;

	return retval;
}

struct OutRef
{
	float3 ref_color;
	float3 ref_ray;
	float3 ref_point;
	float3 l_color;
	float metal;
	bool end;
};

FUNC float3 RecursiveTraceRay(float3 origin, float3 direction, float min_t, float max_t, int depth)
{
#ifdef REFLECTIONS
	float3 reflected_color;
	float3 reflected_ray = direction;

	OutRef refs[REFLECTION_RECURSION];

	[unroll(REFLECTION_RECURSION)]
	while (depth-- > 0)
	{
		Intersection intersect = ClosestIntersection(origin, reflected_ray, min_t, max_t);
		Triangle closest_triangle = intersect.closest;
		const float closest_t = intersect.closest_t;

		if (intersect.closest_t == inf) // if no triangle found
		{
			reflected_color = sky_color;

			refs[depth].ref_color = reflected_color;
			refs[depth].ref_ray = reflected_ray;
			refs[depth].ref_point = origin;
			refs[depth].l_color = reflected_color;
			refs[depth].end = true;
			continue;
		}

		const float3 P = origin + (reflected_ray * closest_t);
		const float3 N = normalize(closest_triangle.normal);

		const float3 view = reflected_ray * -1.f;

		const Material material = GetMaterial(closest_triangle.material_idx);

		const float3 lighting = ComputeLighting(P, N, view, material, inf);
		const float3 local_color = material.color * lighting;

		if (material.metal <= 0 || depth <= 0)
		{
			reflected_color = local_color;

			refs[depth].ref_color = reflected_color;
			refs[depth].ref_ray = reflected_ray;
			refs[depth].ref_point = P;
			refs[depth].l_color = local_color;
			refs[depth].metal = material.metal;
			refs[depth].end = true;
			continue;
		}

		origin = P;
		reflected_ray = ReflectRay(view, N);

		refs[depth].ref_color = reflected_color;
		refs[depth].ref_ray = reflected_ray;
		refs[depth].ref_point = P;
		refs[depth].l_color = local_color;
		refs[depth].metal = material.metal;
		refs[depth].end = false;
	}

	float3 result = refs[REFLECTION_RECURSION - 1].ref_color;
	[unroll(REFLECTION_RECURSION)]
	for (int i = 0; i < REFLECTION_RECURSION; i++)
	{
		if (!refs[i].end)
		{
			result = (refs[i].l_color * (1.f - refs[i].metal)) + (result * refs[i].metal);
		}
		else
		{
			result = refs[i].ref_color;
		}
	}

	return result;
#else
	return sky_color;
#endif
}

FUNC float3 TraceRay(float3 origin, float3 direction, float min_t, float max_t, int depth)
{
	const Intersection intersect = ClosestIntersection(origin, direction, min_t, max_t);
	const Triangle closest_triangle = intersect.closest;
	const float closest_t = intersect.closest_t;

	if (closest_t == inf) // if no triangle found
		return sky_color;

	const float3 P = origin + (direction * closest_t);
	const float3 N = normalize(closest_triangle.normal);

	const float3 view = direction * -1.f;

	const Material material = GetMaterial(closest_triangle.material_idx);

	const float3 lighting = ComputeLighting(P, N, view, material, inf);
	const float3 local_color = material.color * lighting;

#ifndef REFLECTIONS
	return local_color;
#else
	if (material.metal <= 0 || depth <= 0)
	{
		return local_color;
	}

	const float3 reflected_ray = ReflectRay(view, N);
	const float3 result = RecursiveTraceRay(P, reflected_ray, epsilon, inf, depth);

	return (local_color * (1.f - material.metal)) + (result * material.metal);
#endif
}

float4 main(VS_OUTPUT input) : SV_TARGET
{
	spheres[0].center = float3(0, 2, 1);
	spheres[0].color = float3(1, 0, 0);
	spheres[0].radius = 0.5;
	spheres[0].intensity = 1;

	lights[0].position = float3(0, 0, 3);
	lights[0].intensity = 0.2;
	lights[0].type = 0;
	lights[1].position = float3(0, 2, 0);
	lights[1].intensity = 0.8;
	lights[1].type = 1;

	const float2 pixel_pos = {(input.pos.x - (canvas_size.x / 2)), (input.pos.y - (canvas_size.y / 2)) * -1};
	
	const float3 dir = CanvasToViewport(pixel_pos);
	float3 color = TraceRay(camera_pos, dir, z_near, inf, REFLECTION_RECURSION);

	color = clamp(color * exposure, 0.f, 1.f);
	color = pow(color, 1.f / gamma);

	return float4(color.x, color.y, color.z, 1.f);
}
// ##################################
#endif


#ifndef GPU
#undef float3
#undef float2
#undef float4
#undef constant
#undef cbuffer
#undef length(v) {}
#undef dot(a, b) {}
#undef normalize(a) {}
#undef clamp

#undef AMBIENT
#undef POINT
#undef FUNC
#endif