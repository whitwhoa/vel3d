
#include <vel/Scene/Scene.h>
#include <vel/Scene/UI/UIScrollView.h>

namespace vel
{
	UIScrollView::UIScrollView(const std::string& id) :
		UIView(),
		UIElement(id),
		scrollbarButtonColor({ 0.2431f, 0.2431f, 0.2431f, 1.0000f }),
		scrollbarButtonClickColor({ 0.3647f, 0.3647f, 0.3647f, 1.0000f }),
		scrollbarButtonTextColor({ 0.8471f, 0.8471f, 0.8471f, 1.0000f }),
		scrollbarHandleColor({ 0.3059f, 0.3059f, 0.3059f, 1.0000f }),
		scrollbarHandleClickColor({ 0.2431f, 0.2431f, 0.2431f, 1.0000f }),
		scrollbarTrackColor({ 0.3647f, 0.3647f, 0.3647f, 1.0000f }),
		ghostDefaultPos({ 0.f, 0.f, 0.f }),
		ghostMaxPos({ 0.f, 0.f, 0.f }),
		scrollbarUpButton(nullptr),
		scrollbarDownButton(nullptr),
		scrollbarLeftButton(nullptr),
		scrollbarRightButton(nullptr),
		scrollbarXHandleButton(nullptr),
		scrollbarYHandleButton(nullptr),
		cameraScrollYMultiplier(0.f),
		cameraScrollXMultiplier(0.f),
		handleXStartPos(0.f),
		handleXEndPos(0.f),
		handleXDragPos(0.f),
		handleYStartPos(0.f),
		handleYEndPos(0.f),
		handleYDragPos(0.f),
		usableXScrollingDistance(0.f),
		xHandleMag(0.f),
		usableYScrollingDistance(0.f),
		yHandleMag(0.f),
		width(0),
		height(0),
		viewWidth(0),
		viewHeight(0),
		maxX(0),
		maxY(0),
		scrollbarThickness(0),
		scrollContentPadding(0),
		holdingXHandle(false),
		holdingYHandle(false),
		holdingScrollUpButton(false),
		holdingScrollDownButton(false),
		holdingScrollLeftButton(false),
		holdingScrollRightButton(false),
		hideScrollButtons(false),
		requireScrollX(false),
		requireScrollY(false)
	{}

	void UIScrollView::setScrollButtonPositions(glm::vec2& scrollLeftButtonPos,
		glm::vec2& scrollRightButtonPos, glm::vec2& scrollUpButtonPos,
		glm::vec2& scrollDownButtonPos)
	{
		if (this->requireScrollX && this->requireScrollY)
		{
			this->viewWidth = this->width - this->scrollbarThickness;
			this->viewHeight = this->height - this->scrollbarThickness;

			scrollLeftButtonPos = {
				this->getPosition().x + this->scrollbarThickness * 0.5f,
				this->getPosition().y + this->height - this->scrollbarThickness * 0.5f
			};

			scrollRightButtonPos = {
				this->getPosition().x + this->viewWidth + this->scrollbarThickness * 0.5f,
				this->getPosition().y + this->height - this->scrollbarThickness * 0.5f
			};

			scrollUpButtonPos = {
				this->getPosition().x + this->viewWidth + this->scrollbarThickness * 0.5f,
				this->getPosition().y + this->scrollbarThickness * 0.5f
			};

			scrollDownButtonPos = {
				this->getPosition().x + this->viewWidth + this->scrollbarThickness * 0.5f,
				this->getPosition().y + this->viewHeight - this->scrollbarThickness * 0.5f
			};
		}
		else if (this->requireScrollX && !this->requireScrollY)
		{
			this->viewHeight = this->height - this->scrollbarThickness;

			scrollLeftButtonPos = {
				this->getPosition().x + this->scrollbarThickness * 0.5f,
				this->getPosition().y + this->height - this->scrollbarThickness * 0.5f
			};

			scrollRightButtonPos = {
				this->getPosition().x + this->viewWidth - this->scrollbarThickness * 0.5f,
				this->getPosition().y + this->height - this->scrollbarThickness * 0.5f
			};
		}
		else if (!this->requireScrollX && this->requireScrollY)
		{
			this->viewWidth = this->width - this->scrollbarThickness;

			scrollUpButtonPos = {
				this->getPosition().x + this->viewWidth + this->scrollbarThickness * 0.5f,
				this->getPosition().y + this->scrollbarThickness * 0.5f
			};

			scrollDownButtonPos = {
				this->getPosition().x + this->viewWidth + this->scrollbarThickness * 0.5f,
				this->getPosition().y + this->viewHeight - this->scrollbarThickness * 0.5f
			};
		}
	}

	float UIScrollView::getTopOffset() const
	{
		Actor& a = this->parentView->scene->getActor(this->ghostParentActor);
		return std::fabs(a.getTranslation().y - this->getPosition().y);
	}

	float UIScrollView::getLeftOffset() const
	{
		Actor& a = this->parentView->scene->getActor(this->ghostParentActor);
		return std::fabs(a.getTranslation().x - this->getPosition().x);
	}

	UIScrollView* UIScrollView::setPosition(glm::vec2 pos)
	{
		UIElement::setPosition(pos);

		if (!this->isInitialized)
			return this;


		this->camera->position = glm::vec3(pos, 0.f);

		glm::vec2 scrollLeftButtonPos = { 0.f, 0.f };
		glm::vec2 scrollRightButtonPos = { 0.f, 0.f };
		glm::vec2 scrollUpButtonPos = { 0.f, 0.f };
		glm::vec2 scrollDownButtonPos = { 0.f, 0.f };
		this->setScrollButtonPositions(scrollLeftButtonPos, scrollRightButtonPos, scrollUpButtonPos, scrollDownButtonPos);

		Scene* s = this->parentView->scene;

		Actor& gpa = s->getActor(this->ghostParentActor);

		glm::vec3 ghostOffset = gpa.getTranslation() - this->ghostDefaultPos;
		this->ghostDefaultPos = glm::vec3(pos, this->zIndex);
		this->ghostMaxPos = {
			pos.x - this->maxX + this->viewWidth,
			pos.y - this->maxY + this->viewHeight,
			this->zIndex
		};
		gpa.setTranslation(this->ghostDefaultPos + ghostOffset);

		Actor& ba = s->getActor(this->backgroundActor);
		Actor& da = s->getActor(this->displayActor);

		ba.setTranslation({ pos, this->zIndex });
		da.setTranslation({ pos, this->zIndex + 0.001f });

		if (this->requireScrollX)
		{
			float handleXOffset = this->scrollbarXHandleButton->getPosition().x - this->handleXStartPos;

			Actor& stxa = s->getActor(this->scrollbarTrackXActor);

			stxa.setTranslation({
				pos.x + this->width * 0.5f,
				pos.y + this->viewHeight + this->scrollbarThickness * 0.5f,
				this->zIndex + 0.001f
			});

			if (!this->hideScrollButtons)
			{
				this->handleXStartPos = pos.x +
					this->scrollbarThickness +
					this->xHandleMag * 0.5f;
				this->handleXEndPos = pos.x +
					this->width -
					this->scrollbarThickness -
					this->xHandleMag * 0.5f;
			}
			else
			{
				this->handleXStartPos = pos.x +
					this->xHandleMag * 0.5f;
				this->handleXEndPos = pos.x +
					this->width -
					this->xHandleMag * 0.5f;
			}

			this->scrollbarXHandleButton->setPosition({
				this->handleXStartPos + handleXOffset,
				stxa.getTranslation().y
			});

			if (!this->hideScrollButtons)
			{
				this->scrollbarLeftButton->setPosition(scrollLeftButtonPos);
				this->scrollbarRightButton->setPosition(scrollRightButtonPos);
			}
		}

		if (this->requireScrollY)
		{
			float handleYOffset = this->scrollbarYHandleButton->getPosition().y - this->handleYStartPos;

			Actor& stya = s->getActor(this->scrollbarTrackYActor);

			stya.setTranslation({
				pos.x + this->viewWidth,
				pos.y,
				this->zIndex + 0.001f
			});

			if (!this->hideScrollButtons)
			{
				this->handleYStartPos = pos.y +
					this->scrollbarThickness +
					this->yHandleMag * 0.5f;
				this->handleYEndPos = pos.y +
					this->viewHeight -
					this->scrollbarThickness -
					this->yHandleMag * 0.5f;
			}
			else
			{
				this->handleYStartPos = pos.y +
					this->yHandleMag * 0.5f;
				this->handleYEndPos = pos.y +
					this->viewHeight -
					this->yHandleMag * 0.5f;
			}

			this->scrollbarYHandleButton->setPosition({
				stya.getTranslation().x + this->scrollbarThickness * 0.5f,
				this->handleYStartPos + handleYOffset
			});

			if (!this->hideScrollButtons)
			{
				this->scrollbarUpButton->setPosition(scrollUpButtonPos);
				this->scrollbarDownButton->setPosition(scrollDownButtonPos);
			}

		}

		return this;
	}

	UIScrollView* UIScrollView::setZIndex(float z)
	{
		UIElement::setZIndex(z);
		return this;
	}

	UIScrollView* UIScrollView::setVisible(bool b)
	{
		UIElement::setVisible(b);

		if (!this->isInitialized)
			return this;

		Scene* s = this->parentView->scene;

		Actor& ba = s->getActor(this->backgroundActor);
		Actor& da = s->getActor(this->displayActor);

		ba.flags = (ba.flags & ~ACTFLG_VISIBLE) | (b ? ACTFLG_VISIBLE : 0);
		da.flags = (da.flags & ~ACTFLG_VISIBLE) | (b ? ACTFLG_VISIBLE : 0);

		if (this->requireScrollX)
		{
			Actor& stxa = s->getActor(this->scrollbarTrackXActor);
			stxa.flags = (stxa.flags & ~ACTFLG_VISIBLE) | (b ? ACTFLG_VISIBLE : 0);

			this->scrollbarXHandleButton->setVisible(b);

			if (!this->hideScrollButtons)
			{
				this->scrollbarLeftButton->setVisible(b);
				this->scrollbarRightButton->setVisible(b);
			}
		}

		if (this->requireScrollY)
		{
			Actor& stya = s->getActor(this->scrollbarTrackYActor);
			stya.flags = (stya.flags & ~ACTFLG_VISIBLE) | (b ? ACTFLG_VISIBLE : 0);

			this->scrollbarYHandleButton->setVisible(b);

			if (!this->hideScrollButtons)
			{
				this->scrollbarUpButton->setVisible(b);
				this->scrollbarDownButton->setVisible(b);
			}
		}

		return this;
	}

	UIScrollView* UIScrollView::setParent(actor_handle a)
	{
		// Intentionally not implementing this right now, because for the time
		// being, we're just going to say that UIViews cannot be parented,
		// and move on

		return this;
	}


	UIScrollView* UIScrollView::setWidth(int w)
	{
		this->width = w;
		return this;
	}

	UIScrollView* UIScrollView::setHeight(int h)
	{
		this->height = h;
		return this;
	}

	UIScrollView* UIScrollView::setScrollbarButtonColor(glm::vec4 c)
	{
		this->scrollbarButtonColor = c;
		return this;
	}

	UIScrollView* UIScrollView::setScrollbarButtonClickColor(glm::vec4 c)
	{
		this->scrollbarButtonClickColor = c;
		return this;
	}

	UIScrollView* UIScrollView::setScrollbarButtonTextColor(glm::vec4 c)
	{
		this->scrollbarButtonTextColor = c;
		return this;
	}

	UIScrollView* UIScrollView::setScrollbarHandleColor(glm::vec4 c)
	{
		this->scrollbarHandleColor = c;
		return this;
	}
	UIScrollView* UIScrollView::setScrollbarHandleClickColor(glm::vec4 c)
	{
		this->scrollbarHandleClickColor = c;
		return this;
	}

	UIScrollView* UIScrollView::setScrollbarTrackColor(glm::vec4 c)
	{
		this->scrollbarTrackColor = c;
		return this;
	}

	UIScrollView* UIScrollView::setBackgroundColor(glm::vec4 c)
	{
		this->backgroundColor = c;
		return this;
	}

	UIScrollView* UIScrollView::setBackgroundImage(const std::string& s)
	{
		this->backgroundImage = s;
		return this;
	}

	UIScrollView* UIScrollView::setHideScrollButtons(bool b)
	{
		this->hideScrollButtons = b;
		return this;
	}


	bool UIScrollView::containsPoint(glm::vec2 p)
	{
		if (!this->getIsInitialized())
			return false;

		if (!this->isVisible)
			return false;

		if (!this->parentView->scene->actorContainsPoint(this->displayActor, p))
			return false;

		return true;
	}

	int UIScrollView::getWidth() const
	{
		return this->width;
	}

	int UIScrollView::getHeight() const
	{
		return this->height;
	}

	void UIScrollView::findMaxScrollPos()
	{
		int prevMaxX = this->maxX;
		int prevMaxY = this->maxY;

		for (auto& e : this->elements)
		{
			switch (e->getOriginType())
			{
			case vel::PlaneOrigin::LEFT_BOTTOM:
			{
				int x = std::round(e->getPosition().x + e->getWidth());
				if (maxX < x)
					maxX = x;

				int y = std::round(e->getPosition().y);
				if (maxY < y)
					maxY = y;

				break;
			}
			case vel::PlaneOrigin::LEFT_CENTER:
			{
				int x = std::round(e->getPosition().x + e->getWidth());
				if (maxX < x)
					maxX = x;

				int y = std::round(e->getPosition().y + e->getHeight() * 0.5f);
				if (maxY < y)
					maxY = y;

				break;
			}
			case vel::PlaneOrigin::LEFT_TOP:
			{
				int x = std::round(e->getPosition().x + e->getWidth());
				if (maxX < x)
					maxX = x;

				int y = std::round(e->getPosition().y + e->getHeight());
				if (maxY < y)
					maxY = y;

				break;
			}
			case vel::PlaneOrigin::CENTER_BOTTOM:
			{
				int x = std::round(e->getPosition().x + e->getWidth() * 0.5f);
				if (maxX < x)
					maxX = x;

				int y = std::round(e->getPosition().y);
				if (maxY < y)
					maxY = y;

				break;
			}
			case vel::PlaneOrigin::CENTER_CENTER:
			{
				int x = std::round(e->getPosition().x + e->getWidth() * 0.5f);
				if (maxX < x)
					maxX = x;

				int y = std::round(e->getPosition().y + e->getHeight() * 0.5f);
				if (maxY < y)
					maxY = y;

				break;
			}
			case vel::PlaneOrigin::CENTER_TOP:
			{
				int x = std::round(e->getPosition().x + e->getWidth() * 0.5f);
				if (maxX < x)
					maxX = x;

				int y = std::round(e->getPosition().y + e->getHeight());
				if (maxY < y)
					maxY = y;

				break;
			}

			case vel::PlaneOrigin::RIGHT_BOTTOM:
			{
				int x = std::round(e->getPosition().x);
				if (maxX < x)
					maxX = x;

				int y = std::round(e->getPosition().y);
				if (maxY < y)
					maxY = y;

				break;
			}
			case vel::PlaneOrigin::RIGHT_CENTER:
			{
				int x = std::round(e->getPosition().x);
				if (maxX < x)
					maxX = x;

				int y = std::round(e->getPosition().y + e->getHeight() * 0.5f);
				if (maxY < y)
					maxY = y;

				break;
			}
			case vel::PlaneOrigin::RIGHT_TOP:
			{
				int x = std::round(e->getPosition().x);
				if (maxX < x)
					maxX = x;

				int y = std::round(e->getPosition().y + e->getHeight());
				if (maxY < y)
					maxY = y;

				break;
			}
			}
		}

		requireScrollX = false;
		requireScrollY = false;

		//SPDLOG_TRACE("maxX: {} prevMaxX: {}", maxX, prevMaxX);
		//SPDLOG_TRACE("maxY: {} prevMaxY: {}", maxY, prevMaxY);

		if ((maxX > width) && maxX != prevMaxX)
		{
			requireScrollX = true;
			maxX += scrollContentPadding;
		}

		if ((maxY > height) && maxY != prevMaxY)
		{
			requireScrollY = true;
			maxY += scrollContentPadding;
		}

	}

	void UIScrollView::scrollRight(float scrollMag)
	{
		Actor& gpa = this->parentView->scene->getActor(this->ghostParentActor);

		const glm::vec3& ghostPos = gpa.getTranslation();
		const glm::vec2& handlePos = this->scrollbarXHandleButton->getPosition();

		float requestedPos = ghostPos.x - scrollMag * this->cameraScrollXMultiplier;
		float ghostXPos = 0.f;
		float handleXPos = 0.f;

		//SPDLOG_TRACE("requestedPos: {} ghostMaxPos.x: {}", requestedPos, this->ghostMaxPos.x);

		if (requestedPos <= this->ghostMaxPos.x)
		{
			ghostXPos = this->ghostMaxPos.x;
			handleXPos = this->handleXEndPos;
		}
		else
		{
			ghostXPos = requestedPos;
			handleXPos = handlePos.x + scrollMag;
		}

		gpa.setTranslation({ ghostXPos, ghostPos.y, ghostPos.z });
		this->scrollbarXHandleButton->setPosition({ handleXPos, handlePos.y });
	}

	void UIScrollView::scrollLeft(float scrollMag)
	{
		Actor& gpa = this->parentView->scene->getActor(this->ghostParentActor);

		const glm::vec3& ghostPos = gpa.getTranslation();
		const glm::vec2& handlePos = this->scrollbarXHandleButton->getPosition();

		float requestedPos = ghostPos.x + scrollMag * this->cameraScrollXMultiplier;
		float ghostXPos = 0.f;
		float handleXPos = 0.f;

		//SPDLOG_TRACE("requestedPos: {} ghostDefaultPos.x: {}", requestedPos, this->ghostDefaultPos.x);

		if (requestedPos >= this->ghostDefaultPos.x)
		{
			ghostXPos = this->ghostDefaultPos.x;
			handleXPos = this->handleXStartPos;
		}
		else
		{
			ghostXPos = requestedPos;
			handleXPos = handlePos.x - scrollMag;
		}

		gpa.setTranslation({ ghostXPos, ghostPos.y, ghostPos.z });
		this->scrollbarXHandleButton->setPosition({ handleXPos, handlePos.y });
	}

	void UIScrollView::scrollUp(float scrollMag)
	{
		Actor& gpa = this->parentView->scene->getActor(this->ghostParentActor);

		const glm::vec3& ghostPos = gpa.getTranslation();
		const glm::vec2& handlePos = this->scrollbarYHandleButton->getPosition();

		float requestedPos = ghostPos.y + scrollMag * this->cameraScrollYMultiplier;
		float ghostYPos = 0.f;
		float handleYPos = 0.f;

		//SPDLOG_TRACE("requestedPos: {} ghostDefaultPos.y: {}", requestedPos, this->ghostDefaultPos.y);

		if (requestedPos >= this->ghostDefaultPos.y)
		{
			ghostYPos = this->ghostDefaultPos.y;
			handleYPos = this->handleYStartPos;
		}
		else
		{
			ghostYPos = requestedPos;
			handleYPos = handlePos.y - scrollMag;
		}

		gpa.setTranslation({ ghostPos.x, ghostYPos, ghostPos.z });
		this->scrollbarYHandleButton->setPosition({ handlePos.x, handleYPos });
	}

	void UIScrollView::scrollDown(float scrollMag)
	{
		Actor& gpa = this->parentView->scene->getActor(this->ghostParentActor);

		const glm::vec3& ghostPos = gpa.getTranslation();
		const glm::vec2& handlePos = this->scrollbarYHandleButton->getPosition();

		float requestedPos = ghostPos.y - scrollMag * this->cameraScrollYMultiplier;
		float ghostYPos = 0.f;
		float handleYPos = 0.f;

		//SPDLOG_TRACE("requestedPos: {} ghostMaxPos.y: {}", requestedPos, this->ghostMaxPos.y);

		if (requestedPos <= this->ghostMaxPos.y)
		{
			ghostYPos = this->ghostMaxPos.y;
			handleYPos = this->handleYEndPos;
		}
		else
		{
			ghostYPos = requestedPos;
			handleYPos = handlePos.y + scrollMag;
		}

		gpa.setTranslation({ ghostPos.x, ghostYPos, ghostPos.z });
		this->scrollbarYHandleButton->setPosition({ handlePos.x, handleYPos });
	}

	void UIScrollView::updateYDrag(const UICursor* c)
	{
		if (!this->holdingYHandle)
			return;

		float scrollAmount = this->handleYDragPos - c->getPos().y;
		int direction = 0;
		direction = scrollAmount > 0 ? 1 : -1;
		int scrollMag = std::round(fabs(scrollAmount));

		this->handleYDragPos = c->getPos().y;

		if (scrollMag == 0)
			return;

		if (direction == 1)
			this->scrollUp(scrollMag);
		else if (direction == -1)
			this->scrollDown(scrollMag);
	}

	void UIScrollView::updateXDrag(const UICursor* c)
	{
		if (!this->holdingXHandle)
			return;

		float scrollAmount = c->getPos().x - this->handleXDragPos;
		int direction = 0;
		direction = scrollAmount > 0 ? 1 : -1;
		int scrollMag = std::round(fabs(scrollAmount));

		this->handleXDragPos = c->getPos().x;

		if (scrollMag == 0)
			return;

		if (direction == 1)
			this->scrollRight(scrollMag);
		else if (direction == -1)
			this->scrollLeft(scrollMag);
	}

	void UIScrollView::update(float dt, const vel::InputState& is, UICursor* c)
	{
		for (auto& e : this->elements)
			e->update(dt, is, c);

		//SPDLOG_TRACE("({},{})", this->scrollbarYHandleButton->getPosition().x, 
		//	this->scrollbarYHandleButton->getPosition().y);

		this->updateYDrag(c);
		this->updateXDrag(c);


		if (this->holdingScrollUpButton)
			this->scrollUp(70.f * dt);
		else if (this->holdingScrollDownButton)
			this->scrollDown(70.f * dt);
		else if (this->holdingScrollLeftButton)
			this->scrollLeft(70.f * dt);
		else if (this->holdingScrollRightButton)
			this->scrollRight(70.f * dt);


		if (c->overElement && c->overElement->getIsScrollable())
			return;


		if (this->parentView->scene->actorContainsPoint(this->displayActor, glm::vec2(c->getPos())))
		{
			if (this->requireScrollY && is.up(vel::VEL_KEY_LEFT_SHIFT))
			{
				if (is.scroll > 0)
					this->scrollUp(10.f);
				else if (is.scroll < 0)
					this->scrollDown(10.f);
			}

			if (this->requireScrollX && is.down(vel::VEL_KEY_LEFT_SHIFT))
			{
				if (is.scroll > 0)
					this->scrollLeft(10.f);
				else if (is.scroll < 0)
					this->scrollRight(10.f);
			}
		}

	}
}