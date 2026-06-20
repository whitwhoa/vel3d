#pragma once

#include <string>

#include "vel/FontBitmap.h"
#include "vel/Actor.h"
#include "vel/PlaneOrigin.h"

namespace vel 
{
	struct TextActor 
	{
		std::string					name;
		std::string 				text;
		FontBitmap*					fontBitmap;
		PlaneOrigin					originType;
		Actor*						actor = nullptr;
		std::vector<glm::vec2>		caretPositions;
		bool						requiresUpdate = false;
		

		void updateText(const std::string& updatedText)
		{
			this->text = updatedText;
			this->requiresUpdate = true;
		}

		void addCharacter(int caretIndex, const char* c)
		{
			this->text.insert(this->text.begin() + caretIndex, *c);
			this->requiresUpdate = true;
		}

		void backspaceCharacter(int caretIndex)
		{
			if (caretIndex == 0 || this->text.empty())
				return;

			if (caretIndex > this->text.size())
				caretIndex = this->text.size();

			this->text.erase(caretIndex - 1, 1);
			this->caretPositions.erase(this->caretPositions.begin() + caretIndex);

			this->requiresUpdate = true;
		}

		void deleteCharacter(int caretIndex)
		{
			if (this->text.empty())
				return;

			if (caretIndex >= this->text.size())
				return;

			this->text.erase(caretIndex, 1);
			this->caretPositions.erase(this->caretPositions.begin() + caretIndex + 1);

			this->requiresUpdate = true;
		}
	};
}