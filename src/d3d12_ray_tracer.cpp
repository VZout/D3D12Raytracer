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
	m_const_buffer = d3d12_viewer->CreateConstantBuffer<D3D12Viewer::num_back_buffers>(sizeof(RTProperties));

	m_initialized = true;
}

void D3D12RayTracer::TracePixel(Viewer* viewer, std::uint32_t x, std::uint32_t y)
{
	auto d3d12_viewer = static_cast<D3D12Viewer*>(viewer);

	d3d12_viewer->m_cmd_list->SetGraphicsRootConstantBufferView(0, m_const_buffer.first[d3d12_viewer->m_frame_idx]->GetGPUVirtualAddress());
	d3d12_viewer->m_cmd_list->DrawInstanced(4, 1, 0, 0);
}

void D3D12RayTracer::UpdateGeometry()
{
}

void D3D12RayTracer::UpdateSettings(Viewer* viewer, RTProperties properties)
{
	auto d3d12_viewer = static_cast<D3D12Viewer*>(viewer);

	memcpy(GET_CB_ADRESS(m_const_buffer, d3d12_viewer->m_frame_idx), &properties, sizeof(RTProperties));

	m_properties = properties;
}
