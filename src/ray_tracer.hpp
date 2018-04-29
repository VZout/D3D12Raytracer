#pragma once

#include <cstdint>

struct Texture;

class RayTracer
{
public:
	RayTracer();
	virtual ~RayTracer() = default;

	RayTracer(const RayTracer&) = delete;
	RayTracer& operator=(const RayTracer&) = delete;
	RayTracer(RayTracer&&) = delete;
	RayTracer& operator=(RayTracer&&) = delete;

	/*! Traces a single pixel and saves it to `m_texture`. */
	virtual void TracePixel(std::uint32_t x, std::uint32_t y) = 0;
	/*! Updates the scene geometry.
	 * Can be very expensive depending on the ray tracer implementation.
	 */
	virtual void UpdateGeometry() = 0;
	/*! Updates the raytracer's settings.
	 * Should be called as little as possible since it may require uploading data to the GPU
	 */
	virtual void UpdateSettings() = 0;

	/*! Returns the ray traced texture */
	Texture* GetTexture();

private:
	Texture* m_texture;
};

