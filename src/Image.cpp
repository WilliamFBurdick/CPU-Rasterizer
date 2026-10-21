#include "Image.h"
#include "stb_image.h"

Image::Image()
{}

Image::Image(const char* filepath)
{
	unsigned char* data = stbi_load(filepath, &m_Width, &m_Height, &m_Channels, 4);
	m_Pixels = std::vector<uint32_t>();
	for (int i = 0; i < m_Width * m_Height * 4; i += 4)
	{
		uint32_t value = 0xFFFFFF;
		value = SDL_MapRGBA(SDL_GetPixelFormatDetails(SDL_PIXELFORMAT_RGBA8888), NULL, data[i], data[i + 1], data[i + 2], data[i + 3]);
		m_Pixels.push_back(value);
	}
}

Image::~Image()
{
	
}

uint32_t Image::Get(int x, int y) const
{
	return m_Pixels[y * m_Width + x];
}

Color Image::GetColor(int x, int y) const
{
	return Color::Convert<uint32_t>(Get(x, y));
}
