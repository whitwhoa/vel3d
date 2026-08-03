#pragma once


#include <vel/Scene/Stage/Actor/Material/Texture/ImageData.h>


namespace vel
{
	struct TextureData
	{
		unsigned int				id; // opengl buffer object id 
		uint64_t					dsaHandle;
		ImageData					primaryImageData;
		bool						alphaChannel;
	};
}