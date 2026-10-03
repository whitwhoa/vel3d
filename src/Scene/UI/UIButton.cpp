


#include <vel/Scene/UI/UIButton.h>



namespace vel
{
    UIButton::UIButton(const std::string& id) :
        UIElement(id),

        width(0),
        height(0),

        isMouseOver(false),
        initialMouseOver(false),
        isMouseOut(true),
        initialMouseOut(false),
        isMouseUp(true),
        initialMouseUp(false),
        isMouseDown(false),
        initialMouseDown(false),
        holdUntilMouseUp(false),


        mouseOverEvent(),
        mouseOutEvent(),
        mouseUpEvent(),
        mouseDownEvent()

    {}

    UIButton* UIButton::setHoldUntilMouseUp(bool b)
    {
        this->holdUntilMouseUp = b;
        return this;
    }

    void UIButton::setBackgroundImageIndex(int i)
    {
        //static_cast<vel::DiffuseSingleSelectableMaterial*>(this->buttonActor->getMaterial())->activeIndex = i;
    }

    UIButton* UIButton::addBackgroundImage(const std::string& i, bool filter)
    {
        this->backgroundImages.push_back(std::pair<std::string, bool>(i, filter));
        return this;
    }

    UIButton* UIButton::setWidth(int w)
    {
        this->width = w;
        return this;
    }

    UIButton* UIButton::setHeight(int h)
    {
        this->height = h;
        return this;
    }

    int UIButton::getWidth() const
    {
        if (!this->isInitialized)
            return this->width;

        return std::round(this->parentView->scene->getActorWorldAABB(this->parentView->scene->getActor(this->buttonActor)).getSize().x);
    }

    int UIButton::getHeight() const
    {
        if (!this->isInitialized)
            return this->height;

        return std::round(this->parentView->scene->getActorWorldAABB(this->parentView->scene->getActor(this->buttonActor)).getSize().y);
    }

    bool UIButton::getIsMouseOver() const
    {
        return this->isMouseOver;
    }

    bool UIButton::isMouseOverSet() const
    {
        if (!this->mouseOverEvent)
            return false;

        return true;
    }

    bool UIButton::isMouseOutSet() const
    {
        if (!this->mouseOutEvent)
            return false;

        return true;
    }

    bool UIButton::isMouseUpSet() const
    {
        if (!this->mouseUpEvent)
            return false;

        return true;
    }

    bool UIButton::isMouseDownSet() const
    {
        if (!this->mouseDownEvent)
            return false;

        return true;
    }

    UIButton* UIButton::setVisible(bool b)
    {
        UIElement::setVisible(b);

        if (!this->isInitialized)
            return this;

        Actor& a = this->parentView->scene->getActor(this->buttonActor);
        a.flags = (a.flags & ~ACTFLG_VISIBLE) | (b ? ACTFLG_VISIBLE : 0);

        return this;
    }

    UIButton* UIButton::setPosition(glm::vec2 pos)
    {
        UIElement::setPosition(pos);

        if (!this->isInitialized)
            return this;

        Actor& a = this->parentView->scene->getActor(this->buttonActor);
        a.setTranslation({ pos , this->zIndex });

        return this;
    }

    UIButton* UIButton::setZIndex(float z)
    {
        UIElement::setZIndex(z);
        return this;
    }

    UIButton* UIButton::setMouseOverEvent(std::function<void()> f)
    {
        this->mouseOverEvent = std::move(f);
        return this;
    }

    UIButton* UIButton::setMouseOutEvent(std::function<void()> f)
    {
        this->mouseOutEvent = std::move(f);
        return this;
    }

    UIButton* UIButton::setMouseUpEvent(std::function<void()> f)
    {
        this->mouseUpEvent = std::move(f);
        return this;
    }

    UIButton* UIButton::setMouseDownEvent(std::function<void()> f)
    {
        this->mouseDownEvent = std::move(f);
        return this;
    }

    void UIButton::setMouseOver(bool b)
    {
        if (b && !this->isMouseOver)
        {
            this->initialMouseOver = true;
            this->isMouseOver = true;
            this->isMouseOut = false;

            return;
        }

        if (!b && this->isMouseOver)
        {
            this->initialMouseOut = true;
            this->isMouseOut = true;
            this->isMouseOver = false;
            this->isMouseDown = false;
            this->isMouseUp = true;
        }
    }

    void UIButton::setMouseDown(bool b)
    {
        if (b && !this->isMouseDown)
        {
            this->initialMouseDown = true;
            this->isMouseDown = true;
            this->isMouseUp = false;

            return;
        }

        if (!b && this->isMouseDown)
        {
            this->initialMouseUp = true;
            this->isMouseUp = true;
            this->isMouseDown = false;
        }
    }

    bool UIButton::containsPoint(glm::vec2 p)
    {
        if (!this->getIsInitialized())
            return false;

        if (!this->isVisible)
            return false;

        if(this->parentView->scene->actorContainsPoint(this->buttonActor, p))
            return false;

        return true;
    }

    void UIButton::update(float dt, const vel::InputState& is, UICursor* c)
    {
        if (!this->isVisible)
            return;

        if (this == c->overElement)
        {
            this->setMouseOver(true);

            if (is.mouseLeftButton)
                this->setMouseDown(true);
            else
                this->setMouseDown(false);
        }
        else if (this != c->overElement && !c->holdInterrupt)
        {
            this->setMouseOver(false);
        }


        if (this->initialMouseOver)
        {
            this->initialMouseOver = false;

            if (this->mouseOverEvent)
                this->mouseOverEvent();
        }

        if (this->initialMouseOut)
        {
            this->initialMouseOut = false;

            if (this->mouseOutEvent)
                this->mouseOutEvent();
        }

        if (this->initialMouseUp)
        {
            this->initialMouseUp = false;

            if (this == c->overElement && c->holdInterrupt)
                c->holdInterrupt = false;

            if (this->mouseUpEvent)
                this->mouseUpEvent();
        }

        if (this->initialMouseDown)
        {
            this->initialMouseDown = false;

            if (this == c->overElement && this->holdUntilMouseUp)
                c->holdInterrupt = true;

            if (this->mouseDownEvent)
                this->mouseDownEvent();
        }
    }
}