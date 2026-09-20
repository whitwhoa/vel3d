#pragma once

#include <string>

#include <vel/Scene/Font/FontBitmap.h>
#include <vel/Scene/Actor/Actor.h>
#include <vel/Scene/Mesh/PlaneOrigin.h>
#include <vel/Util/slot_map.h>

namespace vel 
{
	typedef slot_handle text_handle;

	struct Text
	{
		std::string 				text;
		std::vector<glm::vec2>		caretPositions;
		FontBitmap*					fontBitmap;
		actor_handle				actor; //getWorldAABB() = exact visible geometry
		
		float						logicalWidth; // stable layout width, including spaces
		float						logicalHeight; // stable layout height
		PlaneOrigin					originType;
		bool						requiresUpdate;
		
		Text();

		void updateText(const std::string& updatedText);
		void addCharacter(int caretIndex, const char* c);
		void backspaceCharacter(int caretIndex);
		void deleteCharacter(int caretIndex);
	};
}