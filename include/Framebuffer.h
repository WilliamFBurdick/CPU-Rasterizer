#pragma once
#include <SDL3/SDL.h>
#include <cstring>

struct Color
{
	uint8_t rgba[4] = {0, 0, 0, 0};
	uint8_t& operator[](const int i) { return rgba[i]; }

	template<typename T>
	static Color Convert(T input)
	{
		return { 0, 0, 0, 255 };
	}

	template<>
	static Color Convert<uint32_t>(uint32_t input)
	{
		Color retval;
		SDL_GetRGBA(input, SDL_GetPixelFormatDetails(SDL_PIXELFORMAT_RGBA8888), NULL, &retval.rgba[0], &retval.rgba[1], &retval.rgba[2], &retval.rgba[3]);
		return retval;
	}
};

static Color White = { 255, 255, 255, 255 };
static Color Green = { 0, 255, 0, 255 };
static Color Blue = { 64, 128, 255, 255 };
static Color Red = { 255, 0, 0, 255 };
static Color Yellow = { 255, 255, 0, 255 };
static Color Black = { 0, 0, 0, 255 };

class Framebuffer
{
public:
	enum Format {RGBA = 4, RGB = 3, GREYSCALE = 1};
	Framebuffer(SDL_Renderer* renderer, int width, int height, int bpp = 4, Color clearColor = Black);
	~Framebuffer();

	void Clear(Color clearColor = Black);
	void Render(SDL_Renderer* renderer) const;
	void Set(int x, int y, Color color);
	uint32_t Get(int x, int y);

	int Width() const { return m_Width; }
	int Height() const { return m_Height; }

private:
	SDL_Texture* m_Texture;
	uint32_t* m_PixelBuffer;
	int m_Width, m_Height, m_Pitch, m_BPP;
};