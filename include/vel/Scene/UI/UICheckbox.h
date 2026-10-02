#pragma once

#include <vel/Scene/UI/UIElement.h>
#include <vel/Scene/UI/UITextButton.h>
#include <vel/Scene/Actor/Actor.h>

namespace vel
{
	class UICheckbox : public UIElement
	{
	private:
		friend class UIScene;


		bool				isChecked;
		int					fontSize;
		std::string			fontType;
		glm::vec4			color;
		glm::vec4			backgroundColor;

		UITextButton*		uncheckedButton;
		UITextButton*		checkedButton;

		void				toggleState();

	public:
		UICheckbox(const std::string& id);

		UICheckbox* setPosition(glm::vec2 pos) override;
		UICheckbox* setZIndex(float z) override;
		UICheckbox* setVisible(bool b) override;
		UICheckbox* setParent(actor_handle a) override;
		UICheckbox* setChecked(bool b);
		UICheckbox* setFontSize(int size);
		UICheckbox* setFontType(const std::string& t);
		UICheckbox* setColor(glm::vec4 c);
		UICheckbox* setBackgroundColor(glm::vec4 c);

		virtual bool	containsPoint(glm::vec2 p) override;
		virtual void	update(float dt, const vel::InputState* is, UICursor* c) override;

		virtual int		getWidth() const override;
		virtual int		getHeight() const override;
		bool			getIsChecked() const;
	};
}

