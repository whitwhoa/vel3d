
#include <vel/Scene/Scene.h>
#include <vel/Scene/UI/UIView.h>
#include <vel/Scene/UI/UITextButton.h>

namespace vel
{
	UITextButton::UITextButton(const std::string& id) :
		UIButton(id),
		text(""),
		fontSize(16),
		fontType("Arial"),
		topPadding(0),
		bottomPadding(0),
		leftPadding(0),
		rightPadding(0),

		textColor({ 1.f, 1.f, 1.f, 1.f }),
		textColorHover({ 1.f, 1.f, 1.f, 1.f }),
		textColorClick({ 1.f, 1.f, 1.f, 1.f }),

		backgroundColor({ 1.f, 1.f, 1.f, 1.f }),
		backgroundColorHover({ 1.f, 1.f, 1.f, 1.f }),
		backgroundColorClick({ 1.f, 1.f, 1.f, 1.f }),

		textAlignment(TextButtonAlignment::CENTER)
	{}

	UITextButton* UITextButton::setHoldUntilMouseUp(bool b)
	{
		UIButton::setHoldUntilMouseUp(b);
		return this;
	}

	UITextButton* UITextButton::addBackgroundImage(const std::string& i, bool filter)
	{
		UIButton::addBackgroundImage(i, filter);
		return this;
	}

	UITextButton* UITextButton::setMouseOverEvent(std::function<void()> f)
	{
		UIButton::setMouseOverEvent([this, f = std::move(f)]() {

			Scene* s = this->getParentView()->scene;

			s->getActor(this->buttonActor).colorMultiplier = this->backgroundColorHover;
			s->getTextActor(this->textActor).colorMultiplier = this->textColorHover;

			f();

		});

		return this;
	}

	UITextButton* UITextButton::setMouseOutEvent(std::function<void()> f)
	{
		UIButton::setMouseOutEvent([this, f = std::move(f)]() {

			Scene* s = this->getParentView()->scene;

			s->getActor(this->buttonActor).colorMultiplier = this->backgroundColor;
			s->getTextActor(this->textActor).colorMultiplier = this->textColor;

			f();

		});

		return this;
	}

	UITextButton* UITextButton::setMouseDownEvent(std::function<void()> f)
	{
		UIButton::setMouseDownEvent([this, f = std::move(f)]() {

			Scene* s = this->getParentView()->scene;

			s->getActor(this->buttonActor).colorMultiplier = this->backgroundColorClick;
			s->getTextActor(this->textActor).colorMultiplier = this->textColorClick;

			f();

		});

		return this;
	}

	UITextButton* UITextButton::setMouseUpEvent(std::function<void()> f)
	{
		UIButton::setMouseUpEvent([this, f = std::move(f)]() {

			Scene* s = this->getParentView()->scene;

			s->getActor(this->buttonActor).colorMultiplier = this->backgroundColor;
			s->getTextActor(this->textActor).colorMultiplier = this->textColor;

			f();

		});

		return this;
	}

	UITextButton* UITextButton::setWidth(int w)
	{
		UIButton::setWidth(w);
		return this;
	}

	UITextButton* UITextButton::setHeight(int h)
	{
		UIButton::setHeight(h);
		return this;
	}

	TextButtonAlignment UITextButton::getTextAlignment() const
	{
		return this->textAlignment;
	}

	UITextButton* UITextButton::setTextAlignment(TextButtonAlignment a)
	{
		this->textAlignment = a;
		return this;
	}

	UITextButton* UITextButton::setTopPadding(int p)
	{
		this->topPadding = p;
		return this;
	}

	UITextButton* UITextButton::setBottomPadding(int p)
	{
		this->bottomPadding = p;
		return this;
	}

	UITextButton* UITextButton::setLeftPadding(int p)
	{
		this->leftPadding = p;
		return this;
	}

	UITextButton* UITextButton::setRightPadding(int p)
	{
		this->rightPadding = p;
		return this;
	}

	int UITextButton::getTopPadding() const
	{
		return this->topPadding;
	}

	int UITextButton::getBottomPadding() const
	{
		return this->bottomPadding;
	}

	int UITextButton::getLeftPadding() const
	{
		return this->leftPadding;
	}

	int UITextButton::getRightPadding() const
	{
		return this->rightPadding;
	}

	const std::string& UITextButton::getText() const
	{
		return this->text;
	}

	int	UITextButton::getFontSize() const
	{
		return this->fontSize;
	}

	const std::string& UITextButton::getFontType() const
	{
		return this->fontType;
	}

	glm::vec4 UITextButton::getTextColor() const
	{
		return this->textColor;
	}

	glm::vec4 UITextButton::getTextColorHover() const
	{
		return this->textColorHover;
	}

	glm::vec4 UITextButton::getTextColorClick() const
	{
		return this->textColorClick;
	}

	glm::vec4 UITextButton::getBackgroundColor() const
	{
		return this->backgroundColor;
	}

	glm::vec4 UITextButton::getBackgroundColorHover() const
	{
		return this->backgroundColorHover;
	}

	glm::vec4 UITextButton::getBackgroundColorClick() const
	{
		return this->backgroundColorClick;
	}

	void UITextButton::updateButtonActorSize()
	{
		Scene* s = this->getParentView()->scene;
		
		// if width value is provided (not 0), we assume padding accounted for in width
		int buttonActorWidth = this->width;
		if (buttonActorWidth == 0)
			buttonActorWidth = s->getActorWorldAABB(s->getTextActor(this->textActor)).getSize().x + this->leftPadding + this->rightPadding;

		// if height value is provided (not 0), we assume padding accounted for in height
		int buttonActorHeight = this->height;
		if (buttonActorHeight == 0)
			buttonActorHeight = s->getText(this->textActor).logicalHeight + this->topPadding + this->bottomPadding;

		s->getActor(this->buttonActor).setScale({ buttonActorWidth, -buttonActorHeight, 1.f });
	}

	UITextButton* UITextButton::setPosition(glm::vec2 pos)
	{
		UIButton::setPosition(pos);

		if (!this->getIsInitialized())
			return this;

		Scene* s = this->parentView->scene;

		Actor& ba = s->getActor(this->buttonActor);
		Actor& ta = s->getTextActor(this->textActor);

		//
		// Translate buttonActor, accounting for vertical padding
		//
		float buttonActorHeightOffset = 0.0f;
		buttonActorHeightOffset -= this->topPadding;
		buttonActorHeightOffset += this->bottomPadding;
		buttonActorHeightOffset *= 0.5f;

		ba.setTranslation({
			this->getPosition().x,
			this->getPosition().y + buttonActorHeightOffset,
			this->zIndex
		});


		//
		// Temporary placement of textActor
		//
		glm::vec3 textPosition = glm::vec3(this->getPosition(), this->zIndex + 0.005f);
		ta.setTranslation(textPosition);


		//
		// Align the visible text AABB within the button.
		//
		vel::AABB buttonAABB = s->getActorWorldAABB(ba);
		vel::AABB textAABB = s->getActorWorldAABB(ta);

		float buttonMinX = buttonAABB.minEdge.x;
		float buttonMaxX = buttonAABB.maxEdge.x;
		float buttonCenterY = (buttonAABB.minEdge.y + buttonAABB.maxEdge.y) * 0.5f;

		float textMinX = textAABB.minEdge.x;
		float textMaxX = textAABB.maxEdge.x;
		float textCenterX = (textMinX + textMaxX) * 0.5f;
		float textCenterY = (textAABB.minEdge.y + textAABB.maxEdge.y) * 0.5f;


		//
		// Vertical visual centering
		//
		textPosition.y += buttonCenterY - textCenterY;


		//
		// Horizontal visual alignment
		//
		if (this->getTextAlignment() == TextButtonAlignment::LEFT)
		{
			float desiredTextMinX = buttonMinX + this->leftPadding;
			textPosition.x += desiredTextMinX - textMinX;
		}
		else if (this->getTextAlignment() == TextButtonAlignment::RIGHT)
		{
			float desiredTextMaxX = buttonMaxX - this->rightPadding;
			textPosition.x += desiredTextMaxX - textMaxX;
		}
		else // CENTER
		{
			float buttonCenterX = (buttonMinX + buttonMaxX) * 0.5f;
			textPosition.x += buttonCenterX - textCenterX;
		}

		ta.setTranslation(textPosition);

		return this;
	}

	UITextButton* UITextButton::setZIndex(float z)
	{
		UIButton::setZIndex(z);
		return this;
	}

	UITextButton* UITextButton::setVisible(bool b)
	{
		UIButton::setVisible(b);

		if (!this->getIsInitialized())
			return this;

		Scene* s = this->parentView->scene;

		Actor& ba = s->getActor(this->buttonActor);
		Actor& ta = s->getTextActor(this->textActor);

		ta.flags = (ta.flags & ~ACTFLG_VISIBLE) | (b ? ACTFLG_VISIBLE : 0);
		ba.flags = (ba.flags & ~ACTFLG_VISIBLE) | (b ? ACTFLG_VISIBLE : 0);

		return this;
	}

	UITextButton* UITextButton::setParent(actor_handle a)
	{
		if (!a)
			return this;

		UIElement::setParent(a);

		if (!this->isInitialized)
			return this;

		Scene* s = this->parentView->scene;

		s->setActorParent(this->buttonActor, a);
		s->setActorParent(s->getText(this->textActor).actor, a);

		return this;
	}

	UITextButton* UITextButton::setFontSize(int px)
	{
		this->fontSize = px;
		return this;
	}

	UITextButton* UITextButton::setFontType(const std::string& t)
	{
		this->fontType = t;
		return this;
	}

	UITextButton* UITextButton::setText(const std::string& t)
	{
		this->text = t;

		if (!this->getIsInitialized())
			return this;

		Scene* s = this->parentView->scene;

		Text& text = s->getText(this->textActor);
		text.updateText(t);

		s->updateText(text);

		this->updateButtonActorSize();
		this->setPosition(this->getPosition());

		return this;
	}

	UITextButton* UITextButton::setTextColor(glm::vec4 c)
	{
		this->textColor = c;
		return this;
	}

	UITextButton* UITextButton::setTextColorHover(glm::vec4 c)
	{
		this->textColorHover = c;
		return this;
	}

	UITextButton* UITextButton::setTextColorClick(glm::vec4 c)
	{
		this->textColorClick = c;
		return this;
	}

	UITextButton* UITextButton::setBackgroundColor(glm::vec4 c)
	{
		this->backgroundColor = c;
		return this;
	}

	UITextButton* UITextButton::setBackgroundColorHover(glm::vec4 c)
	{
		this->backgroundColorHover = c;
		return this;
	}

	UITextButton* UITextButton::setBackgroundColorClick(glm::vec4 c)
	{
		this->backgroundColorClick = c;
		return this;
	}
}