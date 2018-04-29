#include "ray_tracer.hpp"

RayTracer::RayTracer() : m_texture(nullptr)
{
}

Texture * RayTracer::GetTexture()
{
	return m_texture;
}
