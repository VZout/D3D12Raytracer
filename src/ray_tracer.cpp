#include "ray_tracer.hpp"

RayTracer::RayTracer() : m_initialized(false), m_texture(nullptr)
{
}

Texture* RayTracer::GetTexture() const
{
	return m_texture;
}
