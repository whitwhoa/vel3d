
#include <iostream>

#include <vel/Scene/Scene.h>
#include <vel/Scene/UI/UIView.h>
#include <vel/Scene/UI/UITextInput.h>

namespace vel
{
	UITextInput::UITextInput(const std::string& id) :
		UIButton(id),
		fontType("Arial"),
		defaultText(""),
		textColor({ 0.f, 0.f, 0.f, 1.f }),
		backgroundColor({ 1.f, 1.f, 1.f, 1.f }),
		caretColor({ 0.f, 0.f, 0.f, 1.f }),
		cursorColor({ 0.f, 0.f, 0.f, 1.f }),
		lastCaretFlashTime(0.f),
		currentTime(0.f),
		caretFlashInterval(0.53f),
		keyDelay(0.75f),
		keyRepeatSpeed(0.033f),
		keyNextTime(0.f),
		fontSize(12),
		viewWidth(0),
		viewStartX(0),
		viewEndX(0),
		caretIndex(0),
		topPadding(0),
		bottomPadding(0),
		defaultBottomPadding(1),
		leftPadding(0),
		defaultLeftPadding(3),
		rightPadding(0),
		hasFocus(false),
		isCapsLock(false),
		wasBackspace(false),
		wasDelete(false),
		wasKeyPressed(false)
	{

	}

	const std::string& UITextInput::getText()
	{
		return this->parentView->scene->getText(this->ghostTextActor).text;
	}

	UITextInput* UITextInput::setTopPadding(int p)
	{
		this->topPadding = p;
		return this;
	}

	UITextInput* UITextInput::setBottomPadding(int p)
	{
		this->bottomPadding = p;
		return this;
	}

	UITextInput* UITextInput::setLeftPadding(int p)
	{
		this->leftPadding = p;
		return this;
	}

	UITextInput* UITextInput::setRightPadding(int p)
	{
		this->rightPadding = p;
		return this;
	}

	UITextInput* UITextInput::setFontSize(int px)
	{
		this->fontSize = px;
		return this;
	}

	UITextInput* UITextInput::setBackgroundColor(glm::vec4 c)
	{
		this->backgroundColor = c;
		return this;
	}

	UITextInput* UITextInput::setCaretColor(glm::vec4 c)
	{
		this->caretColor = c;
		return this;
	}

	UITextInput* UITextInput::setCursorColor(glm::vec4 c)
	{
		this->cursorColor = c;
		return this;
	}

	UITextInput* UITextInput::setTextColor(glm::vec4 c)
	{
		this->textColor = c;
		return this;
	}

	UITextInput* UITextInput::setFontType(const std::string& t)
	{
		this->fontType = t;
		return this;
	}

	UITextInput* UITextInput::setDefaultText(const std::string& t)
	{
		this->defaultText = t;
		return this;
	}

	UITextInput* UITextInput::setWidth(int w)
	{
		UIButton::setWidth(w);
		return this;
	}

	void UITextInput::updateCaretActorPos()
	{
		Scene* s = this->parentView->scene;

		Text& gt = s->getText(this->ghostTextActor);
		Actor& gta = s->getActor(gt.actor);

		glm::vec3 caretPos = gta.getTranslation();

		if (gt.caretPositions.size() > 0)
		{
			caretPos.x += gt.caretPositions.at(this->caretIndex).x - this->viewStartX;
		}

		Actor& ba = s->getActor(this->buttonActor);

		caretPos.y = ba.getTranslation().y;

		Actor& ca = s->getActor(this->caretActor);

		ca.setTranslation(caretPos);
	}

	UITextInput* UITextInput::setPosition(glm::vec2 pos)
	{
		UIElement::setPosition(pos);

		if (!this->isInitialized)
			return this;

		float txtPosX = this->positionCache.x - (this->getWidth() * 0.5f) + this->leftPadding + this->defaultLeftPadding;
		float txtPosY = this->positionCache.y + (this->getHeight() * 0.5f) - this->bottomPadding - this->defaultBottomPadding;
		glm::vec3 textPos = glm::vec3(txtPosX, txtPosY, this->zIndex + 0.001f);

		Scene* s = this->parentView->scene;

		Actor& gta = s->getTextActor(this->ghostTextActor);
		Actor& ta = s->getTextActor(this->textActor);
		Actor& ba = s->getActor(this->buttonActor);

		gta.setTranslation(textPos);
		ta.setTranslation(textPos);
		ba.setTranslation(glm::vec3(this->positionCache, this->zIndex));

		this->updateCaretActorPos();

		return this;
	}

	UITextInput* UITextInput::setZIndex(float z)
	{
		UIElement::setZIndex(z);
		return this;
	}

	UITextInput* UITextInput::setVisible(bool b)
	{
		UIElement::setVisible(b);

		if (!this->isInitialized)
			return this;

		Scene* s = this->parentView->scene;
		
		Actor& ba = s->getActor(this->buttonActor);
		Actor& ta = s->getTextActor(this->textActor);

		ba.flags = (ba.flags & ~ACTFLG_VISIBLE) | (b ? ACTFLG_VISIBLE : 0);
		ta.flags = (ta.flags & ~ACTFLG_VISIBLE) | (b ? ACTFLG_VISIBLE : 0);

		return this;
	}

	UITextInput* UITextInput::setParent(actor_handle a)
	{
		if (!a)
			return this;

		UIElement::setParent(a);

		if (!this->isInitialized)
			return this;

		Scene* s = this->parentView->scene;

		s->setActorParent(s->getText(this->ghostTextActor).actor, a);
		s->setActorParent(s->getText(this->textActor).actor, a);
		s->setActorParent(this->caretActor, a);
		s->setActorParent(this->buttonActor, a);

		return this;
	}

	int UITextInput::findViewStart(int i)
	{
		// To find viewStartX, start at provided index i and walk backward until we find an index that has
		// a position that is outside the bounds of the view. Use the index of the previous loop
		// to locate the value for viewStartX
		int bounds = this->viewEndX - this->viewWidth;

		if (bounds == 0)
			return 0;

		while (true)
		{
			if (i == -1)
				return 0;

			Text& gt = this->parentView->scene->getText(this->ghostTextActor);

			int caretPos = std::round(gt.caretPositions.at(i).x);
			if (caretPos < bounds)
			{
				i++;
				break;
			}
			i--;
		}
		return i;
	}

	int UITextInput::findViewEnd(int i)
	{
		// To find viewEndX, start at provided index i and walk forward until we find an index that
		// has a position that is outside the bounds of the view. Use the index of the previous loop
		// to locate the value for viewEndX
		int bounds = this->viewStartX + this->viewWidth;

		Text& gt = this->parentView->scene->getText(this->ghostTextActor);

		if (bounds == std::round(gt.caretPositions.at(i).x))
			return i;

		while (true)
		{
			if (i == gt.caretPositions.size())
				return i - 1;

			int caretPos = std::round(gt.caretPositions.at(i).x);
			if (caretPos > bounds)
			{
				i--;
				break;
			}
			i++;
		}
		return i;
	}

	void UITextInput::moveCaretRight()
	{
		Text& gt = this->parentView->scene->getText(this->ghostTextActor);

		// Check if caretIndex is already as far right as it can go. If so, leave
		if (this->caretIndex == gt.caretPositions.size() - 1)
			return;

		this->caretIndex++;

		// Check if we need to shift the view to the right. If not, leave
		if (std::round(gt.caretPositions.at(this->caretIndex).x) <= this->viewEndX)
			return;

		this->viewEndX = std::round(gt.caretPositions.at(this->caretIndex).x);
		this->viewStartX = std::round(gt.caretPositions.at(this->findViewStart(this->caretIndex)).x);
	}

	void UITextInput::moveCaretLeft()
	{
		// Check if caretIndex is already as far left as it can go
		if (this->caretIndex == 0)
			return;

		this->caretIndex--;

		Text& gt = this->parentView->scene->getText(this->ghostTextActor);

		// Check if we need to shift the view to the left
		if (std::round(gt.caretPositions.at(this->caretIndex).x) >= this->viewStartX)
			return;

		this->viewStartX = std::round(gt.caretPositions.at(this->caretIndex).x);
		this->viewEndX = std::round(gt.caretPositions.at(this->findViewEnd(this->caretIndex)).x);
	}

	void UITextInput::backspaceChar()
	{
		if (this->caretIndex == 0)
			return;

		Text& gt = this->parentView->scene->getText(this->ghostTextActor);

		gt.backspaceCharacter(this->caretIndex);

		this->caretIndex--;

		this->wasBackspace = true;
	}

	void UITextInput::deleteChar()
	{
		Text& gt = this->parentView->scene->getText(this->ghostTextActor);

		if (this->caretIndex == gt.caretPositions.size() - 1)
			return;

		gt.deleteCharacter(this->caretIndex);

		this->wasDelete = true;
	}

	void UITextInput::addChar(const char* c)
	{
		Text& gt = this->parentView->scene->getText(this->ghostTextActor);

		gt.addCharacter(this->caretIndex, c);
		this->caretIndex++;
		this->wasKeyPressed = true;
	}

	void UITextInput::checkBackspace()
	{
		if (!this->wasBackspace)
			return;

		this->wasBackspace = false;

		Text& gt = this->parentView->scene->getText(this->ghostTextActor);

		// we have characters outside our view to the left, but not outside our view to the right
		// and we should update viewEndX to current last .x and then shift the view to the left by one
		if (this->viewEndX > std::round(gt.caretPositions.back().x))
		{
			this->viewEndX = std::round(gt.caretPositions.back().x);
			this->viewStartX = std::round(gt.caretPositions.at(this->findViewStart(this->caretIndex)).x);
			return;
		}

		// we have removed a character with a position before viewStartX, so we need to shift viewStartX such that
		// the view remains relative to the text that it encompasses
		if (this->viewStartX > std::round(gt.caretPositions.at(this->caretIndex).x))
		{
			this->viewStartX = std::round(gt.caretPositions.at(this->caretIndex).x);
			this->viewEndX = std::round(gt.caretPositions.at(this->findViewEnd(this->caretIndex)).x);
			return;
		}

		// removal was of a character within the range of the view. set viewEndX equal to last caret position
		// within view range
		this->viewEndX = std::round(gt.caretPositions.at(this->findViewEnd(this->caretIndex)).x);
	}

	void UITextInput::checkDelete()
	{
		if (!this->wasDelete)
			return;

		this->wasDelete = false;

		Text& gt = this->parentView->scene->getText(this->ghostTextActor);

		if (this->viewEndX > std::round(gt.caretPositions.back().x))
		{
			this->viewEndX = std::round(gt.caretPositions.back().x);
			this->viewStartX = std::round(gt.caretPositions.at(this->findViewStart(this->caretIndex)).x);
			return;
		}

		this->viewEndX = std::round(gt.caretPositions.at(this->findViewEnd(this->caretIndex)).x);
	}

	void UITextInput::checkKeyPress()
	{
		if (!this->wasKeyPressed)
			return;

		this->wasKeyPressed = false;

		Text& gt = this->parentView->scene->getText(this->ghostTextActor);

		if (std::round(gt.caretPositions.at(this->caretIndex).x) > this->viewEndX)
		{
			this->viewEndX = std::round(gt.caretPositions.at(this->caretIndex).x);
			this->viewStartX = std::round(gt.caretPositions.at(this->findViewStart(this->caretIndex)).x);
		}
	}

	void UITextInput::updateView()
	{
		this->checkBackspace();
		this->checkDelete();
		this->checkKeyPress();

		int startIndex = -1;
		int endIndex = -1;
		Text& gt = this->parentView->scene->getText(this->ghostTextActor);
		if (gt.caretPositions.size() == 1)
		{
			startIndex = 0;
			endIndex = 0;
		}
		else
		{
			for (int i = 0; i < gt.caretPositions.size(); i++)
			{
				if (startIndex == -1)
				{
					if (std::round(gt.caretPositions.at(i).x) == this->viewStartX)
						startIndex = i;
				}
				else
				{
					if (std::round(gt.caretPositions.at(i).x) == this->viewEndX)
					{
						endIndex = i;
						break;
					}
				}
			}
		}

		//SPDLOG_TRACE("startIndex: {} | endIndex: {}", startIndex, endIndex);
		Text& t = this->parentView->scene->getText(this->textActor);
		t.updateText(gt.text.substr(startIndex, endIndex - startIndex));
	}

	bool UITextInput::canKey()
	{
		if (this->keyNextTime == 0.f) // yeah...but we set it explicitly
		{
			this->keyNextTime = this->currentTime + this->keyDelay;
			return true;
		}

		if (this->keyNextTime <= this->currentTime)
		{
			this->keyNextTime = this->currentTime + this->keyRepeatSpeed;
			return true;
		}

		return false;
	}

	bool UITextInput::shouldTypeKey(const InputState& is, VEL_KEY key)
	{
		if (is.pressed(key))
		{
			this->keyNextTime = this->currentTime + this->keyDelay;
			return true;
		}

		if (is.held(key) && this->canKey())
			return true;

		return false;
	}

	void UITextInput::updateKeys(const InputState& is)
	{
		// Navigation
		if (this->shouldTypeKey(is, VEL_KEY_RIGHT))
		{
			this->moveCaretRight();
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_LEFT))
		{
			this->moveCaretLeft();
			return;
		}


		// Removal
		if (this->shouldTypeKey(is, VEL_KEY_BACKSPACE))
		{
			this->backspaceChar();
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_DELETE))
		{
			this->deleteChar();
			return;
		}


		// Shift State
		bool shifted = is.down(VEL_KEY_LEFT_SHIFT) ||
			is.down(VEL_KEY_RIGHT_SHIFT);

		if (is.pressed(VEL_KEY_CAPS_LOCK))
			this->isCapsLock = !this->isCapsLock;

		bool capitalized = shifted != this->isCapsLock;


		// Keys
		if (this->shouldTypeKey(is, VEL_KEY_SPACE))
		{
			this->addChar(" ");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_APOSTROPHE))
		{
			this->addChar(shifted ? "\"" : "'");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_COMMA))
		{
			this->addChar(shifted ? "<" : ",");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_MINUS))
		{
			this->addChar(shifted ? "_" : "-");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_PERIOD))
		{
			this->addChar(shifted ? ">" : ".");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_SLASH))
		{
			this->addChar(shifted ? "?" : "/");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_0))
		{
			this->addChar(shifted ? ")" : "0");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_1))
		{
			this->addChar(shifted ? "!" : "1");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_2))
		{
			this->addChar(shifted ? "@" : "2");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_3))
		{
			this->addChar(shifted ? "#" : "3");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_4))
		{
			this->addChar(shifted ? "$" : "4");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_5))
		{
			this->addChar(shifted ? "%" : "5");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_6))
		{
			this->addChar(shifted ? "^" : "6");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_7))
		{
			this->addChar(shifted ? "&" : "7");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_8))
		{
			this->addChar(shifted ? "*" : "8");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_9))
		{
			this->addChar(shifted ? "(" : "9");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_SEMICOLON))
		{
			this->addChar(shifted ? ":" : ";");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_EQUAL))
		{
			this->addChar(shifted ? "+" : "=");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_A))
		{
			this->addChar(capitalized ? "A" : "a");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_B))
		{
			this->addChar(capitalized ? "B" : "b");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_C))
		{
			this->addChar(capitalized ? "C" : "c");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_D))
		{
			this->addChar(capitalized ? "D" : "d");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_E))
		{
			this->addChar(capitalized ? "E" : "e");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_F))
		{
			this->addChar(capitalized ? "F" : "f");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_G))
		{
			this->addChar(capitalized ? "G" : "g");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_H))
		{
			this->addChar(capitalized ? "H" : "h");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_I))
		{
			this->addChar(capitalized ? "I" : "i");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_J))
		{
			this->addChar(capitalized ? "J" : "j");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_K))
		{
			this->addChar(capitalized ? "K" : "k");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_L))
		{
			this->addChar(capitalized ? "L" : "l");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_M))
		{
			this->addChar(capitalized ? "M" : "m");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_N))
		{
			this->addChar(capitalized ? "N" : "n");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_O))
		{
			this->addChar(capitalized ? "O" : "o");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_P))
		{
			this->addChar(capitalized ? "P" : "p");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_Q))
		{
			this->addChar(capitalized ? "Q" : "q");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_R))
		{
			this->addChar(capitalized ? "R" : "r");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_S))
		{
			this->addChar(capitalized ? "S" : "s");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_T))
		{
			this->addChar(capitalized ? "T" : "t");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_U))
		{
			this->addChar(capitalized ? "U" : "u");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_V))
		{
			this->addChar(capitalized ? "V" : "v");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_W))
		{
			this->addChar(capitalized ? "W" : "w");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_X))
		{
			this->addChar(capitalized ? "X" : "x");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_Y))
		{
			this->addChar(capitalized ? "Y" : "y");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_Z))
		{
			this->addChar(capitalized ? "Z" : "z");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_LEFT_BRACKET))
		{
			this->addChar(shifted ? "{" : "[");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_RIGHT_BRACKET))
		{
			this->addChar(shifted ? "}" : "]");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_BACKSLASH))
		{
			this->addChar(shifted ? "|" : "\\");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_GRAVE_ACCENT))
		{
			this->addChar(shifted ? "~" : "`");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_KP_0))
		{
			this->addChar("0");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_KP_1))
		{
			this->addChar("1");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_KP_2))
		{
			this->addChar("2");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_KP_3))
		{
			this->addChar("3");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_KP_4))
		{
			this->addChar("4");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_KP_5))
		{
			this->addChar("5");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_KP_6))
		{
			this->addChar("6");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_KP_7))
		{
			this->addChar("7");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_KP_8))
		{
			this->addChar("8");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_KP_9))
		{
			this->addChar("9");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_KP_DECIMAL))
		{
			this->addChar(".");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_KP_DIVIDE))
		{
			this->addChar("/");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_KP_MULTIPLY))
		{
			this->addChar("*");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_KP_SUBTRACT))
		{
			this->addChar("-");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_KP_ADD))
		{
			this->addChar("+");
			return;
		}

		if (this->shouldTypeKey(is, VEL_KEY_KP_EQUAL))
		{
			this->addChar("=");
			return;
		}
	}

	void UITextInput::update(float dt, const InputState& is, UICursor* c)
	{
		UIButton::update(dt, is, c);

		this->currentTime += dt;

		if (!this->hasFocus)
			return;

		// unfocus if mouse click outside button
		if (!this->getIsMouseOver() && is.mouseLeftButton)
		{
			this->hasFocus = false;

			Actor& ca = this->parentView->scene->getActor(this->caretActor);
			ca.flags &= ~ACTFLG_VISIBLE;

			Text& gt = this->parentView->scene->getText(this->ghostTextActor);
			Text& t = this->parentView->scene->getText(this->textActor);

			if (gt.text == "" && this->defaultText != "")
			{
				this->caretIndex = 0;
				gt.updateText(this->defaultText);
				t.updateText(this->defaultText);
			}

			return;
		}

		// update caret flash
		if (this->currentTime - this->lastCaretFlashTime >= this->caretFlashInterval)
		{
			Actor& ca = this->parentView->scene->getActor(this->caretActor);

			if (ca.flags & ACTFLG_VISIBLE)
				ca.flags &= ~ACTFLG_VISIBLE;
			else
				ca.flags |= ACTFLG_VISIBLE;

			this->lastCaretFlashTime = this->currentTime;
		}

		this->updateView();
		this->updateCaretActorPos();
		this->updateKeys(is);
	}
}