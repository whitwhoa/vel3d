#pragma once

#include <string>
#include <memory>

#include <vel/Scene/Texture/Texture.h>

namespace vel
{
	// redefinition of `stbtt_packedchar` in stb_truetype.h
	// as we need this struct to be includable in other files
	// without having to include the stb_truetype header in all
	// of them, so we do this, then cast to stbtt_packedchar
	// where necessary
	typedef struct
	{
		unsigned short x0, y0, x1, y1;
		float xoff, yoff, xadvance;
		float xoff2, yoff2;
	} fb_packedchar;

	struct FontBitmap
	{
		std::string		fontName;
		std::string		fontPath;
		uint32_t		fontSize = 20;
		uint32_t		textureWidth = 512;
		uint32_t		textureHeight = 512;
		uint32_t		oversampleX = 2;
		uint32_t		oversampleY = 2;
		uint32_t		firstChar = ' ';
		uint32_t		charCount = '~' - ' ' + 1;

		float ascent = 0.0f;
		float descent = 0.0f;
		float lineGap = 0.0f;
		float fontHeight = 0.0f; // complete ascent-to-descent height
		float lineHeight = 0.0f; // distance from one baseline to the next

		//fb_packedchar* charInfo; // cast as stbtt_packedchar
		//unsigned char* data;

		//std::shared_ptr<unsigned char[]> data;
		//std::shared_ptr<fb_packedchar[]> charInfo;

		std::unique_ptr<unsigned char[]> data;
		std::unique_ptr<fb_packedchar[]> charInfo;

		Texture			texture;

		//~FontBitmap()
		//{
		//	delete this->data;
		//	delete this->charInfo;
		//}
	};

	
}
