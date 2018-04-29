#pragma once

#include "ray_tracer.hpp"

class D3D12RayTracer : public RayTracer
{
public:
	D3D12RayTracer();
	~D3D12RayTracer() override = default;

	void TracePixel(std::uint32_t x, std::uint32_t y) override;
	void UpdateGeometry() override {};
	void UpdateSettings() override {};
};