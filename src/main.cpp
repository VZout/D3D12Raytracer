#include <iostream>

#include <memory>
#include <chrono>

#include "window.hpp"
#include "d3d12_viewer.hpp"
#include "d3d12_ray_tracer.hpp"
//#include "cpu_ray_tracer.hpp"
#ifdef ENABLE_IMGUI
#include "imgui\imgui.h"
#endif

static float rt_viewport_size = 1;
static float rt_z_near = 1;
static float rt_epsilon = /*std::numeric_limits<float>::epsilon()*/ 0.000011920929;
static fm::vec2 rt_canvas_size;
static fm::vec3 rt_camera_pos = { 0, 0, -12 };
static fm::vec3 rt_sky_color = { 190.f / 255.f, 240.f / 255.f, 1 };
static fm::vec3 rt_floor_color = { 1, 1, 1 };
static bool rt_use_cpu = false;
static float rt_gamma = 2.2f; 
static float rt_exposure = 1.f;

int CALLBACK WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int cmd_show)
{
	auto app = std::make_unique<Application>(instance, cmd_show, "Raytracer", 600, 600);

	rt_canvas_size = { (float)app->GetWidth(), (float)app->GetHeight() };
	rt_sky_color = { 0, 0, 0 };
	
	app->SetKeyCallback([&app](int key, int, int)
	{
		if (key == 0x51)
		{
			app->Stop();
		}
	});

	auto viewer = std::make_unique<D3D12Viewer>(*app);
	auto ray_tracer = std::make_shared<D3D12RayTracer>();
	//auto cpu_ray_tracer = std::make_shared<CPURayTracer>();
	viewer->SetRayTracer(ray_tracer.get());

	// Timing variables
	bool imgui_show_properties = true;
	int frames = 0;
	int fps = 0;
	double cpu_frame_time = 0;
	std::chrono::time_point<std::chrono::high_resolution_clock> last_sec;
	std::chrono::time_point<std::chrono::high_resolution_clock> last_frame;

	std::vector<fm::vec3> vertices =
	{
		{ -1, 0, 0 },
		{ 1, 0, 0 },
		{ 0, 1, 0 },
	};

	std::vector<std::int32_t> indices =
	{
		0, 1, 2
	};

	RTGeometry scene_triangles;

	for (auto i = 0; i < 10; i++)
	{
		scene_triangles.triangles[i].specular = 20;
		scene_triangles.triangles[i].metal = 0.2;
		scene_triangles.triangles[i].color = { 1, 1, 1 };
	}

	scene_triangles.num_triangles = 10;
	float rect_size = 4.0f;
	float depth = 4;
	// #### BACK WALL
	// bottom left tri
	scene_triangles.triangles[0].a = { -rect_size, rect_size, depth };
	scene_triangles.triangles[0].b = { -rect_size, -rect_size, depth };
	scene_triangles.triangles[0].c = { rect_size, -rect_size, depth };
	scene_triangles.triangles[0].normal = { 0, 0, 1 };
	// top right tri
	scene_triangles.triangles[1].a = { -rect_size, rect_size, depth };
	scene_triangles.triangles[1].b = { rect_size, rect_size, depth };
	scene_triangles.triangles[1].c = { rect_size, -rect_size, depth };
	scene_triangles.triangles[1].normal = { 0, 0, 1 };

	// #### TOP WALL
	// bottom left tri
	scene_triangles.triangles[2].a = { -rect_size, depth, rect_size };
	scene_triangles.triangles[2].b = { -rect_size, depth, -rect_size };
	scene_triangles.triangles[2].c = { rect_size, depth, -rect_size };
	scene_triangles.triangles[2].normal = { 0, 1, 0 };
	// top right tri
	scene_triangles.triangles[3].a = { -rect_size, depth, rect_size };
	scene_triangles.triangles[3].b = { rect_size, depth, rect_size };
	scene_triangles.triangles[3].c = { rect_size, depth, -rect_size };
	scene_triangles.triangles[3].normal = { 0, 1, 0 };

	// #### BOTTOM WALL
	// bottom left tri
	scene_triangles.triangles[4].a = { -rect_size, -depth, rect_size };
	scene_triangles.triangles[4].b = { -rect_size, -depth, -rect_size };
	scene_triangles.triangles[4].c = { rect_size, -depth, -rect_size };
	scene_triangles.triangles[4].normal = { 0, -1, 0 };
	// top right tri
	scene_triangles.triangles[5].a = { -rect_size, -depth, rect_size };
	scene_triangles.triangles[5].b = { rect_size, -depth, rect_size };
	scene_triangles.triangles[5].c = { rect_size, -depth, -rect_size };
	scene_triangles.triangles[5].normal = { 0, -1, 0 };

	// #### RIGHT WALL
	// bottom left tri
	scene_triangles.triangles[6].a = { depth, -rect_size, rect_size };
	scene_triangles.triangles[6].b = { depth, -rect_size, -rect_size };
	scene_triangles.triangles[6].c = { depth, rect_size, -rect_size };
	scene_triangles.triangles[6].normal = { 1, 0, 0 };
	scene_triangles.triangles[6].color = { 0, 1, 0 };
	// top right tri
	scene_triangles.triangles[7].a = { depth, -rect_size, rect_size };
	scene_triangles.triangles[7].b = { depth, rect_size, rect_size };
	scene_triangles.triangles[7].c = { depth, rect_size, -rect_size };
	scene_triangles.triangles[7].normal = { 1, 0, 0 };
	scene_triangles.triangles[7].color = { 0, 1, 0 };

	// #### LEFT WALL
	// bottom left tri
	scene_triangles.triangles[8].a = { -depth, -rect_size, rect_size };
	scene_triangles.triangles[8].b = { -depth, -rect_size, -rect_size };
	scene_triangles.triangles[8].c = { -depth, rect_size, -rect_size };
	scene_triangles.triangles[8].normal = { -1, 0, 0 };
	scene_triangles.triangles[8].color = { 1, 0, 0 };
	// top right tri
	scene_triangles.triangles[9].a = { -depth, -rect_size, rect_size };
	scene_triangles.triangles[9].b = { -depth, rect_size, rect_size };
	scene_triangles.triangles[9].c = { -depth, rect_size, -rect_size };
	scene_triangles.triangles[9].normal = { -1, 0, 0 };
	scene_triangles.triangles[9].color = { 1, 0, 0 };

	ray_tracer->UpdateGeometry(viewer.get(), scene_triangles, true);

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
		viewer->ImGui_RenderSystemInfo();
		ImGui::Separator();
		ImGui::PushItemWidth(100);
		ImGui::DragFloat("Projection Near", &rt_z_near, 0.01f, 0);
		ImGui::DragFloat("Epsilon", &rt_epsilon, 0.01f, 0);
		ImGui::DragFloat("Viewport Size", &rt_viewport_size, 0.01f, 0);
		ImGui::DragFloat2("Canvas Size", rt_canvas_size.data);
		ImGui::Checkbox("Use CPU", &rt_use_cpu);
		ImGui::PopItemWidth();
		ImGui::Separator();
		ImGui::DragFloat3("Camera Position", rt_camera_pos.data, 0.1f);
		ImGui::ColorEdit3("Sky Color", rt_sky_color.data);
		ImGui::ColorEdit3("Floor Color", rt_floor_color.data);
		ImGui::DragFloat("Gamma", &rt_gamma, 0.1f);
		ImGui::DragFloat("Exposure", &rt_exposure, 0.1f);
		ImGui::Separator();
		ImGui::Button("Reload GPU Raytracer");
		ImGui::End();
#endif

		RTProperties properties;
		properties.z_near = rt_z_near;
		properties.canvas_size = rt_canvas_size;
		properties.viewport_size = rt_viewport_size;
		properties.epsilon = rt_epsilon;
		properties.camera_pos = rt_camera_pos;
		properties.use_cpu = rt_use_cpu;
		properties.sky_color = rt_sky_color;
		properties.gamma = rt_gamma;
		properties.exposure = rt_exposure;
		properties.floor_color = rt_floor_color;

		if (!rt_use_cpu)
		{
			ray_tracer->UpdateSettings(viewer.get(), properties);
		}
		ray_tracer->TracePixel(viewer.get(), 0, 0);

		if (rt_use_cpu)
		{
			//cpu_ray_tracer->UpdateSettings(viewer.get(), properties);
			//cpu_ray_tracer->TracePixel(viewer.get(), 0, 0);
		}

		viewer->Present();
	}

	return 0;
}
