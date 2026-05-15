#pragma once

#include <string>

#include "vel/FontBitmap.h"
#include "vel/Actor.h"

namespace vel 
{
	enum class TextActorOriginType
	{
		LEFT_BOTTOM,
		LEFT_CENTER,
		LEFT_TOP,
		CENTER_BOTTOM,
		CENTER_CENTER,
		CENTER_TOP,
		RIGHT_BOTTOM,
		RIGHT_CENTER,
		RIGHT_TOP
	};

	struct TextActor 
	{
		std::string					name;
		std::string 				text;
		FontBitmap*					fontBitmap;
		TextActorOriginType			originType;
		Actor*						actor = nullptr;
		bool						requiresUpdate = false;

		void updateText(const std::string& updatedText)
		{
			this->text = updatedText;
			this->requiresUpdate = true;
		}
	};
}