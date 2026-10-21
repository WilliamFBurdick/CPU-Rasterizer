#include "Framebuffer.h"
#include <cassert>
#include <string.h>

Framebuffer::Framebuffer(SDL_Renderer* renderer, int width, int height, int bpp, Color clearColor)
{
	m_Texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, width, height);
	SDL_SetTextureScaleMode(m_Texture, SDL_SCALEMODE_NEAREST);
	m_PixelBuffer = new uint32_t[width * height];
	m_Width = width;
	m_Height = height;
	m_Pitch = width * SDL_BYTESPERPIXEL(SDL_PIXELFORMAT_RGBA8888);
	m_BPP = bpp;
	Clear(clearColor);
}

Framebuffer::~Framebuffer()
{
	SDL_DestroyTexture(m_Texture);
	delete m_PixelBuffer;
	m_PixelBuffer = nullptr;
}

void Framebuffer::Clear(Color clearColor)
{
	auto pixelFormat = SDL_GetPixelFormatDetails(m_Texture->format);
	uint32_t pixelColor = SDL_MapRGBA(pixelFormat, NULL, clearColor.rgba[0], clearColor.rgba[1], clearColor.rgba[2], clearColor.rgba[3]);
	for (int i = 0; i < (m_Width * m_Height); i++)
		m_PixelBuffer[i] = pixelColor;
}

void Framebuffer::Render(SDL_Renderer* renderer) const
{
	SDL_UpdateTexture(m_Texture, NULL, m_PixelBuffer, m_Pitch);

	SDL_RenderTextureRotated(renderer, m_Texture, NULL, NULL, 0.0, NULL, SDL_FLIP_VERTICAL);
}

void Framebuffer::Set(int x, int y, Color color)
{
	auto pixelFormat = SDL_GetPixelFormatDetails(m_Texture->format);
	uint32_t pixelColor;
	if (m_BPP == 4)
		pixelColor = SDL_MapRGBA(pixelFormat, NULL, color.rgba[0], color.rgba[1], color.rgba[2], color.rgba[3]);
	else if (m_BPP == 3)
		pixelColor = SDL_MapRGBA(pixelFormat, NULL, color.rgba[0], color.rgba[1], color.rgba[2], 0xFF);
	else if (m_BPP == 1)
		pixelColor = SDL_MapRGBA(pixelFormat, NULL, color.rgba[0], color.rgba[0], color.rgba[0], 0xFF);
	else
		pixelColor = SDL_MapRGBA(pixelFormat, NULL, 0xFF, 0xFF, 0xFF, 0xFF);
	m_PixelBuffer[m_Width * y + x] = pixelColor;
}

uint32_t Framebuffer::Get(int x, int y)
{
	assert(x >= 0 && x <= m_Width && y >= 0 && y <= m_Height);
	return m_PixelBuffer[m_Width * y + x];
}
