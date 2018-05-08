#include "d3d12_ray_tracer.hpp"

#include "d3d12_viewer.hpp"

D3D12RayTracer::D3D12RayTracer() : RayTracer()
{

}

void D3D12RayTracer::Initialize(Viewer* viewer)
{
	if (m_initialized)
	{ 
		return;
	}

	auto d3d12_viewer = static_cast<D3D12Viewer*>(viewer);

	// Create A CB
	m_properties_const_buffer = d3d12_viewer->CreateConstantBuffer<D3D12Viewer::num_back_buffers>(sizeof(RTProperties));

	auto triangle_size = (sizeof(fm::vec3) * 3) + sizeof(float);
	triangle_size = sizeof(Triangle);
	auto size = triangle_size;
	m_geom_const_buffer = d3d12_viewer->CreateStructuredBuffer<D3D12Viewer::num_back_buffers>(triangle_size * NUM_GEOMETRY);

	auto handle = (CD3DX12_CPU_DESCRIPTOR_HANDLE)d3d12_viewer->m_main_srv_desc_heap->GetCPUDescriptorHandleForHeapStart();
	handle.Offset(1, d3d12_viewer->m_cbv_srv_uav_increment_size);
	d3d12_viewer->CreateStructuredBufferSRV(m_geom_const_buffer.first, handle, NUM_GEOMETRY, sizeof(Triangle));

	m_material_const_buffer = d3d12_viewer->CreateConstantBuffer<D3D12Viewer::num_back_buffers>(sizeof(RTMaterials));

	m_initialized = true;
}

void D3D12RayTracer::TracePixel(Viewer* viewer, std::uint32_t x, std::uint32_t y)
{
	auto d3d12_viewer = static_cast<D3D12Viewer*>(viewer);

	d3d12_viewer->m_cmd_list->SetGraphicsRootConstantBufferView(0, m_properties_const_buffer.first[d3d12_viewer->m_frame_idx]->GetGPUVirtualAddress());
	//d3d12_viewer->m_cmd_list->SetGraphicsRootConstantBufferView(1, m_geom_const_buffer.first[d3d12_viewer->m_frame_idx]->GetGPUVirtualAddress());
	d3d12_viewer->m_cmd_list->SetGraphicsRootConstantBufferView(1, m_material_const_buffer.first[d3d12_viewer->m_frame_idx]->GetGPUVirtualAddress());
	d3d12_viewer->m_cmd_list->DrawInstanced(4, 1, 0, 0);
}

void D3D12RayTracer::UpdateGeometry(Viewer* viewer, std::array<Triangle, NUM_GEOMETRY> geometry, bool all_frames)
{
	auto d3d12_viewer = static_cast<D3D12Viewer*>(viewer);
	size_t size = sizeof(Triangle) * NUM_GEOMETRY;
	if (all_frames)
	{
		for (auto i = 0; i < d3d12_viewer->num_back_buffers; i++)
		{
			memcpy(GET_CB_ADDRESS(m_geom_const_buffer, i), geometry.data(), size);
		}
	}
	else
	{
		memcpy(GET_CB_ADDRESS(m_geom_const_buffer, d3d12_viewer->m_frame_idx), geometry.data(), size);
	}
}

void D3D12RayTracer::UpdateMaterials(Viewer * viewer, RTMaterials geometry, int num_materials, bool all_frames)
{
	auto d3d12_viewer = static_cast<D3D12Viewer*>(viewer);
	size_t size = (sizeof(RTMaterials) * num_materials);
	if (all_frames)
	{
		for (auto i = 0; i < d3d12_viewer->num_back_buffers; i++)
		{
			memcpy(GET_CB_ADDRESS(m_material_const_buffer, i), &geometry, size);
		}
	}
	else
	{
		memcpy(GET_CB_ADDRESS(m_material_const_buffer, d3d12_viewer->m_frame_idx), &geometry, size);
	}
}

void D3D12RayTracer::UpdateSettings(Viewer* viewer, RTProperties properties)
{
	auto d3d12_viewer = static_cast<D3D12Viewer*>(viewer);

	memcpy(GET_CB_ADDRESS(m_properties_const_buffer, d3d12_viewer->m_frame_idx), &properties, sizeof(RTProperties));

	m_properties = properties;
}
