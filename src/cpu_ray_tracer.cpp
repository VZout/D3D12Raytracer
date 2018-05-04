#include "cpu_ray_tracer.hpp"

#include "d3d12_viewer.hpp"
#include "window.hpp"

CPURayTracer::CPURayTracer() : RayTracer()
{
	pixels = new fm::vec4[600 * 600];
}

void CPURayTracer::Initialize(Viewer* viewer)
{

}

void CPURayTracer::TracePixel(Viewer* viewer, std::uint32_t _x, std::uint32_t _y)
{
	auto d3d12_viewer = static_cast<D3D12Viewer*>(viewer);

	/*
	for (auto k = 0; k < 300; k++)
	{
		Input input = { { (float)x - 300, (float)y - 300 } };
		fm::vec4 color = PixelTrace(input);
		pixels[rp] = color;

		rp++;
		x++;
		if (x > 599)
		{
			y++;
			x = 0;
		}
		if (rp == 600 * 600)
		{
			x = 0;
			y = 0;
			rp = 0;
		}
	}
	*/

	x = 0;
	y = 0;

	for (auto k = 0; k < d3d12_viewer->m_app.GetWidth() * d3d12_viewer->m_app.GetHeight(); k++)
	{
		Input input = { { (float)x - (d3d12_viewer->m_app.GetWidth() / 2), (float)y - (d3d12_viewer->m_app.GetHeight() / 2) } };
		//fm::vec4 color = PixelTrace(input);
		fm::vec4 color = {1, 1, 1, 1 };
		pixels[k] = color;

		x++;
		if (x > 599)
		{
			y++;
			x = 0;
		}
	}

	d3d12_viewer->UpdateRenderTexture(d3d12_viewer->m_cmd_list, d3d12_viewer->m_render_texture, (BYTE*)pixels);
}

void CPURayTracer::UpdateGeometry(Viewer* viewer, RTGeometry geomertry, bool all_frames)
{
}

void CPURayTracer::UpdateSettings(Viewer* viewer, RTProperties properties)
{
	m_properties = properties;

	/*camera_pos = properties.camera_pos;
	sky_color = properties.sky_color;
	canvas_size = properties.canvas_size;
	epsilon = properties.epsilon;
	viewport_size = properties.viewport_size;
	z_near = properties.z_near;
	gamma = properties.gamma;
	exposure = properties.exposure;
	floor_color = properties.floor_color;*/
}
