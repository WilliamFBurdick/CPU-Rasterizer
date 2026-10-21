#include <random>
#include <thread>
#include "Application.h"
#include "my_gl.h"
#include "Framebuffer.h"
#include "Model.h"
#include "FileManagement.h"
#include <map>
#include <memory>
#include <SDL3/SDL_main.h>

const int width = 800;
const int height = 800;
const int shadoww = 8000, shadowh = 8000;

extern mat<4, 4> Viewport, ModelView, Perspective;
extern std::vector<double> zbuffer;

std::shared_ptr<Application> app;
std::shared_ptr<Framebuffer> framebuffer, shadowbuffer, maskbuffer;

std::unique_ptr<std::thread> drawThread;
bool renderInProgress = false;

// Asset directories that contain .obj files, in tuple format alongside .obj file path.
std::vector<std::tuple<std::string, std::string>> modelPaths;
std::map<std::string, bool> selectedModels;
std::vector<std::string> modelsToDraw;

void RenderMainMenuBar();

struct BlankShader : IShader
{
	const Model& model;

	BlankShader(const Model& m) : model(m) {}

	virtual vec4 vertex(const int face, const int vert)
	{
		vec4 gl_Position = ModelView * model.vert(face, vert);
		return Perspective * gl_Position;
	}

	virtual std::pair<bool, Color> fragment(const vec3 bar) const
	{
		return { false, {255, 255, 255, 255} };
	}
};

struct PhongShader : IShader
{
	const Model& model;
	vec4 l;
	vec2 varying_uv[3];
	vec4 varying_nrm[3];
	vec4 tri[3];

	PhongShader(const vec3 light, const Model& m) : model(m)
	{
		l = normalized((ModelView * vec4{ light.x, light.y, light.z, 0.0 }));
	}

	virtual vec4 vertex(const int face, const int vert)
	{
		varying_uv[vert] = model.uv(face, vert);
		varying_nrm[vert] = ModelView.invert_transpose() * model.normal(face, vert);
		vec4 gl_Position = ModelView * model.vert(face, vert);
		tri[vert] = gl_Position;
		return Perspective * gl_Position;
	}

	virtual std::pair<bool, Color> fragment(const vec3 bar) const
	{
		mat<2, 4> E = { tri[1] - tri[0], tri[2] - tri[0] };
		mat<2, 2> U = { varying_uv[1] - varying_uv[0], varying_uv[2] - varying_uv[0] };
		mat<2, 4> T = U.invert() * E;
		mat<4, 4> D = { normalized(T[0]), normalized(T[1]), normalized(varying_nrm[0] * bar[0] + varying_nrm[1] * bar[1] + varying_nrm[2] * bar[2]), {0,0,0,1} };
		vec2 uv = varying_uv[0] * bar[0] + varying_uv[1] * bar[1] + varying_uv[2] * bar[2];
		vec4 n = normalized(D.transpose() * model.normal(uv));
		vec4 r = normalized(n * (n * l) * 2 - l);
		double ambient = 0.3;
		double diff = 1.0 * std::max(0.0, n * l);
		double spec = (1.0 + 3.0 * model.specular(uv)[0]/255.0) * std::pow(std::max(r.z, 0.0), 35);
		Color gl_FragColor = model.diffuse(uv);
		for (int channel : {0, 1, 2})
			gl_FragColor[channel] *= std::min(1.0, ambient + 0.4 * diff + 0.9 * spec);
		return { false, gl_FragColor };
	}
};

struct RandomShader : IShader
{
	const Model& model;
	Color color = {};
	vec3 tri[3];

	RandomShader(const Model& m) : model(m) {}

	virtual vec4 vertex(const int face, const int vert)
	{
		vec4 v = model.vert(face, vert);
		vec4 gl_Position = ModelView * v;
		tri[vert] = gl_Position.xyz();
		return Perspective * gl_Position;
	}

	virtual std::pair<bool, Color> fragment(const vec3 bar) const
	{
		return { false, color };
	}
};

Framebuffer store_zbuffer(SDL_Renderer* renderer, std::vector<double>& zbuffer, int width, int height)
{
	Framebuffer zmap(renderer, width, height);
	double minz = +1000;
	double maxz = -1000;
	
	for (int x = 0; x < width; x++)
	{
		for (int y = 0; y < height; y++)
		{
			double z = zbuffer[x + y * width];
			if (z < -100)
				continue;
			minz = std::min(z, minz);
			maxz = std::max(z, maxz);
		}
	}
	for (int x = 0; x < width; x++)
	{
		for (int y = 0; y < height; y++)
		{
			double z = zbuffer[x + y * width];
			if (z < -100)
				continue;
			z = (z - minz) / (maxz - minz) * 255.0;
			zmap.Set(x, y, { (unsigned char)z, 255, 255, 255 });
		}
	}
	return zmap;
}

void render_scene(SDL_Renderer* renderer, std::vector<string> filenames)
{
	renderInProgress = true;
	framebuffer->Clear({ 177, 195, 209, 255 });
	shadowbuffer->Clear();
	maskbuffer->Clear();

	// Setup scene
	const vec3 light{ 1, 1, 1 };
	const vec3 eye{ -1, 0, 2 };
	const vec3 center{ 0, 0, 0 };
	const vec3 up{ 0, 1, 0 };

	// Standard rendering pass
	lookat(eye, center, up);
	init_perspective(norm(eye - center));
	init_viewport(width / 16, height / 16, width * 7 / 8, height * 7 / 8);
	init_zbuffer(width, height);
	for (auto file : filenames)
	{
		const char* filename = file.c_str();
		Model model(filename);
		PhongShader shader(light, model);
		for (int f = 0; f < model.nfaces(); f++)
		{
			Triangle clip = {
				shader.vertex(f, 0),
				shader.vertex(f, 1),
				shader.vertex(f, 2)
			};
			rasterize(clip, shader, *framebuffer);
		}
	}

	// SSAO
	const double ao_radius = 0.1;
	const int nsamples = 128;
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<double> dist(-ao_radius, ao_radius);
	auto smoothstep = [](double edge0, double edge1, double x)
		{
			double t = std::clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
			return t * t * (3 - 2 * t);
		};
#pragma omp parallel for
	for (int x = 0; x < width; x++)
	{
		for (int y = 0; y < height; y++)
		{
			double z = zbuffer[x + y * width];
			if (z < -100)
				continue;
			vec4 fragment = Viewport.invert() * vec4 { (double)x, (double)y, (double)z, 1.0 };
			double vote = 0;
			double voters = 0;
			for (int i = 0; i < nsamples; i++)
			{
				vec4 p = Viewport * (fragment + vec4{ dist(gen), dist(gen), dist(gen), 0.0 });
				if (p.x < 0 || p.x >= width || p.y < 0 || p.y >= height)
					continue;
				double d = zbuffer[int(p.x) + int(p.y) * width];
				if (z + 8 * ao_radius < d)
					continue;
				voters++;
				vote += d > p.z;
			}
			double ssao = smoothstep(0, 1, 1 - vote / voters * 0.4);
			Color c = Color::Convert<uint32_t>(framebuffer->Get(x, y));
			framebuffer->Set(x, y, { (unsigned char)(c.rgba[0] * ssao), (unsigned char)(c.rgba[1] * ssao), (unsigned char)(c.rgba[2] * ssao), c.rgba[3] });
		}
	}

	std::vector<bool> mask(width * height, false);
	std::vector<double> zbuffer_copy = zbuffer;
	mat<4, 4> M = (Viewport * Perspective * ModelView).invert();

	// Shadow rendering pass
	lookat(light, center, up);
	init_perspective(norm(eye - center));
	init_viewport(shadoww / 16, shadowh / 16, shadoww * 7 / 8, shadowh * 7 / 8);
	init_zbuffer(shadoww, shadowh);

	for (auto file : filenames)
	{
		const char* filename = file.c_str();
		Model model(filename);
		BlankShader shader(model);
		for (int f = 0; f < model.nfaces(); f++)
		{
			Triangle clip = { shader.vertex(f, 0), shader.vertex(f, 1), shader.vertex(f, 2) };
			rasterize(clip, shader, *shadowbuffer);
			std::cerr << "Shadow Rendering: f#" << f << std::endl;
		}
	}

	mat<4, 4> N = Viewport * Perspective * ModelView;

	// post-processing
	for (int x = 0; x < width; x++)
	{
		for (int y = 0; y < height; y++)
		{
			vec4 fragment = M * vec4{ (double)x, (double)y, zbuffer_copy[x + y * width], 1.0 };
			vec4 q = N * fragment;
			vec3 p = q.xyz() / q.w;
			bool lit = (fragment.z < -100 || (p.x < 0 || p.x >= shadoww || p.y < 0 || p.y >= shadowh) || (p.z > zbuffer[int(p.x) + int(p.y) * shadoww] - 0.03));
			mask[x + y * width] = lit;
		}
	}

	for (int x = 0; x < width; x++)
	{
		for (int y = 0; y < height; y++)
		{
			if (mask[x + y * width])
				continue;
			maskbuffer->Set(x, y, { 255, 255, 255, 255 });
		}
	}

	for (int x = 0; x < width; x++)
	{
		for (int y = 0; y < height; y++)
		{
			if (mask[x + y * width])
				continue;
			Color c = Color::Convert<uint32_t>(framebuffer->Get(x, y));
			vec3 a = { c.rgba[0], c.rgba[1], c.rgba[2] };
			if (norm(a) < 80)
				continue;
			a = normalized(a) * 80;
			framebuffer->Set(x, y, { (unsigned char)a[0], (unsigned char)a[1], (unsigned char)a[2], 255 });
		}
	}
	renderInProgress = false;
}

int main(int argc, char** argv)
{
	modelPaths = GetModelPaths();
	selectedModels = std::map<std::string, bool>();
	for (auto d : modelPaths)
	{
		std::cout << "Model Found: " << std::get<0>(d).c_str() << " - Path: " << std::get<1>(d).c_str() << std::endl;
		selectedModels.emplace(std::get<1>(d), false);
	}

	app = std::make_shared<Application>("Rasterizer", 1080, 1080);
	Color clearColor = { 177, 195, 209, 255 };
	framebuffer = std::make_shared<Framebuffer>(app->GetRenderer(), width, height, 4, clearColor);
	// Set up shadowbuffer and maskbuffer
	shadowbuffer = std::make_shared<Framebuffer>(app->GetRenderer(), shadoww, shadowh, 3);
	maskbuffer = std::make_shared<Framebuffer>(app->GetRenderer(), width, height, 4);

	while (!app->ShouldClose())
	{
		SDL_Event e;
		while (SDL_PollEvent(&e))
		{
			ImGui_ImplSDL3_ProcessEvent(&e);
			switch (e.type)
			{
			case SDL_EVENT_QUIT:
				app->SetShouldClose(true);
				break;
			}
		}
		app->Clear();
		RenderMainMenuBar();

		framebuffer->Render(app->GetRenderer());
		app->Present();
	}
	if (drawThread != nullptr)
		drawThread->join();
	return 0;
}

void RenderMainMenuBar()
{
	ImGui::BeginDisabled(renderInProgress);
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("Model"))
		{
			if (ImGui::BeginMenu("Select Model"))
			{
				for (auto m : modelPaths)
				{
					auto modelLocation = std::find(modelsToDraw.begin(), modelsToDraw.end(), std::get<1>(m));
					bool modelIsQueued = modelLocation != modelsToDraw.end();
					if (ImGui::MenuItem(std::get<0>(m).c_str(), 0, modelIsQueued))
					{
						if (modelIsQueued)
						{
							modelsToDraw.erase(modelLocation);
						}
						else
						{
							modelsToDraw.emplace_back(std::get<1>(m));
						}
					}
				}
				ImGui::EndMenu();
			}
			if (ImGui::MenuItem("Render"))
			{
				if (drawThread != nullptr)
					drawThread.release();
				drawThread = std::make_unique<std::thread>(render_scene, app->GetRenderer(), modelsToDraw);
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Exit"))
			{
				app->SetShouldClose(true);
			}
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}
	ImGui::EndDisabled();
}