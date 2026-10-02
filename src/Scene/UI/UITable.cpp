
#include <vel/Scene/UI/UITable.h>

namespace vel
{
	UITable::UITable(const std::string& id) :
		UIElement(id),
		cellPadding(0),
		width(0),
		height(0)
	{
		UIElement::setOriginType(vel::PlaneOrigin::LEFT_TOP);
	}

	bool UITable::containsPoint(glm::vec2 p)
	{
		if (!this->getIsInitialized())
			return false;

		if (!this->isVisible)
			return false;

		Actor& ta = this->parentView->scene->getActor(this->tableActor);

		glm::vec3 tp = ta.getTranslation();

		if (p.x > tp.x && p.x < tp.x + this->width &&
			p.y > tp.y && p.y < tp.y + this->height)
		{
			return true;
		}

		return false;
	}

	void UITable::update(float dt, const vel::InputState* is, UICursor* c)
	{}

	UITable* UITable::setOriginType(vel::PlaneOrigin ot)
	{
		UIElement::setOriginType(ot);

		if (!this->isInitialized)
			return this;

		this->setPosition(this->getPosition());

		return this;
	}

	UITable* UITable::setPosition(glm::vec2 pos)
	{
		UIElement::setPosition(pos);

		if (!this->isInitialized)
			return this;

		Actor& ta = this->parentView->scene->getActor(this->tableActor);

		ta.setTranslation(glm::vec3(pos, this->zIndex));
		return this;
	}

	UITable* UITable::setZIndex(float z)
	{
		UIElement::setZIndex(z);
		return this;
	}

	UITable* UITable::setVisible(bool b)
	{
		UIElement::setVisible(b);

		if (!this->isInitialized)
			return this;

		for (auto& r : this->rows)
			for (auto& c : r)
				c.element->setVisible(b);

		return this;
	}

	UITable* UITable::setParent(actor_handle a)
	{
		UIElement::setParent(a);

		if (!this->isInitialized)
			return this;

		this->parentView->scene->setActorParent(this->tableActor, a);

		return this;
	}

	UITable* UITable::addRow(const std::vector<UITableCell>& row)
	{
		this->rows.push_back(row);

		if (!this->isInitialized)
			return this;

		this->setPosition(this->getPosition());

		return this;
	}

	UITable* UITable::setCellPadding(int p)
	{
		this->cellPadding = p;

		if (!this->isInitialized)
			return this;

		this->setPosition(this->getPosition());

		return this;
	}

	UITable* UITable::setPostInit(std::function<void()> f)
	{
		this->postInit = std::move(f);
		return this;
	}

	int UITable::getWidth() const
	{
		return this->width;
	}

	int UITable::getHeight() const
	{
		return this->height;
	}
}