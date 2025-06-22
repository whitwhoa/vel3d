#pragma once

#include <string>

#include "vel/FontBitmap.h"
#include "vel/Actor.h"

namespace vel 
{
	enum class TextActorAlignment {
		LEFT_ALIGN,
		CENTER_ALIGN,
		RIGHT_ALIGN
	};

	enum class TextActorVerticalAlignment {
		BOTTOM_ALIGN,
		CENTER_ALIGN,
		TOP_ALIGN
	};

	struct TextActor 
	{
		std::string					name;
		std::string 				text;
		FontBitmap*					fontBitmap;
		TextActorAlignment			alignment;
		TextActorVerticalAlignment	vAlignment;
		Actor*						actor = nullptr;
		bool						requiresUpdate = false;

		void updateText(const std::string& updatedText)
		{
			this->text = updatedText;
			this->requiresUpdate = true;
		}
	};
}