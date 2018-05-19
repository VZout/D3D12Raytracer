#pragma once

#include <cstdint>
#include "../rt_structs.hlsl"
#include "../raytracer.ps"

#define NUM_VERTICES 100
#define NUM_INDICES 90
#define INDICES_TYPE std::uint16_t

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

	/*! Initializes the ray tracer (Called by the viewer by `Viewer::SetRayTracer`) */
	virtual void Initialize(Viewer* viewer) = 0;

	/*! Traces a single pixel and saves it to `m_texture`. */
	virtual void TracePixel(Viewer* viewer, std::uint32_t x, std::uint32_t y) = 0;
	/*! Updates the scene geometry.
	 * Can be very expensive depending on the ray tracer implementation.
	 */
	virtual void UpdateGeometry(Viewer* viewer, std::array<Triangle, 1> geometry, bool all_frames = false) = 0;
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
