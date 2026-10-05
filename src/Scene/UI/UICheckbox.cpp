
#include "spdlog/spdlog.h"

#include <vel/Scene/UI/UICheckbox.h>

namespace vel
{
	UICheckbox::UICheckbox(const std::string& id) :
		UIElement(id),
		isChecked(false),
		fontSize(10),
		fontType("Arial"),
		color({ 1.f, 1.f, 1.f, 1.f }),
		backgroundColor({ 0.f, 0.f, 0.f, 0.f }),

		uncheckedButton(nullptr),
		checkedButton(nullptr)
	{}

	int UICheckbox::getWidth() const
	{
		return this->checkedButton->getWidth();
	}

	int UICheckbox::getHeight() const
	{
		return this->checkedButton->getHeight();
	}

	void UICheckbox::toggleState()
	{
		if (!this->isChecked)
		{
			this->isChecked = true;
			this->checkedButton->setVisible(true);
			this->uncheckedButton->setVisible(false);
		}
		else
		{
			this->isChecked = false;
			this->uncheckedButton->setVisible(true);
			this->checkedButton->setVisible(false);
		}
	}

	UICheckbox* UICheckbox::setPosition(glm::vec2 pos)
	{
		UIElement::setPosition(pos);

		if (!this->isInitialized)
			return this;

		this->checkedButton->setPosition(pos);
		this->uncheckedButton->setPosition(pos);

		return this;
	}

	UICheckbox* UICheckbox::setZIndex(float z)
	{
		UIElement::setZIndex(z);
		return this;
	}

	UICheckbox* UICheckbox::setVisible(bool b)
	{
		UIElement::setVisible(b);

		if (!this->isInitialized)
			return this;

		if (this->isChecked)
			this->checkedButton->setVisible(b);
		else
			this->uncheckedButton->setVisible(b);

		return this;
	}

	UICheckbox* UICheckbox::setParent(actor_handle a)
	{
		if (!a)
			return this;

		UIElement::setParent(a);

		if (!this->isInitialized)
			return this;

		this->uncheckedButton->setParent(a);
		this->checkedButton->setParent(a);

		return this;
	}

	UICheckbox* UICheckbox::setChecked(bool b)
	{
		this->isChecked = b;
		return this;
	}

	UICheckbox* UICheckbox::setFontSize(int size)
	{
		this->fontSize = size;
		return this;
	}

	UICheckbox* UICheckbox::setFontType(const std::string& t)
	{
		this->fontType = t;
		return this;
	}

	UICheckbox* UICheckbox::setColor(glm::vec4 c)
	{
		this->color = c;
		return this;
	}

	UICheckbox* UICheckbox::setBackgroundColor(glm::vec4 c)
	{
		this->backgroundColor = c;
		return this;
	}

	bool UICheckbox::getIsChecked() const
	{
		return this->isChecked;
	}

	bool UICheckbox::containsPoint(glm::vec2 p)
	{
		if (!this->getIsInitialized())
			return false;

		if (!this->isVisible)
			return false;

		if (!this->uncheckedButton->containsPoint(p) && !this->checkedButton->containsPoint(p))
			return false;

		return true;
	}

	void UICheckbox::update(float dt, const vel::InputState& is, UICursor* c)
	{}
}