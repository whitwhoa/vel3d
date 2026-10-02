

#include <vel/Scene/UI/UIView.h>
#include <vel/Scene/UI/UIScrollView.h>

namespace vel
{
	UIView::UIView(Scene* scene, Stage* s, Camera* camera) :
		scene(scene),
		stage(s),
		camera(camera),
		initializedElements(0)
	{
		this->stage->cameras.push_back(camera);
	}

	UIView::UIView() :
		scene(nullptr),
		stage(nullptr),
		camera(nullptr),
		initializedElements(0)
	{}

	void UIView::addStage(Scene* scene, Stage* s, Camera* camera)
	{
		this->scene = scene;
		this->stage = s;
		this->camera = camera;

		this->stage->cameras.push_back(this->camera);
	}

	UITextButton* UIView::addTextButton(const std::string& id)
	{
		std::unique_ptr<UITextButton> textButton = std::make_unique<UITextButton>(id);
		UITextButton* textButtonRaw = textButton.get();
		textButtonRaw->setParentView(this);
		this->elements.push_back(std::move(textButton));

		return textButtonRaw;
	}

	UISelect* UIView::addSelect(const std::string& id)
	{
		std::unique_ptr<UISelect> selectButton = std::make_unique<UISelect>(id);
		UISelect* selectButtonRaw = selectButton.get();
		selectButtonRaw->setParentView(this);
		this->elements.push_back(std::move(selectButton));

		return selectButtonRaw;
	}

	UICheckbox* UIView::addCheckbox(const std::string& id)
	{
		std::unique_ptr<UICheckbox> checkbox = std::make_unique<UICheckbox>(id);
		UICheckbox* checkboxRaw = checkbox.get();
		checkboxRaw->setParentView(this);
		this->elements.push_back(std::move(checkbox));

		return checkboxRaw;
	}

	UITextInput* UIView::addTextInput(const std::string& id)
	{
		std::unique_ptr<UITextInput> textInput = std::make_unique<UITextInput>(id);
		UITextInput* textInputRaw = textInput.get();
		textInputRaw->setParentView(this);
		this->elements.push_back(std::move(textInput));

		return textInputRaw;
	}

	UIText* UIView::addText(const std::string& id)
	{
		std::unique_ptr<UIText> text = std::make_unique<UIText>(id);
		UIText* textRaw = text.get();
		textRaw->setParentView(this);
		this->elements.push_back(std::move(text));

		return textRaw;
	}

	UIImage* UIView::addImage(const std::string& id)
	{
		std::unique_ptr<UIImage> image = std::make_unique<UIImage>(id);
		UIImage* imageRaw = image.get();
		imageRaw->setParentView(this);
		this->elements.push_back(std::move(image));

		return imageRaw;
	}

	UIButton* UIView::addButton(const std::string& id)
	{
		std::unique_ptr<UIButton> button = std::make_unique<UIButton>(id);
		UIButton* buttonRaw = button.get();
		buttonRaw->setParentView(this);
		this->elements.push_back(std::move(button));

		return buttonRaw;
	}

	UITable* UIView::addTable(const std::string& id)
	{
		std::unique_ptr<UITable> table = std::make_unique<UITable>(id);
		UITable* tableRaw = table.get();
		tableRaw->setParentView(this);
		this->elements.push_back(std::move(table));

		return tableRaw;
	}

	UIScrollView* UIView::addScrollView(const std::string& id)
	{
		std::unique_ptr<UIScrollView> scrollView = std::make_unique<UIScrollView>(id);
		UIScrollView* scrollViewRaw = scrollView.get();
		scrollViewRaw->setParentView(this);
		this->elements.push_back(std::move(scrollView));

		return scrollViewRaw;
	}

	int	UIView::getElementIndex(const std::string& id) const
	{
		for (int i = 0; i < this->elements.size(); i++)
			if (this->elements[i]->getId() == id)
				return i;

		return -1;
	}

	const UIElement* UIView::getElementByIndex(int i) const
	{
		if (i < 0 || i > this->elements.size() - 1)
			return nullptr;

		return this->elements[i].get();
	}

	void UIView::removeElementByIndex(int i)
	{
		if (i < 0 || i >= this->elements.size())
			return;

		if (i != this->elements.size() - 1)
			this->elements[i] = std::move(this->elements.back());

		this->elements.pop_back();
	}
}

