#include "d3d12_ray_tracer.hpp"

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

	m_vertices_buffer = d3d12_viewer->CreateStructuredBuffer<1>(sizeof(Vertex) * NUM_VERTICES);
	m_indices_buffer = d3d12_viewer->CreateByteAddressBuffer<1>(sizeof(INDICES_TYPE) * (NUM_INDICES));
	m_bvh_buffer = d3d12_viewer->CreateStructuredBuffer<1>(sizeof(BVHNode) * BVH_NODES);

	// Create the SRV to the structured buffers.
	//auto handle = (CD3DX12_CPU_DESCRIPTOR_HANDLE)d3d12_viewer->m_main_srv_desc_heap->GetCPUDescriptorHandleForHeapStart();
	//handle.Offset(1, d3d12_viewer->m_cbv_srv_uav_increment_size);
	//d3d12_viewer->CreateStructuredBufferSRV(m_vertices_buffer.first, handle, NUM_VERTICES, sizeof(Vertex));

	//handle = (CD3DX12_CPU_DESCRIPTOR_HANDLE)d3d12_viewer->m_main_srv_desc_heap->GetCPUDescriptorHandleForHeapStart();
//	handle.Offset(3, d3d12_viewer->m_cbv_srv_uav_increment_size);
	//d3d12_viewer->CreateByteAddressBufferSRV(m_indices_buffer.first, handle, NUM_INDICES / (sizeof(std::uint32_t) / sizeof(INDICES_TYPE))); // device the number of indices by 2 since 1 position in the buffer is 32 bit and our index type is 16 bit.

	m_material_const_buffer = d3d12_viewer->CreateConstantBuffer<1>(sizeof(RTMaterials));

	m_initialized = true;
}

void D3D12RayTracer::TracePixel(Viewer* viewer, std::uint32_t x, std::uint32_t y)
{
	auto d3d12_viewer = static_cast<D3D12Viewer*>(viewer);

	d3d12_viewer->m_cmd_list->SetGraphicsRootConstantBufferView(0, m_properties_const_buffer.first[d3d12_viewer->m_frame_idx]->GetGPUVirtualAddress());
	d3d12_viewer->m_cmd_list->SetGraphicsRootConstantBufferView(1, m_material_const_buffer.first[0]->GetGPUVirtualAddress());
	d3d12_viewer->m_cmd_list->SetGraphicsRootShaderResourceView(2, m_vertices_buffer.first[0]->GetGPUVirtualAddress());
	d3d12_viewer->m_cmd_list->SetGraphicsRootShaderResourceView(3, m_indices_buffer.first[0]->GetGPUVirtualAddress());
	d3d12_viewer->m_cmd_list->SetGraphicsRootShaderResourceView(4, m_bvh_buffer.first[0]->GetGPUVirtualAddress());
	d3d12_viewer->m_cmd_list->DrawInstanced(4, 1, 0, 0);
}

void D3D12RayTracer::UpdateGeometry(Viewer* viewer, std::array<Triangle, 1> geometry, bool all_frames)
{
}

void D3D12RayTracer::UpdateVertices(Viewer * viewer, std::vector<Vertex> vertices, bool all_frames)
{
	auto d3d12_viewer = static_cast<D3D12Viewer*>(viewer);
	size_t size = sizeof(Vertex) * NUM_VERTICES;

	if (all_frames)
	{
		for (auto i = 0; i < m_vertices_buffer.first.size(); i++)
		{
			memcpy(GET_CB_ADDRESS(m_vertices_buffer, i), vertices.data(), size);
		}
	}
	else
	{
		memcpy(GET_CB_ADDRESS(m_vertices_buffer, d3d12_viewer->m_frame_idx), vertices.data(), size);
	}
}

void D3D12RayTracer::UpdateBVH(Viewer * viewer, std::array<BVHNode, BVH_NODES> nodes)
{
	auto d3d12_viewer = static_cast<D3D12Viewer*>(viewer);
	size_t size = sizeof(BVHNode) * BVH_NODES;

	for (auto i = 0; i < m_bvh_buffer.first.size(); i++)
	{
		memcpy(GET_CB_ADDRESS(m_bvh_buffer, i), nodes.data(), size);
	}
}

void D3D12RayTracer::UpdateIndices(Viewer * viewer, std::vector<INDICES_TYPE> indices, bool all_frames)
{
	auto d3d12_viewer = static_cast<D3D12Viewer*>(viewer);
	size_t size = sizeof(INDICES_TYPE) * indices.size();

	if (all_frames)
	{
		for (auto i = 0; i < m_indices_buffer.first.size(); i++)
		{
			memcpy(GET_CB_ADDRESS(m_indices_buffer, i), indices.data(), size);
		}
	}
	else
	{
		memcpy(GET_CB_ADDRESS(m_indices_buffer, d3d12_viewer->m_frame_idx), indices.data(), size);
	}
}

void D3D12RayTracer::UpdateMaterials(Viewer * viewer, RTMaterials geometry, int num_materials, bool all_frames)
{
	auto d3d12_viewer = static_cast<D3D12Viewer*>(viewer);
	size_t size = (sizeof(RTMaterials) * num_materials);
	if (all_frames)
	{
		for (auto i = 0; i < m_material_const_buffer.first.size(); i++)
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
