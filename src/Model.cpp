#include "Model.h"
#include <fstream>
#include <string>
#include <sstream>

Model::Model(const std::string filename)
{
	std::ifstream in;
	in.open(filename, std::ifstream::in);
	if (in.fail())
	{
		std::cerr << "Error: Could not load model at filepath: " << filename << std::endl;
		return;
	}
	std::string line;
	while (!in.eof())
	{
		std::getline(in, line);
		std::istringstream iss(line.c_str());
		char trash;
		if (!line.compare(0, 2, "v "))
		{
			iss >> trash;
			vec4 v = { 0, 0, 0, 1 };
			for (int i : {0, 1, 2})
				iss >> v[i];
			verts.push_back(v);
		}
		else if (!line.compare(0, 3, "vn "))
		{
			iss >> trash >> trash;
			vec4 n;
			for (int i : {0, 1, 2})
				iss >> n[i];
			norms.push_back(normalized(n));
		}
		else if (!line.compare(0, 3, "vt "))
		{
			iss >> trash >> trash;
			vec2 uv;
			for (int i : {0, 1})
				iss >> uv[i];
			tex.push_back({ uv.x, 1 - uv.y });
		}
		else if (!line.compare(0, 2, "f "))
		{
			int f, t, n, cnt = 0;
			iss >> trash;
			while (iss >> f >> trash >> t >> trash >> n)
			{
				facet_vrt.push_back(--f);
				facet_tex.push_back(--t);
				facet_nrm.push_back(--n);
				cnt++;
			}
			if (cnt != 3)
			{
				std::cerr << "Error: the obj file is supposed to be triangulated" << std::endl;
				return;
			}
		}
	}
	std::cerr << "# v# " << nverts() << " f# " << nfaces() << std::endl;
	int extLocation = filename.find_last_of(".");
	std::string normalPath = filename.substr(0, extLocation) + "_nm_tangent.tga";
	normalmap = Image(normalPath.c_str());
	std::string diffusePath = filename.substr(0, extLocation) + "_diffuse.tga";
	diffusemap = Image(diffusePath.c_str());
	std::string specularPath = filename.substr(0, extLocation) + "_spec.tga";
	specularmap = Image(specularPath.c_str());
}

int Model::nverts() const
{
	return verts.size();
}

int Model::nfaces() const
{
	return facet_vrt.size() / 3;
}

vec4 Model::vert(const int i) const
{
	return verts[i];
}

vec4 Model::vert(const int iface, const int nthvert) const
{
	return verts[facet_vrt[iface * 3 + nthvert]];
}

vec4 Model::normal(const int iface, const int nthvert) const
{
	return norms[facet_nrm[iface * 3 + nthvert]];
}

vec4 Model::normal(const vec2& uv) const
{
	Color c = normalmap.GetColor(uv[0] * normalmap.Width(), uv[1] * normalmap.Height());
	return normalized(vec4{ (double)c[2], (double)c[1], (double)c[0], 0 } * 2.0 / 255.0 - vec4{ 1, 1, 1, 0 });
}

vec2 Model::uv(const int iface, const int nthvert) const
{
	return tex[facet_tex[iface * 3 + nthvert]];
}

Color Model::diffuse(const vec2& uv) const
{
	Color c = diffusemap.GetColor(uv[0] * diffusemap.Width(), uv[1] * diffusemap.Height());
	return c;
}

Color Model::specular(const vec2& uv) const
{
	Color c = specularmap.GetColor(uv[0] * specularmap.Width(), uv[1] * specularmap.Height());
	return c;
}