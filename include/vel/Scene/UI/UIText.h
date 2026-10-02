#pragma once


#include <vel/Scene/UI/UIElement.h>
#include <vel/Scene/Text.h>

namespace vel
{
	class UIText : public UIElement
	{
	private:
		friend class UIScene;

		int							fontSize;
		std::string					fontType;
		glm::vec4					color;
		std::string					text;

		text_handle					textActor;


	public:
		UIText(const std::string& id);

		UIText* setPosition(glm::vec2 pos) override;
		UIText* setZIndex(float z) override;
		UIText* setVisible(bool b) override;
		UIText* setOriginType(vel::PlaneOrigin ot) override;
		UIText* setParent(actor_handle a) override;

		UIText* setFontSize(int size);
		UIText* setFontType(const std::string& t);
		UIText* setText(const std::string& t);
		UIText* setColor(glm::vec4 c);

		virtual bool	containsPoint(glm::vec2 p) override;
		virtual void	update(float dt, const vel::InputState* is, UICursor* c) override;

		virtual int		getWidth() const override;
		virtual int		getHeight() const override;
		int				getXPos();
		int				getYPos();
	};
}