#pragma once

#include "glm/glm.hpp"

#include <vel/Scene/Actor/Actor.h>
#include <vel/Scene/Text.h>
#include <vel/Scene/UI/UIButton.h>

namespace vel
{
	enum class TextButtonAlignment
	{
		LEFT,
		CENTER,
		RIGHT
	};

	class UITextButton : public UIButton
	{
	private:
		friend class UIScene;

		text_handle			textActor;

		std::string			text;
		int					fontSize;
		std::string			fontType;
		int					topPadding;
		int					bottomPadding;
		int					leftPadding;
		int					rightPadding;
		glm::vec4			textColor;
		glm::vec4			textColorHover;
		glm::vec4			textColorClick;
		glm::vec4			backgroundColor;
		glm::vec4			backgroundColorHover;
		glm::vec4			backgroundColorClick;
		TextButtonAlignment textAlignment;

		void				updateButtonActorSize();


	public:
		UITextButton(const std::string& id);

		UITextButton* setText(const std::string& t);
		UITextButton* setFontSize(int px);
		UITextButton* setFontType(const std::string& t);
		UITextButton* setTextColor(glm::vec4 c);
		UITextButton* setTextColorHover(glm::vec4 c);
		UITextButton* setTextColorClick(glm::vec4 c);
		UITextButton* setBackgroundColor(glm::vec4 c);
		UITextButton* setBackgroundColorHover(glm::vec4 c);
		UITextButton* setBackgroundColorClick(glm::vec4 c);
		UITextButton* setTopPadding(int p);
		UITextButton* setBottomPadding(int p);
		UITextButton* setLeftPadding(int p);
		UITextButton* setRightPadding(int p);
		UITextButton* setTextAlignment(TextButtonAlignment a = TextButtonAlignment::CENTER);
		virtual UITextButton* setMouseOverEvent(std::function<void()> f) override;
		virtual UITextButton* setMouseOutEvent(std::function<void()> f) override;
		virtual UITextButton* setMouseUpEvent(std::function<void()> f) override;
		virtual UITextButton* setMouseDownEvent(std::function<void()> f) override;
		virtual UITextButton* setPosition(glm::vec2 pos) override;
		virtual UITextButton* setZIndex(float z) override;
		virtual UITextButton* setVisible(bool b) override;
		virtual UITextButton* setParent(actor_handle a) override;
		virtual UITextButton* addBackgroundImage(const std::string& i, bool filter = true) override;
		virtual UITextButton* setHoldUntilMouseUp(bool b) override;
		virtual UITextButton* setWidth(int w);
		virtual UITextButton* setHeight(int h);


		const std::string& getText() const;
		int					getFontSize() const;
		const std::string& getFontType() const;
		int					getTopPadding() const;
		int					getBottomPadding() const;
		int					getLeftPadding() const;
		int					getRightPadding() const;
		glm::vec4			getTextColor() const;
		glm::vec4			getTextColorHover() const;
		glm::vec4			getTextColorClick() const;
		glm::vec4			getBackgroundColor() const;
		glm::vec4			getBackgroundColorHover() const;
		glm::vec4			getBackgroundColorClick() const;
		TextButtonAlignment getTextAlignment() const;

	};
}