
#include <vel/Scene/Text.h>

namespace vel
{
	Text::Text() :
		text(""),
		fontBitmap(nullptr),
		logicalWidth(0.f),
		logicalHeight(0.f),
		originType(PlaneOrigin::LEFT_BOTTOM),
		requiresUpdate(false)
	{}

	void Text::updateText(const std::string& updatedText)
	{
		this->text = updatedText;
		this->requiresUpdate = true;
	}

	void Text::addCharacter(int caretIndex, const char* c)
	{
		this->text.insert(this->text.begin() + caretIndex, *c);
		this->requiresUpdate = true;
	}

	void Text::backspaceCharacter(int caretIndex)
	{
		if (caretIndex == 0 || this->text.empty())
			return;

		if (caretIndex > this->text.size())
			caretIndex = this->text.size();

		this->text.erase(caretIndex - 1, 1);
		this->caretPositions.erase(this->caretPositions.begin() + caretIndex);

		this->requiresUpdate = true;
	}

	void Text::deleteCharacter(int caretIndex)
	{
		if (this->text.empty())
			return;

		if (caretIndex >= this->text.size())
			return;

		this->text.erase(caretIndex, 1);
		this->caretPositions.erase(this->caretPositions.begin() + caretIndex + 1);

		this->requiresUpdate = true;
	}

}