#pragma once

#include "ray_tracer.hpp"
#include "../raytracer.ps"
#include "vec.hpp"

class Viewer;

class CPURayTracer : public RayTracer
{
public:
	CPURayTracer();
	~CPURayTracer() override = default;

	fm::vec4* pixels;

	void Initialize(Viewer* viewer) override;
	void TracePixel(Viewer* viewer, std::uint32_t x, std::uint32_t y) override;
	void UpdateGeometry(Viewer* viewer, std::array<Triangle, NUM_GEOMETRY> geometry, bool all_frames = false) override;
	void UpdateSettings(Viewer* viewer, RTProperties properties) override;

	int rp = 0;
	int x = 0;
	int y = 0;
};