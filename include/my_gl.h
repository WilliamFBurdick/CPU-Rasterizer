#pragma once
#include "Framebuffer.h"
#include "Geometry.h"
#include "Image.h"

void lookat(const vec3 eye, const vec3 center, const vec3 up);
void init_perspective(const double f);
void init_viewport(const int x, const int y, const int w, const int h);
void init_zbuffer(const int width, const int height);

struct IShader
{
	static Color sample2D(const Image& img, const vec2& uvf)
	{
		return img.GetColor(uvf[0] * img.Width(), uvf[1] * img.Height());
	}
	virtual std::pair<bool, Color> fragment(const vec3 bar) const = 0;
};

typedef vec4 Triangle[3];
void rasterize(const Triangle& clip, const IShader& shader, Framebuffer& framebuffer);