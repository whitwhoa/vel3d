
#include <vel/Scene/Scene.h>
#include <vel/Scene/UI/UIView.h>
#include <vel/Scene/UI/UISelect.h>

namespace vel
{
	UISelect::UISelect(const std::string& id) :
		UIElement(id),
		isExpanded(false),

		fontSize(10),
		fontType("Arial"),
		width(100),
		scrollbarWidth(10),
		dropdownHeight(0),
		handleHeight(0.f),
		fullExpandedHeight(0.f),

		selectedTextColor({ 0.8471f, 0.8471f, 0.8471f, 1.0000f }),
		selectedBackgroundColor({ 0.2431f, 0.2431f, 0.2431f, 1.0000f }),
		optionTextColor({ 0.6235f, 0.6235f, 0.6235f, 1.0000f }),
		optionTextColorHover({ 1.0000f, 1.0000f, 1.0000f, 1.0000f }),
		optionBackgroundColor({ 0.3059f, 0.3059f, 0.3059f, 1.0000f }),
		optionBackgroundColorHover({ 0.3882f, 0.3882f, 0.3882f, 1.0000f }),
		scrollbarHandleColor({ 0.3059f, 0.3059f, 0.3059f, 1.0000f }),
		scrollbarHandleColorClick({ 0.2431f, 0.2431f, 0.2431f, 1.0000f }),
		scrollbarTrackColor({ 0.3647f, 0.3647f, 0.3647f, 1.0000f }),

		selectedIndex(-1),
		visibleOptions(0),

		visibleOptionsCursor(0),
		scrollHandleIncrement(0),
		selectButton(nullptr),
		scrollUpButton(nullptr),
		scrollDownButton(nullptr),
		scrollbarHandleButton(nullptr),
		scrollbarHandleDefaultYPos(0.f),
		holdingHandle(false),
		handleDragYPos(0.f)
	{}

	int UISelect::getWidth() const
	{
		if (!this->isInitialized)
			return this->width;

		return this->selectButton->getWidth();
	}

	int UISelect::getHeight() const
	{
		if (!this->isInitialized)
			return 0;

		return this->selectButton->getHeight();
	}

	int UISelect::getSelectedIndex() const
	{
		return this->selectedIndex;
	}

	void UISelect::resetVisibleOptionsCursor()
	{
		this->visibleOptionsCursor = 0;
		if (this->selectedIndex >= this->visibleOptions)
			this->visibleOptionsCursor = this->selectedIndex + 1 - this->visibleOptions;
	}

	void UISelect::repositionOptions()
	{
		int optionIndex = -1;
		for (auto* opt : this->optionButtons)
		{
			optionIndex++;

			float startY = this->selectButton->getPosition().y + this->selectButton->getHeight() * 0.5f + opt->getHeight() * 0.5f;
			float xPos = this->selectButton->getPosition().x - this->scrollUpButton->getWidth() * 0.5f;
			float yPos = startY + (opt->getHeight() * (optionIndex - this->visibleOptionsCursor));
			opt->setPosition({ xPos, yPos });
		}
	}

	void UISelect::displayOptions()
	{
		this->isExpanded = true;

		Actor& sta = this->parentView->scene->getActor(this->scrollbarTrackActor);
		sta.flags |= ACTFLG_VISIBLE;

		this->scrollUpButton->setVisible(true);
		this->scrollDownButton->setVisible(true);
		this->scrollbarHandleButton->setVisible(true);

		this->repositionOptions();

		int optionIndex = -1;
		for (auto* opt : this->optionButtons)
		{
			optionIndex++;

			opt->setVisible(true);
			if (optionIndex < this->visibleOptionsCursor || optionIndex >= (this->visibleOptionsCursor + this->visibleOptions))
				opt->setVisible(false);
		}
	}

	void UISelect::hideOptions()
	{
		this->isExpanded = false;

		Actor& sta = this->parentView->scene->getActor(this->scrollbarTrackActor);
		sta.flags &= ~ACTFLG_VISIBLE;

		this->scrollUpButton->setVisible(false);
		this->scrollDownButton->setVisible(false);
		this->scrollbarHandleButton->setVisible(false);
		this->resetVisibleOptionsCursor();
		this->scrollHome();

		for (auto* opt : this->optionButtons)
			opt->setVisible(false);
	}

	void UISelect::scrollUp()
	{
		if (this->visibleOptionsCursor == 0)
			return;

		this->visibleOptionsCursor--;

		float yAmount = -1 * this->scrollHandleIncrement;

		this->scrollbarHandleButton->setPosition(
			this->scrollbarHandleButton->getPosition() +
			glm::vec2(0.f, yAmount)
		);

		this->displayOptions();
	}

	void UISelect::scrollDown()
	{
		if (this->visibleOptionsCursor == this->optionButtons.size() - this->visibleOptions)
			return;

		this->visibleOptionsCursor++;

		float yAmount = this->scrollHandleIncrement;

		this->scrollbarHandleButton->setPosition(
			this->scrollbarHandleButton->getPosition() +
			glm::vec2(0.f, yAmount)
		);

		this->displayOptions();
	}

	void UISelect::scrollHome()
	{
		float scrollbarHandleYPos = this->scrollbarHandleDefaultYPos +
			this->visibleOptionsCursor * this->scrollHandleIncrement;

		glm::vec2 curPos = this->scrollbarHandleButton->getPosition();

		this->scrollbarHandleButton->setPosition({ curPos.x, scrollbarHandleYPos });
	}

	bool UISelect::cursorOverOptions(glm::vec2 p) const
	{
		return this->parentView->scene->actorContainsPoint(this->optionsGhostActor, p);
	}

	bool UISelect::cursorOverExpanded(glm::vec2 p) const
	{
		return this->parentView->scene->actorContainsPoint(this->expandedGhostActor, p);
	}

	UISelect* UISelect::setVisible(bool v)
	{
		UIElement::setVisible(v);

		if (!this->isInitialized)
			return this;

		this->selectButton->setVisible(v);

		this->hideOptions();
		Actor& sta = this->parentView->scene->getActor(this->scrollbarTrackActor);
		sta.flags &= ~ACTFLG_VISIBLE;
		this->scrollUpButton->setVisible(false);
		this->scrollDownButton->setVisible(false);
		this->scrollbarHandleButton->setVisible(false);

		return this;
	}

	UISelect* UISelect::setParent(actor_handle a)
	{
		if (!a)
			return this;

		UIElement::setParent(a);

		if (!this->isInitialized)
			return this;

		for (auto* o : this->optionButtons)
			o->setParent(a);

		Scene* s = this->parentView->scene;

		s->setActorParent(this->scrollbarTrackActor, a);
		s->setActorParent(this->optionsGhostActor, a);
		s->setActorParent(this->expandedGhostActor, a);

		this->selectButton->setParent(a);
		this->scrollUpButton->setParent(a);
		this->scrollDownButton->setParent(a);
		this->scrollbarHandleButton->setParent(a);

		return this;
	}

	UISelect* UISelect::setFontSize(int size)
	{
		this->fontSize = size;
		return this;
	}

	UISelect* UISelect::setFontType(const std::string& t)
	{
		this->fontType = t;
		return this;
	}

	UISelect* UISelect::setWidth(int width)
	{
		this->width = width;
		return this;
	}

	UISelect* UISelect::setSelectedIndex(int val)
	{
		this->selectedIndex = val;
		return this;
	}

	UISelect* UISelect::setVisibleOptions(int opts)
	{
		this->visibleOptions = opts;
		return this;
	}

	UISelect* UISelect::addOption(const std::string& opt)
	{
		this->optionCache.push_back(opt);
		return this;
	}

	UISelect* UISelect::setPosition(glm::vec2 pos)
	{
		this->positionCache = pos;

		if (!this->isInitialized)
			return this;


		this->selectButton->setPosition(pos);
		this->repositionOptions();


		float scrollUpButtonXPos = this->selectButton->getPosition().x +
			this->selectButton->getWidth() * 0.5f -
			this->scrollUpButton->getWidth() * 0.5f;

		float scrollUpButtonYPos = this->selectButton->getPosition().y +
			this->selectButton->getHeight() * 0.5f +
			this->scrollUpButton->getHeight() * 0.5f;

		this->scrollUpButton->setPosition({ scrollUpButtonXPos, scrollUpButtonYPos });


		float scrollDownButtonXPos = this->selectButton->getPosition().x +
			this->selectButton->getWidth() * 0.5f -
			this->scrollDownButton->getWidth() * 0.5f;

		float scrollDownButtonYPos = this->selectButton->getPosition().y +
			this->selectButton->getHeight() * 0.5f +
			this->dropdownHeight -
			this->scrollDownButton->getHeight() * 0.5f;

		this->scrollDownButton->setPosition({ scrollDownButtonXPos, scrollDownButtonYPos });


		float scrollbarTrackYPos = this->selectButton->getPosition().y +
			this->selectButton->getHeight() * 0.5f +
			this->dropdownHeight * 0.5f;

		Scene* s = this->parentView->scene;

		Actor& sta = s->getActor(this->scrollbarTrackActor);
		sta.setTranslation({ scrollDownButtonXPos, scrollbarTrackYPos, this->zIndex + 0.001f });


		this->scrollbarHandleDefaultYPos = this->scrollUpButton->getPosition().y +
			this->scrollUpButton->getHeight() * 0.5f +
			this->handleHeight * 0.5f;

		float scrollbarHandleYPos = this->scrollbarHandleDefaultYPos +
			this->visibleOptionsCursor * this->scrollHandleIncrement;

		this->scrollbarHandleButton->setPosition({ scrollDownButtonXPos, scrollbarHandleYPos });


		float optionsGhostActorYPos = this->selectButton->getPosition().y +
			this->selectButton->getHeight() * 0.5f +
			this->dropdownHeight * 0.5f;

		Actor& oga = s->getActor(this->optionsGhostActor);
		oga.setTranslation({ this->selectButton->getPosition().x, optionsGhostActorYPos, this->zIndex });


		float expandedGhostActorYPos = this->selectButton->getPosition().y -
			this->selectButton->getHeight() * 0.5f +
			this->fullExpandedHeight * 0.5f;

		Actor& ega = s->getActor(this->expandedGhostActor);
		ega.setTranslation({ this->selectButton->getPosition().x, expandedGhostActorYPos, this->zIndex });


		return this;
	}

	UISelect* UISelect::setZIndex(float z)
	{
		UIElement::setZIndex(z);
		return this;
	}

	UISelect* UISelect::setSelectedTextColor(glm::vec4 c)
	{
		this->selectedTextColor = c;
		return this;
	}

	UISelect* UISelect::setSelectedBackgroundColor(glm::vec4 c)
	{
		this->selectedBackgroundColor = c;
		return this;
	}

	UISelect* UISelect::setOptionTextColor(glm::vec4 c)
	{
		this->optionTextColor = c;
		return this;
	}

	UISelect* UISelect::setOptionTextColorHover(glm::vec4 c)
	{
		this->optionTextColorHover = c;
		return this;
	}

	UISelect* UISelect::setOptionBackgroundColor(glm::vec4 c)
	{
		this->optionBackgroundColor = c;
		return this;
	}

	UISelect* UISelect::setOptionBackgroundColorHover(glm::vec4 c)
	{
		this->optionBackgroundColorHover = c;
		return this;
	}

	UISelect* UISelect::setScrollbarHandleColor(glm::vec4 c)
	{
		this->scrollbarHandleColor = c;
		return this;
	}

	UISelect* UISelect::setScrollbarHandleColorClick(glm::vec4 c)
	{
		this->scrollbarHandleColorClick = c;
		return this;
	}

	UISelect* UISelect::setScrollbarTrackColor(glm::vec4 c)
	{
		this->scrollbarTrackColor = c;
		return this;
	}

	bool UISelect::containsPoint(glm::vec2 p)
	{
		if (!this->getIsInitialized())
			return false;

		if (!this->isVisible)
			return false;

		if (!this->isExpanded && this->selectButton->containsPoint(p))
			return true;

		if (this->isExpanded && (this->selectButton->containsPoint(p) || this->cursorOverExpanded(p)))
			return true;


		return false;
	}

	void UISelect::update(float dt, const vel::InputState& is, UICursor* c)
	{
		if (!this->isVisible)
			return;

		//
		// Expanded events
		//
		if (this->isExpanded)
		{
			if (this->cursorOverOptions(glm::vec2(c->getPos())))
			{
				if (is.scroll > 0)
					this->scrollUp();
				else if (is.scroll < 0)
					this->scrollDown();
			}

			if (!this->cursorOverExpanded(glm::vec2(c->getPos())))
				if (is.mouseLeftButton)
					this->hideOptions();
		}

		//
		// Pull handle
		//
		if (!this->holdingHandle)
			return;

		float scrollAmount = this->handleDragYPos - c->getPos().y;
		int direction = 0;
		direction = scrollAmount > 0 ? 1 : -1;
		int magnitude = static_cast<int>(fabs(scrollAmount) / this->scrollHandleIncrement);

		if (magnitude > 0 && direction != 0)
		{
			this->handleDragYPos = c->getPos().y;

			while (magnitude > 0)
			{
				if (direction > 0)
					this->scrollUp();
				else
					this->scrollDown();

				magnitude--;
			}
		}
	}
}