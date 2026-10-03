#pragma once

#include <string>
#include <vector>

#include <vel/Scene/UI/UITextButton.h>

namespace vel
{
	class UISelect : public UIElement
	{
	private:
		friend class Scene;

		bool				isExpanded;
		int					fontSize;
		std::string			fontType;
		int					width;
		int					scrollbarWidth;
		int					dropdownHeight;
		float				handleHeight;
		float				fullExpandedHeight;

		glm::vec4			selectedTextColor;
		glm::vec4			selectedBackgroundColor;
		glm::vec4			optionTextColor;
		glm::vec4			optionTextColorHover;
		glm::vec4			optionBackgroundColor;
		glm::vec4			optionBackgroundColorHover;
		glm::vec4			scrollbarHandleColor;
		glm::vec4			scrollbarHandleColorClick;
		glm::vec4			scrollbarTrackColor;

		int					selectedIndex;
		int					visibleOptions;
		std::vector<std::string> optionCache;


		std::vector<UITextButton*>	optionButtons;
		int							visibleOptionsCursor;
		float						scrollHandleIncrement;
		actor_handle				scrollbarTrackActor;
		actor_handle				optionsGhostActor;
		actor_handle				expandedGhostActor;
		UITextButton*				selectButton;
		UITextButton*				scrollUpButton;
		UITextButton*				scrollDownButton;
		UITextButton*				scrollbarHandleButton;
		float						scrollbarHandleDefaultYPos;

		bool				holdingHandle;
		float				handleDragYPos;

		void				resetVisibleOptionsCursor();
		void				repositionOptions();
		void				displayOptions();
		void				hideOptions();
		void				scrollUp();
		void				scrollDown();
		void				scrollHome();
		bool				cursorOverExpanded(glm::vec2 p) const;
		bool				cursorOverOptions(glm::vec2 p) const;

	public:
		UISelect(const std::string& id);

		UISelect* setFontSize(int size);
		UISelect* setFontType(const std::string& t);
		UISelect* setWidth(int width);
		UISelect* setSelectedTextColor(glm::vec4 c);
		UISelect* setSelectedBackgroundColor(glm::vec4 c);
		UISelect* setOptionTextColor(glm::vec4 c);
		UISelect* setOptionTextColorHover(glm::vec4 c);
		UISelect* setOptionBackgroundColor(glm::vec4 c);
		UISelect* setOptionBackgroundColorHover(glm::vec4 c);
		UISelect* setScrollbarHandleColor(glm::vec4 c);
		UISelect* setScrollbarHandleColorClick(glm::vec4 c);
		UISelect* setScrollbarTrackColor(glm::vec4 c);
		UISelect* setSelectedIndex(int val);
		UISelect* setVisibleOptions(int opts);
		UISelect* addOption(const std::string& opt);
		UISelect* setPosition(glm::vec2 pos) override;
		UISelect* setZIndex(float z) override;
		UISelect* setVisible(bool v) override;
		UISelect* setParent(actor_handle a) override;

		virtual bool	containsPoint(glm::vec2 p) override;
		virtual void	update(float dt, const InputState& is, UICursor* c) override;

		virtual int		getWidth() const override;
		virtual int		getHeight() const override;

		int				getSelectedIndex() const;
	};
}