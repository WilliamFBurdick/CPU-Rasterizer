#pragma once
#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

class Application
{
public:
	Application(const char* title, int width, int height);
	~Application();
	void Clear();
	void Present();

	void SetShouldClose(bool shouldClose) { m_ShouldClose = shouldClose; }

	bool ShouldClose() const { return m_ShouldClose; }
	SDL_Renderer* GetRenderer() const { return m_Renderer; }

private:
	SDL_Window* m_Window;
	SDL_Renderer* m_Renderer;
	bool m_ShouldClose;
};