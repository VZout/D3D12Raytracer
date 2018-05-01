#pragma once

#include "ray_tracer.hpp"

#include <wrl/client.h>
#include <utility>
#include <array>
#include <d3d12.h>

class Viewer;

class D3D12RayTracer : public RayTracer
{
public:
	D3D12RayTracer();
	~D3D12RayTracer() override = default;

	void Initialize(Viewer* viewer) override;
	void TracePixel(Viewer* viewer, std::uint32_t x, std::uint32_t y) override;
	void UpdateGeometry() override;
	void UpdateSettings(Viewer* viewer, RTProperties properties) override;

public:
	std::pair<std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, 2>, std::array<UINT8*, 2>> m_const_buffer;
};