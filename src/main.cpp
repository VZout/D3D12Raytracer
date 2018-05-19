#include <iostream>

#include <memory>
#include <chrono>

#include "bvh.hpp"
#include "window.hpp"
#include "d3d12_viewer.hpp"
#include "d3d12_ray_tracer.hpp"
#include "model.hpp"
//#include "cpu_ray_tracer.hpp"
#ifdef ENABLE_IMGUI
#include "imgui\imgui.h"
#endif

static float rt_viewport_size = 1;
static float rt_z_near = 1;
static float rt_epsilon = /*std::numeric_limits<float>::epsilon()*/ 0.000011920929;
static fm::vec2 rt_canvas_size;
static fm::vec3 rt_camera_pos = { 0, 2, -6 };
static fm::vec3 rt_sky_color = { 190.f / 255.f, 240.f / 255.f, 1 };
static fm::vec3 rt_floor_color = { 1, 1, 1 };
static bool rt_use_cpu = false;
static float rt_gamma = 2.2f; 
static float rt_exposure = 1.f;

static bool rt_temp = false;

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

	std::vector<Vertex> scene_vertices;
	std::vector<INDICES_TYPE> scene_indices;
	RTMaterials materials;

	materials.materials[0].color = { 1, 1, 1 };
	materials.materials[0].metal = 0.4;
	materials.materials[0].specular = 10;
	materials.materials[1].color = { 1, 0, 0 };
	materials.materials[1].metal = 0.4;
	materials.materials[1].specular = 10;
	materials.materials[2].color = { 0, 1, 0 };
	materials.materials[2].metal = 0.4;
	materials.materials[2].specular = 10;

	{ /* LOAD MODEL AND CONVERT TO CRAPPY TRIANGLES*/
		rlr::Model* model = new rlr::Model();
		rlr::Load(*model, "scene.fbx");

		int vertex_offset = 0;
		for (auto m = 0; m < model->meshes.size(); m++)
		{
			for (auto i = 0; i < model->meshes[m].vertices.size(); i++)
			{
				auto pv = model->meshes[m].vertices[i];
				Vertex v;
				v.material_idx = m % 3;
				v.normal = pv.m_normal;
				v.position = pv.m_pos;
				v.uv = pv.m_texCoord;

				scene_vertices.push_back(v);
			}

			for (auto i = 0; i < model->meshes[m].indices.size(); i++)
			{
				scene_indices.push_back((int)model->meshes[m].indices[i] + vertex_offset);
			}
			vertex_offset += model->meshes[m].vertices.size();
		}
	}
	scene_vertices.resize(NUM_VERTICES);

	BVH<90/3> bvh;
	bvh.Construct(scene_vertices, scene_indices);

	ray_tracer->UpdateVertices(viewer.get(), scene_vertices, true);
	ray_tracer->UpdateIndices(viewer.get(), bvh.big_index_buffer, true);
	ray_tracer->UpdateMaterials(viewer.get(), materials, materials.materials.size(), true);


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
		ImGui::Checkbox("Use temp", &rt_temp);
		ImGui::PopItemWidth();
		ImGui::Separator();
		ImGui::DragFloat3("Camera Position", rt_camera_pos.data, 0.1f);
		ImGui::ColorEdit3("Sky Color", rt_sky_color.data);
		ImGui::ColorEdit3("Floor Color", rt_floor_color.data);
		ImGui::DragFloat("Gamma", &rt_gamma, 0.1f);
		ImGui::DragFloat("Exposure", &rt_exposure, 0.1f);
		ImGui::Separator();
		ImGui::Button("Reload GPU Raytracer");

		if (ImGui::CollapsingHeader("Material Editor"))
		{
			for (auto i = 0; i < materials.materials.size(); i++)
			{
				ImGui::DragFloat(std::string("Metal " + std::to_string(i)).c_str(), &materials.materials[i].metal, 0.1f);
				ImGui::DragFloat(std::string("Specular " + std::to_string(i)).c_str(), &materials.materials[i].specular, 0.1f);
				ImGui::ColorEdit3(std::string("Color " + std::to_string(i)).c_str(), materials.materials[i].color.data);
			}
			ray_tracer->UpdateMaterials(viewer.get(), materials, materials.materials.size(), true);
		}
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

		if (rt_temp)
		{
			for (auto i = 0; i < properties.randoms.size(); i++)
			{
				properties.randoms[i] = 2;
			}
		}
		else
		{
			for (auto i = 0; i < properties.randoms.size(); i++)
			{
				srand(std::chrono::high_resolution_clock::now().time_since_epoch().count());
				properties.randoms[i] = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
			}
		}

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