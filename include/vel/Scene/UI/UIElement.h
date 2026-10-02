#pragma once

#include <string>

#include <glm/glm.hpp>

#include <vel/Scene/Actor/Actor.h>
#include <vel/Scene/Mesh/PlaneOrigin.h>
#include <vel/Scene/UI/UIView.h>

namespace vel
{
	class UICursor;
	class InputState;

	class UIElement
	{
	private:
		friend class UIScene;

	protected:
		std::string			id;
		UIView*				parentView;
		actor_handle		parentActorCache;
		glm::vec2			positionCache;
		float				zIndex;
		PlaneOrigin			originType;
		bool				isVisible;

		bool				isScrollable;
		bool				isInitialized;

	public:
		UIElement(const std::string& id);
		virtual ~UIElement() = default;

		virtual UIElement*	setPosition(glm::vec2 pos);
		virtual UIElement*	setZIndex(float z);
		virtual UIElement*	setVisible(bool b);
		virtual UIElement*	setOriginType(PlaneOrigin ot);
		virtual UIElement*	setParent(actor_handle a);
		void				setParentView(UIView* v);

		virtual bool		containsPoint(glm::vec2 p) = 0;
		virtual void		update(float dt, const InputState* is, UICursor* c) = 0;
		virtual int			getWidth() const = 0;
		virtual int			getHeight() const = 0;

		const std::string&	getId() const;
		glm::vec2			getPosition() const;
		float				getZIndex() const;
		PlaneOrigin			getOriginType() const;
		actor_handle		getParent() const;
		bool                getIsVisible() const;
		bool				getIsScrollable() const;
		bool                getIsInitialized() const;
		UIView*				getParentView();

	};
}

