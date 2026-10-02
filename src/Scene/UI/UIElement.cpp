

#include <vel/Scene/UI/UIElement.h>


namespace vel
{
    UIElement::UIElement(const std::string& id) :
        id(id),
        parentView(nullptr),
        positionCache({ 0.f, 0.f }),
        zIndex(0.f),
        originType(PlaneOrigin::CENTER_CENTER),
        isVisible(true),
        isScrollable(false),
        isInitialized(false)
    {}

    void UIElement::setParentView(UIView* v)
    {
        this->parentView = v;
    }

    UIView* UIElement::getParentView()
    {
        return this->parentView;
    }

    bool UIElement::getIsScrollable() const
    {
        return this->isScrollable;
    }

    UIElement* UIElement::setPosition(glm::vec2 pos)
    {
        this->positionCache = pos;
        return this;
    }

    UIElement* UIElement::setZIndex(float z)
    {
        this->zIndex = z;
        return this;
    }

    UIElement* UIElement::setVisible(bool b)
    {
        this->isVisible = b;
        return this;
    }

    UIElement* UIElement::setParent(actor_handle a)
    {
        this->parentActorCache = a;
        return this;
    }

    UIElement* UIElement::setOriginType(PlaneOrigin ot)
    {
        this->originType = ot;
        return this;
    }

    const std::string& UIElement::getId() const
    {
        return this->id;
    }

    glm::vec2 UIElement::getPosition() const
    {
        return this->positionCache;
    }

    float UIElement::getZIndex() const
    {
        return this->zIndex;
    }

    PlaneOrigin UIElement::getOriginType() const
    {
        return this->originType;
    }

    actor_handle UIElement::getParent() const
    {
        return this->parentActorCache;
    }

    bool UIElement::getIsVisible() const
    {
        return this->isVisible;
    }

    bool UIElement::getIsInitialized() const
    {
        return this->isInitialized;
    }
}