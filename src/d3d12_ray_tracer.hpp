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
	void UpdateGeometry(Viewer* viewer, std::array<Triangle, 1> geometry, bool all_frames = false) override;
	void UpdateVertices(Viewer* viewer, std::vector<Vertex> vertices, bool all_frames = false);
	void UpdateIndices(Viewer* viewer, std::vector<INDICES_TYPE> indices, bool all_frames = false);
	void UpdateMaterials(Viewer* viewer, RTMaterials geometry, int num_materials, bool all_frames = false);
	void UpdateSettings(Viewer* viewer, RTProperties properties) override;

public:
	std::pair<std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, 2>, std::array<UINT8*, 2>> m_properties_const_buffer;
	std::pair<std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, 1>, std::array<UINT8*, 1>> m_vertices_buffer;
	std::pair<std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, 1>, std::array<UINT8*, 1>> m_indices_buffer;
	std::pair<std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, 1>, std::array<UINT8*, 1>> m_material_const_buffer;
};