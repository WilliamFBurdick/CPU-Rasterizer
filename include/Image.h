#pragma once
#include "Framebuffer.h"
#include <vector>

class Image
{
public:
	Image();
	Image(const char* filepath);
	~Image();

	int Width() const { return m_Width; }
	int Height() const { return m_Height; }
	int Channels() const { return m_Channels; }

	uint32_t Get(int x, int y) const;
	Color GetColor(int x, int y) const;

private:
	int m_Width, m_Height, m_Channels;
	std::vector<uint32_t> m_Pixels;
};