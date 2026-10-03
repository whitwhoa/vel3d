#include "spdlog/spdlog.h"

#include <vel/Scene/UI/UIImage.h>

namespace vel
{
	UIImage::UIImage(const std::string& id) :
		UIElement(id),
		src(""),
		scale(1.f),
		filter(true)
	{}

	UIImage* UIImage::setPosition(glm::vec2 pos)
	{
		UIElement::setPosition(pos);

		if (!this->isInitialized)
			return this;

		this->parentView->scene->getActor(this->imageActor).setTranslation({ pos, this->zIndex });

		return this;
	}

	UIImage* UIImage::setZIndex(float z)
	{
		UIElement::setZIndex(z);
		return this;
	}

	UIImage* UIImage::setVisible(bool b)
	{
		UIElement::setVisible(b);

		if (!this->isInitialized)
			return this;

		Actor& a = this->parentView->scene->getActor(this->imageActor);
		a.flags = (a.flags & ~ACTFLG_VISIBLE) | (b ? ACTFLG_VISIBLE : 0);

		return this;
	}

	UIImage* UIImage::setParent(actor_handle a)
	{
		UIElement::setParent(a);

		if (!this->isInitialized)
			return this;

		this->parentView->scene->setActorParent(this->imageActor, a);

		return this;
	}

	UIImage* UIImage::setSrc(const std::string& src)
	{
		this->src = src;
		return this;
	}

	UIImage* UIImage::setScale(float scale)
	{
		this->scale = scale;

		if (!this->isInitialized)
			return this;

		Scene* s = this->parentView->scene;
		Actor& a = s->getActor(this->imageActor);

		glm::vec3 currentSize = s->getActorWorldAABB(a).getSize();
		currentSize *= scale;
		currentSize.z = 1.f;
		a.setScale(currentSize);

		return this;
	}

	UIImage* UIImage::setSize(int w, int h)
	{
		this->size = std::pair<int, int>(w, h);

		if (!this->isInitialized)
			return this;

		Scene* s = this->parentView->scene;
		Actor& a = s->getActor(this->imageActor);

		a.setScale({ static_cast<float>(w), static_cast<float>(h), 1.f });

		return this;
	}

	UIImage* UIImage::setFilter(bool f)
	{
		this->filter = f;
		return this;
	}

	bool UIImage::containsPoint(glm::vec2 p)
	{
		if (!this->isInitialized)
			return false;

		if (!this->isVisible)
			return false;

		if (!this->parentView->scene->actorContainsPoint(this->imageActor, p))
			return false;

		return true;
	}

	void UIImage::update(float dt, const vel::InputState& is, UICursor* c) {}

	int UIImage::getWidth() const
	{
		if (!this->getIsInitialized())
			return 0;

		Scene* s = this->parentView->scene;
		Actor& a = s->getActor(this->imageActor);

		return std::round(s->getActorWorldAABB(a).getSize().x);
	}

	int UIImage::getHeight() const
	{
		if (!this->getIsInitialized())
			return 0;

		Scene* s = this->parentView->scene;
		Actor& a = s->getActor(this->imageActor);

		return std::round(s->getActorWorldAABB(a).getSize().y);
	}
}