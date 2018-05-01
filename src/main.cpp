#include <iostream>

#include <memory>
#include <chrono>

#include "window.hpp"
#include "d3d12_viewer.hpp"
#include "d3d12_ray_tracer.hpp"
#ifdef ENABLE_IMGUI
#include "imgui\imgui.h"
#endif

float rt_viewport_size = 1;
float rt_z_near = 1;
float rt_epsilon = 0.08;
float rt_canvas_size[2] = { 600, 600 };
float rt_camera_pos[3] = { 0, 0, -3 };
fm::vec3 rt_sky_color = { 190.f / 255.f, 240.f / 255.f, 1 };
bool rt_use_cpu = false;

int CALLBACK WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int cmd_show)
{
	auto app = std::make_unique<Application>(instance, cmd_show, "Raytracer", 600, 600);
	
	app->SetKeyCallback([&app](int key, int, int)
	{
		if (key == VK_ESCAPE)
		{
			app->Stop();
		}
	});

	auto viewer = std::make_unique<D3D12Viewer>(*app);
	auto ray_tracer = std::make_shared<D3D12RayTracer>();
	viewer->SetRayTracer(ray_tracer.get());

	// Timing variables
	bool imgui_show_properties = true;
	int frames = 0;
	int fps = 0;
	double cpu_frame_time = 0;
	std::chrono::time_point<std::chrono::high_resolution_clock> last_sec;
	std::chrono::time_point<std::chrono::high_resolution_clock> last_frame;

	while (app->IsRunning())
	{
		// Update FPS
		auto now = std::chrono::high_resolution_clock::now();
		{
			frames++;
			auto diff = std::chrono::duration_cast<std::chrono::seconds>(now - last_sec);
			if (diff.count() >= 1)
			{
				last_sec = std::chrono::high_resolution_clock::now();
				fps = frames;
				frames = 0;
			}
		}

		// Update Frametime
		{
			auto diff = std::chrono::duration_cast<std::chrono::microseconds>(now - last_frame);
			last_frame = std::chrono::high_resolution_clock::now();
			cpu_frame_time = diff.count();
		}

		app->PollEvents();

		viewer->NewFrame();

		// ImGui
#ifdef ENABLE_IMGUI
		ImGui::Begin("Raytracer Properties", &imgui_show_properties);
		ImGui::Text("Framerate: %d", fps);
		ImGui::Text("CPU Frametime: %f (Mu)", cpu_frame_time);
		ImGui::Separator();
		ImGui::PushItemWidth(100);
		ImGui::DragFloat("Projection Near", &rt_z_near, 0.01f, 0);
		ImGui::DragFloat("Epsilon", &rt_epsilon, 0.01f, 0);
		ImGui::DragFloat("Viewport Size", &rt_viewport_size, 0.01f, 0);
		ImGui::DragFloat2("Canvas Size", rt_canvas_size);
		ImGui::Checkbox("Use CPU", &rt_use_cpu);
		ImGui::PopItemWidth();
		ImGui::Separator();
		ImGui::DragFloat3("Camera Position", rt_camera_pos, 0.1f);
		ImGui::ColorEdit3("SkyColor", rt_sky_color.data);
		ImGui::End();
#endif

		RTProperties properties;
		properties.z_near = rt_z_near;
		properties.canvas_size = { rt_canvas_size[0], rt_canvas_size[1] };
		properties.viewport_size = rt_viewport_size;
		properties.epsilon = rt_epsilon;
		properties.camera_pos = { rt_camera_pos[0], rt_camera_pos[1], rt_camera_pos[2] };
		properties.use_cpu = rt_use_cpu;
		properties.sky_color = rt_sky_color;

		ray_tracer->UpdateSettings(viewer.get(), properties);
		ray_tracer->TracePixel(viewer.get(), 0, 0);

		viewer->Present();
	}

	return 0;
}
