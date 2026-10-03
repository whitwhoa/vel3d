#pragma once

#include <vel/Scene/UI/UIElement.h>

namespace vel
{
	using UIColAlign = vel::PlaneOrigin;

	struct UITableCell
	{
		UIElement* element;
		UIColAlign	align;
	};

	class UITable : public UIElement
	{
	private:
		friend class Scene;

		std::vector<std::vector<UITableCell>>	rows;
		actor_handle tableActor;
		int										cellPadding;
		int										width;
		int										height;

		std::function<void()> postInit;

	public:
		UITable(const std::string& id);

		UITable* setPosition(glm::vec2 pos) override;
		UITable* setZIndex(float z) override;
		UITable* setVisible(bool b) override;
		UITable* setOriginType(vel::PlaneOrigin ot) override;
		UITable* setParent(actor_handle a) override;

		UITable* addRow(const std::vector<UITableCell>& row);
		UITable* setCellPadding(int p);
		UITable* setPostInit(std::function<void()> f);

		virtual bool containsPoint(glm::vec2 p) override;
		virtual void update(float dt, const InputState& is, UICursor* c) override;

		virtual int	getWidth() const override;
		virtual int	getHeight() const override;
	};
}