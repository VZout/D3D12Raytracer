#pragma once

#include <cstdint>
#include "../rt_structs.hlsl"

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

struct Texture;
class Viewer;

class RayTracer
{
public:
	RayTracer();
	virtual ~RayTracer() = default;

	RayTracer(const RayTracer&) = delete;
	RayTracer& operator=(const RayTracer&) = delete;
	RayTracer(RayTracer&&) = delete;
	RayTracer& operator=(RayTracer&&) = delete;

	virtual void Initialize(Viewer* viewer) = 0;

	/*! Traces a single pixel and saves it to `m_texture`. */
	virtual void TracePixel(Viewer* viewer, std::uint32_t x, std::uint32_t y) = 0;
	/*! Updates the scene geometry.
	 * Can be very expensive depending on the ray tracer implementation.
	 */
	virtual void UpdateGeometry() = 0;
	/*! Updates the raytracer's settings.
	 * Should be called as little as possible since it may require uploading data to the GPU
	 */
	virtual void UpdateSettings(Viewer* viewer, RTProperties properties) = 0;

	/*! Returns the ray traced texture */
	Texture* GetTexture() const;

protected:
	bool m_initialized;
	Texture* m_texture;
	RTProperties m_properties;
};

