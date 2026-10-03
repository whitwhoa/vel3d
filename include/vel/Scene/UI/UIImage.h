#pragma once


#include <vel/Scene/UI/UIElement.h>

namespace vel
{
	class UIImage : public UIElement
	{
	private:
		friend class Scene;

		std::string		src;
		actor_handle	imageActor;
		float			scale;
		bool			filter;
		std::optional<std::pair<int, int>> size;

	public:
		UIImage(const std::string& id);

		UIImage* setPosition(glm::vec2 pos) override;
		UIImage* setZIndex(float z) override;
		UIImage* setVisible(bool b) override;
		UIImage* setParent(actor_handle a) override;

		UIImage* setSrc(const std::string& src);
		UIImage* setScale(float scale);
		UIImage* setSize(int w, int h);
		UIImage* setFilter(bool f);


		virtual bool	containsPoint(glm::vec2 p) override;
		virtual void	update(float dt, const InputState& is, UICursor* c) override;

		virtual int		getWidth() const override;
		virtual int		getHeight() const override;

	};
}

