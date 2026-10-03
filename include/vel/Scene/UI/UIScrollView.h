#pragma once


#include <vel/Scene/Actor/Actor.h>

#include <vel/Scene/UI/UIElement.h>
#include <vel/Scene/UI/UIView.h>

namespace vel
{
	class UIScrollView : public UIView, public UIElement
	{
	private:
		friend class Scene;

		glm::vec4		scrollbarButtonColor;
		glm::vec4		scrollbarButtonClickColor;
		glm::vec4		scrollbarButtonTextColor;
		glm::vec4		scrollbarHandleColor;
		glm::vec4		scrollbarHandleClickColor;
		glm::vec4		scrollbarTrackColor;
		glm::vec3		ghostDefaultPos;
		glm::vec3		ghostMaxPos;
		std::optional<glm::vec4>	backgroundColor;
		std::optional<std::string>	backgroundImage;

		actor_handle ghostParentActor;
		actor_handle backgroundActor;
		actor_handle displayActor;

		actor_handle scrollbarTrackXActor;
		actor_handle scrollbarTrackYActor;

		UITextButton* scrollbarUpButton;
		UITextButton* scrollbarDownButton;
		UITextButton* scrollbarLeftButton;
		UITextButton* scrollbarRightButton;
		UITextButton* scrollbarXHandleButton;
		UITextButton* scrollbarYHandleButton;

		float			handleXStartPos;
		float			handleXEndPos;
		float			handleXDragPos;
		float			handleYStartPos;
		float			handleYEndPos;
		float			handleYDragPos;
		float			usableXScrollingDistance;
		float			xHandleMag;
		float			usableYScrollingDistance;
		float			yHandleMag;

		int				width;
		int				height;
		int				viewWidth;
		int				viewHeight;
		int				maxX;
		int				maxY;
		int				scrollbarThickness;
		int				scrollContentPadding;
		bool			holdingXHandle;
		bool			holdingYHandle;
		bool			holdingScrollUpButton;
		bool			holdingScrollDownButton;
		bool			holdingScrollLeftButton;
		bool			holdingScrollRightButton;
		bool			hideScrollButtons;
		bool			requireScrollX;
		bool			requireScrollY;

		void			findMaxScrollPos();
		void			setScrollButtonPositions(glm::vec2& scrollLeftButtonPos,
			glm::vec2& scrollRightButtonPos, glm::vec2& scrollUpButtonPos,
			glm::vec2& scrollDownButtonPos);

		void			scrollRight(float scrollMag);
		void			scrollLeft(float srollMag);

		void			scrollUp(float scrollMag);
		void			scrollDown(float scrollMag);

		void			updateYDrag(const UICursor* c);
		void			updateXDrag(const UICursor* c);



	public:
		float			cameraScrollYMultiplier;
		float			cameraScrollXMultiplier;

		UIScrollView(const std::string& id);

		virtual UIScrollView* setPosition(glm::vec2 pos) override;
		virtual UIScrollView* setZIndex(float z) override;
		virtual UIScrollView* setVisible(bool b) override;
		virtual UIScrollView* setParent(actor_handle a) override;

		UIScrollView* setWidth(int w);
		UIScrollView* setHeight(int h);
		UIScrollView* setScrollbarButtonColor(glm::vec4 c);
		UIScrollView* setScrollbarButtonClickColor(glm::vec4 c);
		UIScrollView* setScrollbarButtonTextColor(glm::vec4 c);
		UIScrollView* setScrollbarHandleColor(glm::vec4 c);
		UIScrollView* setScrollbarHandleClickColor(glm::vec4 c);
		UIScrollView* setScrollbarTrackColor(glm::vec4 c);
		UIScrollView* setBackgroundColor(glm::vec4 c);
		UIScrollView* setBackgroundImage(const std::string& s);
		UIScrollView* setHideScrollButtons(bool b);


		virtual bool			containsPoint(glm::vec2 p) override;
		virtual int				getWidth() const override;
		virtual int				getHeight() const override;

		virtual void			update(float dt, const InputState& is, UICursor* c) override;


		float					getTopOffset() const;
		float					getLeftOffset() const;

	};
}