#include "viewer.hpp"

Viewer::Viewer(Application& app) : m_app(app), m_ray_tracer(nullptr), m_allow_fullscreen(false)
{
}

void Viewer::SetRayTracer(RayTracer* ray_tracer)
{
	m_ray_tracer = ray_tracer;
}