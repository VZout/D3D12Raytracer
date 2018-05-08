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
	void UpdateGeometry(Viewer* viewer, std::array<Triangle, NUM_GEOMETRY> geometry, bool all_frames = false) override;
	void UpdateMaterials(Viewer* viewer, RTMaterials geometry, int num_materials, bool all_frames = false);
	void UpdateSettings(Viewer* viewer, RTProperties properties) override;

public:
	std::pair<std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, 2>, std::array<UINT8*, 2>> m_properties_const_buffer;
	std::pair<std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, 2>, std::array<UINT8*, 2>> m_geom_const_buffer;
	std::pair<std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, 2>, std::array<UINT8*, 2>> m_material_const_buffer;
};