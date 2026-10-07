
#include <vel/Scene/Scene.h>
#include <vel/Scene/UI/UIView.h>
#include <vel/Scene/UI/UIText.h>

namespace vel 
{
	UIText::UIText(const std::string& id) :
		UIElement(id),
		fontSize(10),
		fontType("Arial"),
		color({ 0.f, 0.f, 0.f, 1.f }),
		text("")
	{
		UIElement::setOriginType(vel::PlaneOrigin::LEFT_TOP);
	}

	UIText* UIText::setPosition(glm::vec2 pos)
	{
		UIElement::setPosition(pos);

		if (!this->getIsInitialized())
			return this;

		Actor& ta = this->parentView->scene->getTextActor(this->textActor);
		ta.setTranslation(glm::vec3(pos, this->zIndex));

		return this;
	}

	UIText* UIText::setZIndex(float z)
	{
		UIElement::setZIndex(z);
		return this;
	}

	UIText* UIText::setVisible(bool b)
	{
		UIElement::setVisible(b);

		if (!this->getIsInitialized())
			return this;

		Actor& ta = this->parentView->scene->getTextActor(this->textActor);
		ta.flags = (ta.flags & ~ACTFLG_VISIBLE) | (b ? ACTFLG_VISIBLE : 0);

		return this;
	}

	UIText* UIText::setOriginType(vel::PlaneOrigin ot)
	{
		UIElement::setOriginType(ot);
		return this;
	}

	UIText* UIText::setParent(actor_handle a)
	{
		if (!a)
			return this;

		UIElement::setParent(a);

		if (!this->isInitialized)
			return this;

		Scene* s = this->parentView->scene;
		actor_handle textActorHandle = s->getText(this->textActor).actor;

		this->parentView->scene->setActorParent(textActorHandle, a);

		return this;
	}

	UIText* UIText::setFontSize(int px)
	{
		this->fontSize = px;
		return this;
	}

	UIText* UIText::setFontType(const std::string& t)
	{
		this->fontType = t;
		return this;
	}

	UIText* UIText::setText(const std::string& t)
	{
		this->text = t;

		if (!this->getIsInitialized())
			return this;

		Scene* s = this->parentView->scene;

		Text& text = s->getText(this->textActor);
		text.updateText(t);

		s->updateText(text);

		return this;
	}

	UIText* UIText::setColor(glm::vec4 c)
	{
		this->color = c;
		return this;
	}

	bool UIText::containsPoint(glm::vec2 p)
	{
		if (!this->getIsInitialized())
			return false;

		if (!this->isVisible)
			return false;

		Scene* s = this->parentView->scene;
		Text& text = s->getText(this->textActor);

		if (!this->parentView->scene->actorContainsPoint(text.actor, p))
			return false;

		return true;
	}

	void UIText::update(float dt, const vel::InputState& is, UICursor* c)
	{}

	int UIText::getWidth() const
	{
		if (!this->getIsInitialized())
			return 0;

		
		Scene* s = this->parentView->scene;
		Text& text = s->getText(this->textActor);

		return std::round(text.logicalWidth);
	}

	int UIText::getHeight() const
	{
		if (!this->getIsInitialized())
			return 0;

		Scene* s = this->parentView->scene;

		//Text& text = s->getText(this->textActor);
		//return std::round(text.logicalHeight);

		Actor& ta = s->getTextActor(this->textActor);
		return std::round(s->getActorWorldAABB(ta).getSize().y);
	}

	int UIText::getXPos()
	{
		return std::round(this->positionCache.x);
	}

	int UIText::getYPos()
	{
		return std::round(this->positionCache.y);
	}
}