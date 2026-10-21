#include "Application.h"
#include <stdexcept>

Application::Application(const char* title, int width, int height)
{
	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		SDL_Log("Failed to initialize SDL");
		throw std::runtime_error("Failed to initialize SDL");
	}
	m_Window = SDL_CreateWindow(title, width, height, SDL_WINDOW_RESIZABLE);
	if (!m_Window)
	{
		SDL_Log("Failed to create SDL window");
		throw std::runtime_error("Failed to create SDL window");
	}
	m_Renderer = SDL_CreateRenderer(m_Window, NULL);
	if (!m_Renderer)
	{
		SDL_Log("Failed to create SDL renderer");
		throw std::runtime_error("Failed to create SDL renderer");
	}
	m_ShouldClose = false;

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	(void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	ImGui::StyleColorsDark();

	float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
	ImGuiStyle& style = ImGui::GetStyle();
	style.ScaleAllSizes(main_scale);
	style.FontScaleDpi = main_scale;

	ImGui_ImplSDL3_InitForSDLRenderer(m_Window, m_Renderer);
	ImGui_ImplSDLRenderer3_Init(m_Renderer);
}

Application::~Application()
{
	SDL_DestroyRenderer(m_Renderer);
	SDL_DestroyWindow(m_Window);
	SDL_Quit();
}

void Application::Clear()
{
	SDL_RenderClear(m_Renderer);
	ImGui_ImplSDLRenderer3_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();
}

void Application::Present()
{
	ImGui::Render();
	ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), m_Renderer);
	SDL_RenderPresent(m_Renderer);
}
