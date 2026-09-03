#pragma once

#include <string>

#include <vel/Scene/Font/FontBitmap.h>
#include <vel/Scene/Actor/Actor.h>
#include <vel/Scene/Mesh/PlaneOrigin.h>

namespace vel 
{
	struct Text
	{
		std::string					name;
		std::string 				text;
		std::vector<glm::vec2>		caretPositions;
		FontBitmap*					fontBitmap;
		Actor*						actor = nullptr; //getWorldAABB() = exact visible geometry
		
		float						logicalWidth = 0.f; // stable layout width, including spaces
		float						logicalHeight = 0.f; // stable layout height
		PlaneOrigin					originType;
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