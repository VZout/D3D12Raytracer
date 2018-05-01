#include "viewer.hpp"

#include "ray_tracer.hpp"

Viewer::Viewer(Application& app) : m_app(app), m_ray_tracer(nullptr), m_allow_fullscreen(false)
{
}

void Viewer::SetRayTracer(RayTracer* ray_tracer)
{
	ray_tracer->Initialize(this);
	m_ray_tracer = ray_tracer;
}