#pragma once
#include <vector>
#include "Framebuffer.h"
#include "Geometry.h"
#include "Image.h"

class Model
{
public:
	Model(const std::string filename);
	int nverts() const;
	int nfaces() const;
	vec4 vert(const int i) const;
	vec4 vert(const int iface, const int nthvert) const;
	vec4 normal(const int iface, const int nthvert) const;
	vec4 normal(const vec2& uv) const;
	vec2 uv(const int iface, const int nthvert) const;
	Color diffuse(const vec2& uv) const;
	Color specular(const vec2& uv) const;
private:
	std::vector<vec4> verts = {};
	std::vector<vec4> norms = {};
	std::vector<vec2> tex = {};
	std::vector<int> facet_vrt = {};
	std::vector<int> facet_nrm = {};
	std::vector<int> facet_tex = {};
	Image normalmap;
	Image diffusemap;
	Image specularmap;
};