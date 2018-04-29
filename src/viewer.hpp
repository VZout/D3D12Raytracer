#pragma once

#include <memory>

class RayTracer;
class Application;

class Viewer
{
public:
	/*! @param app Reference to the target Application */
	explicit Viewer(Application& app);
	virtual ~Viewer() = default;

	Viewer(const Viewer&) = delete;
    Viewer& operator=(const Viewer&) = delete;
    Viewer(Viewer&&) = delete;
    Viewer& operator=(Viewer&&) = delete;

	/*! @param ray_tracer Sets the active ray tracer */
	void SetRayTracer(RayTracer* ray_tracer);

	/*! Begins a new frame.
	 * In between `NewFrame` and `Present` you can call any ImGui or drawing functions.
	 * GPU synchronization occurs here.
	 * */
	virtual void NewFrame() = 0;
	/*! Presents the frame */
	virtual void Present() = 0;

protected:
	Application& m_app;
	RayTracer* m_ray_tracer;

	bool m_allow_fullscreen;
};

