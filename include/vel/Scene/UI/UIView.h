#pragma once

#include <vel/Scene/Scene.h>
#include <vel/Scene/UI/UIButton.h>
#include <vel/Scene/UI/UICheckbox.h>
#include <vel/Scene/UI/UISelect.h>
#include <vel/Scene/UI/UITextInput.h>
#include <vel/Scene/UI/UIText.h>
#include <vel/Scene/UI/UIImage.h>
#include <vel/Scene/UI/UITable.h>


namespace vel
{
	class UIScrollView;

	class UIView
	{
	protected:
		friend class UIScene;

		std::vector<std::unique_ptr<UIElement>>	elements;
		int initializedElements;

	public:
		Scene* scene;
		Stage* stage;
		Camera* camera;
		UIView(Scene* scene, Stage* s, Camera* camera);
		UIView();

		void addStage(Scene* scene, Stage* s, Camera* camera);

		UITextButton* addTextButton(const std::string& id);
		UISelect* addSelect(const std::string& id);
		UICheckbox* addCheckbox(const std::string& id);
		UITextInput* addTextInput(const std::string& id);
		UIText* addText(const std::string& id);
		UIImage* addImage(const std::string& id);
		UIButton* addButton(const std::string& id);
		UITable* addTable(const std::string& id);
		UIScrollView* addScrollView(const std::string& id);

		int					getElementIndex(const std::string& id) const;
		const UIElement*	getElementByIndex(int i) const;
		void				removeElementByIndex(int i);


	};
}
