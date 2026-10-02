#pragma once

#include <optional>


#include <vel/InputState.h>

#include <vel/Scene/UI/UIButton.h>

namespace vel
{
	class UITextInput : public UIButton
	{
	private:
		friend class UIScene;

		std::string			fontType;
		std::string			defaultText;
		glm::vec4			textColor;
		glm::vec4			backgroundColor;
		glm::vec4			caretColor;
		glm::vec4			cursorColor;
		text_handle			ghostTextActor;
		text_handle			textActor;
		actor_handle		caretActor;
		float				lastCaretFlashTime;
		float				currentTime;
		float				caretFlashInterval;

		float				keyDelay;
		float				keyRepeatSpeed;
		float				keyNextTime;

		int					fontSize;

		int					viewWidth;
		int					viewStartX;
		int					viewEndX;
		int					caretIndex;

		int					topPadding;
		int					bottomPadding;
		int					defaultBottomPadding;
		int					leftPadding;
		int					defaultLeftPadding;
		int					rightPadding;
		bool				hasFocus;
		bool				isCapsLock;
		bool				wasBackspace;
		bool				wasDelete;
		bool				wasKeyPressed;


		int					findViewStart(int i);
		int					findViewEnd(int i);

		void				moveCaretRight();
		void				moveCaretLeft();

		void				backspaceChar();
		void				deleteChar();

		void				addChar(const char* c);

		void				checkBackspace();
		void				checkDelete();
		void				checkKeyPress();

		bool				canKey();
		bool				shouldTypeKey(const InputState* is, VEL_KEY key);

		void				updateView();
		void				updateCaretActorPos();
		void				updateKeys(const InputState* is);


	public:
		UITextInput(const std::string& id);

		UITextInput* setFontType(const std::string& t);
		UITextInput* setDefaultText(const std::string& t);
		UITextInput* setTextColor(glm::vec4 c);
		UITextInput* setBackgroundColor(glm::vec4 c);
		UITextInput* setCaretColor(glm::vec4 c);
		UITextInput* setCursorColor(glm::vec4 c);
		UITextInput* setFontSize(int px);
		virtual UITextInput* setWidth(int w) override;
		UITextInput* setTopPadding(int p);
		UITextInput* setBottomPadding(int p);
		UITextInput* setLeftPadding(int p);
		UITextInput* setRightPadding(int p);


		UITextInput* setPosition(glm::vec2 pos) override;
		UITextInput* setZIndex(float z) override;
		UITextInput* setVisible(bool b) override;
		UITextInput* setParent(actor_handle a) override;

		virtual void			update(float dt, const InputState* is, UICursor* c) override;

		const std::string& getText();
	};
}