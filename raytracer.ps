#ifndef GPU
#pragma once
#endif

#include "rt_structs.hlsl"

#define REFLECTIONS
#define REFLECTION_RECURSION 2

#ifdef GPU
//Texture2D in_texture : register(t0);
SamplerState s0 : register(s0);

struct VS_OUTPUT
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD;
};
#endif

struct Sphere
{
	float3 center;
	float radius;
	float3 color;
	float specular;
	float metal;
};

struct Light
{
	int type;
	float intensity;
	float3 position;
	float3 direction;
};

#ifndef GPU
static float2 canvas_size(600, 600);
constant float viewport_size = 1;
constant float epsilon = 0.08;
static float3 camera_pos(0, 0, -3);
constant float z_near = 1;
#endif

struct Intersection
{
	Sphere closest;
	float closest_t;
};

static const float inf = 9999999;

constant int num_spheres = 4;
static Sphere spheres[num_spheres];

constant int num_lights = 3;
static Light lights[num_lights];

float3 CanvasToViewport(float2 pos)
{
	return float3(pos.x * viewport_size / canvas_size.x,
		pos.y * viewport_size / canvas_size.y,
		z_near);
}

Intersection ClosestIntersection(float3 origin, float3 direction, float min_t, float max_t);
float3 ReflectRay(float3 v1, float3 v2);

Sphere MakeNull()
{
	Sphere null;
	null.radius = -1;

	return null;
}

bool IsNull(Sphere s)
{
	if (s.radius < 0)
	{
		return true;
	}

	return false;
}

#define AMBIENT 0
#define POINT 1

float3 ComputeLighting(float3 pont, float3 N, float3 V, float specular, float max_t)
{
	float intensity = 0;
	float length_n = length(N);
	float length_v = length(V);

	for (int i = 0; i < num_lights; i++)
	{
		Light light = lights[i];
		if (light.type == AMBIENT)
		{
			intensity += light.intensity;
			continue;
		}

		float3 vec_l;
		if (light.type == POINT)
		{
			vec_l = light.position - pont;
		}
		else // Light.DIRECTIONAL
		{
			vec_l = light.position;
		}

		// Shadow check
		Intersection intersect = ClosestIntersection(pont, vec_l, epsilon, max_t);
		if (!IsNull(intersect.closest))
			continue;

		// Diffuse
		float n_dot_l = dot(N, vec_l);
		if (n_dot_l > 0)
		{
			intensity += light.intensity * n_dot_l / (length_n * length(vec_l));
		}

		if (specular != -1)
		{
			float3 vec_r = ReflectRay(vec_l, N);
			float r_dot_v = dot(vec_r, V);
			if (r_dot_v > 0)
			{
				intensity += light.intensity * pow(r_dot_v / (length(vec_r) * length_v), specular);
			}
		}
	}

	return float3(intensity, intensity, intensity);
}

float2 IntersectRaySphere(float3 origin, float3 direction, Sphere sphere)
{
	float3 oc = origin - sphere.center;

	float k1 = dot(direction, direction);
	float k2 = 2 * dot(oc, direction);
	float k3 = dot(oc, oc) - sphere.radius * sphere.radius;

	float discriminant = k2 * k2 - 4.f * k1 * k3;
	if (discriminant < 0)
	{
		return float2(inf, inf);
	}

	float t1 = (-k2 + sqrt(discriminant)) / (2.f * k1);
	float t2 = (-k2 - sqrt(discriminant)) / (2.f * k1);

	return float2(t1, t2);
}

Intersection ClosestIntersection(float3 origin, float3 direction, float min_t, float max_t)
{
	float closest_t = inf;
	Sphere closest_sphere = MakeNull();

	for (int i = 0; i < num_spheres; i++)
	{
		float2 ts = IntersectRaySphere(origin, direction, spheres[i]);
		if (ts[0] < closest_t && ts[0] > min_t && ts[0] < max_t)
		{
			closest_t = ts[0];
			closest_sphere = spheres[i];
		}
		if (ts[1] < closest_t && ts[1] > min_t && ts[1] < max_t)
		{
			closest_t = ts[1];
			closest_sphere = spheres[i];
		}
	}

	Intersection retval;
	retval.closest = closest_sphere;
	retval.closest_t = closest_t;

	return retval;
}

float3 ReflectRay(float3 v1, float3 v2)
{
	return (v2 * ((2.f * dot(v1, v2))) - v1);
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

float3 RecursiveTraceRay(float3 origin, float3 direction, float min_t, float max_t, int depth)
{
#ifdef REFLECTIONS
	float3 reflected_color;
	float3 reflected_ray = direction;

	OutRef refs[REFLECTION_RECURSION];

	while (depth > 0)
	{

		depth--;

		Intersection intersect = ClosestIntersection(origin, reflected_ray, min_t, max_t);
		Sphere closest_sphere = intersect.closest;
		float closest_t = intersect.closest_t;

		if (IsNull(closest_sphere))
		{
			reflected_color = sky_color;

			refs[depth].ref_color = reflected_color;
			refs[depth].ref_ray = reflected_ray;
			refs[depth].ref_point = origin;
			refs[depth].l_color = reflected_color;
			refs[depth].metal = closest_sphere.metal;
			refs[depth].end = true;
			continue;
		}

		float3 P = origin + (reflected_ray * closest_t);
		float3 N = normalize(P - closest_sphere.center);

		float3 view = reflected_ray * -1.f;

		float3 lighting = ComputeLighting(P, N, view, closest_sphere.specular, inf);
		float3 local_color = closest_sphere.color * lighting;

		if (closest_sphere.metal <= 0 || depth <= 0)
		{
			reflected_color = local_color;

			refs[depth].ref_color = reflected_color;
			refs[depth].ref_ray = reflected_ray;
			refs[depth].ref_point = P;
			refs[depth].l_color = local_color;
			refs[depth].metal = closest_sphere.metal;
			refs[depth].end = true;
			continue;
		}

		origin = P;
		reflected_ray = ReflectRay(view, N);

		refs[depth].ref_color = reflected_color;
		refs[depth].ref_ray = reflected_ray;
		refs[depth].ref_point = P;
		refs[depth].l_color = local_color;
		refs[depth].metal = closest_sphere.metal;
		refs[depth].end = false;
	}

	float3 result = refs[REFLECTION_RECURSION - 1].ref_color;
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

float3 TraceRay(float3 origin, float3 direction, float min_t, float max_t, int depth)
{

	Intersection intersect = ClosestIntersection(origin, direction, min_t, max_t);
	Sphere closest_sphere = intersect.closest;
	float closest_t = intersect.closest_t;

	if (IsNull(closest_sphere))
		return sky_color;

	float3 P = origin + (direction * closest_t);
	float3 N = normalize(P - closest_sphere.center);

	float3 view = direction * -1.f;

	float3 lighting = ComputeLighting(P, N, view, closest_sphere.specular, inf);
	float3 local_color = closest_sphere.color * lighting;

	if (closest_sphere.metal <= 0 || depth <= 0)
	{
		return local_color;
	}

	float3 reflected_ray = ReflectRay(view, N);
	float3 result = RecursiveTraceRay(P, reflected_ray, epsilon, inf, depth);

	return (local_color * (1.f - closest_sphere.metal)) + (result * closest_sphere.metal);
}

#ifdef GPU
float4 main(VS_OUTPUT input) : SV_TARGET
#else
float4 PixelTrace(Input input)
#endif
{
	spheres[0].center = float3(0, 0, 5);
	spheres[0].color = float3(1, 0, 0);
	spheres[0].radius = 1;
	spheres[0].specular = 20;
	spheres[0].metal = 0.2;

	spheres[1].center = float3(2, 2, 4);
	spheres[1].color = float3(0, 0, 1);
	spheres[1].radius = 1;
	spheres[1].specular = 20;
	spheres[1].metal = 0.6;

	spheres[2].center = float3(-2, 0, 4);
	spheres[2].color = float3(0, 1, 0);
	spheres[2].radius = 1;
	spheres[2].specular = 20;
	spheres[2].metal = 0.4;

	spheres[3].center = float3(0, -5001, 0);
	spheres[3].color = float3(1, 1, 1);
	spheres[3].radius = 5000;
	spheres[3].specular = 20;
	spheres[3].metal = 0.5;

	lights[0].position = float3(0, 0, 0);
	lights[0].intensity = 0.2;
	lights[0].type = 0;
	lights[1].position = float3(2, 4, 0);
	lights[1].intensity = 0.6;
	lights[1].type = 1;
	lights[2].position = float3(1, 4, 4);
	lights[2].intensity = 0.2;
	lights[2].type = 2;

	float2 pixel_pos = float2(input.pos.x, input.pos.y);

#ifdef GPU
	pixel_pos.x = (input.pos.x - (canvas_size.x / 2));
	pixel_pos.y = (input.pos.y - (canvas_size.y / 2));
	pixel_pos.y *= -1;
#endif

	/*float pixel_size = 0.25;
	float3 dir0 = CanvasToViewport(pixel_pos - float2(-pixel_size, -pixel_size));
	float3 dir1 = CanvasToViewport(pixel_pos - float2(-pixel_size, pixel_size));
	float3 dir2 = CanvasToViewport(pixel_pos - float2(pixel_size, -pixel_size));
	float3 dir3 = CanvasToViewport(pixel_pos - float2(pixel_size, pixel_size));

	float3 color0 = TraceRay(camera_pos, dir0, z_near, inf, REFLECTION_RECURSION);
	float3 color1 = TraceRay(camera_pos, dir1, z_near, inf, REFLECTION_RECURSION);
	float3 color2 = TraceRay(camera_pos, dir2, z_near, inf, REFLECTION_RECURSION);
	float3 color3 = TraceRay(camera_pos, dir3, z_near, inf, REFLECTION_RECURSION);
	float3 color = (color0 + color1 + color2 + color3) / 4;*/
	
	float3 dir = CanvasToViewport(pixel_pos);
	float3 color = TraceRay(camera_pos, dir, z_near, inf, REFLECTION_RECURSION);

#ifdef GPU
	//if (use_cpu)
	//{
		//color.rgb = in_texture.Sample(s0, input.uv);
	//}
#endif

	return float4(color.x, color.y, color.z, 1.f);
}


#ifndef GPU
#undef float3
#undef float2
#undef float4
#undef constant
#undef cbuffer
#undef length(v) {}
#undef dot(a, b) {}
#undef normalize(a) {}

#undef AMBIENT
#undef POINT
#endif
