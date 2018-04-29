#include <iostream>

#include <memory>
#include <chrono>

#include "window.hpp"
#include "d3d12_viewer.hpp"
#include "d3d12_ray_tracer.hpp"
#include "imgui\imgui.h"

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
		/*
		ImGui::Begin("Raytracer Properties", &imgui_show_properties);
		ImGui::Text("Framerate: %d", fps);
		ImGui::Text("CPU Frametime: %f (Mu)", cpu_frame_time);
		ImGui::Separator();
		ImGui::End();
		*/

		ray_tracer->TracePixel(0, 0);

		viewer->Present();
	}

	return 0;
}
