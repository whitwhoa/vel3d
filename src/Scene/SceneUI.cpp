
#include <spdlog/spdlog.h>

#include <vel/Runtime.h>
#include <vel/Scene/Scene.h>
#include <vel/Util/functions.h>
#include <vel/Util/Assert.h>

#include <vel/Scene/UI/UIElement.h>
#include <vel/Scene/UI/UIButton.h>
#include <vel/Scene/UI/UICheckbox.h>
#include <vel/Scene/UI/UICursor.h>
#include <vel/Scene/UI/UIImage.h>
#include <vel/Scene/UI/UIScrollView.h>
#include <vel/Scene/UI/UISelect.h>
#include <vel/Scene/UI/UITable.h>
#include <vel/Scene/UI/UIText.h>
#include <vel/Scene/UI/UITextButton.h>
#include <vel/Scene/UI/UITextInput.h>
#include <vel/Scene/UI/UIView.h>

namespace vel
{
	int Scene::px(int value) const
	{
		return std::round(value * this->uiScale);
	}

	glm::ivec2 Scene::screenCenter() const
	{
		return { std::round(this->uiScreenSize.x * 0.5f), std::round(this->uiScreenSize.y * 0.5f) };
	}

	int Scene::screenLeft() const
	{
		return 0.f;
	}

	int Scene::screenRight() const
	{
		return std::round(this->uiScreenSize.x);
	}

	int Scene::screenTop() const
	{
		return 0.f;
	}

	int Scene::screenBottom() const
	{
		return std::round(this->uiScreenSize.y);
	}

	void Scene::preloadUI()
	{
		this->ui = std::make_unique<UIView>(this, this->addStage(), this->addCamera(CameraType::SCREEN_SPACE));

		this->loadMesh(Runtime::_config.dataDir + "/meshes/plane_1x1.fbx"); // contains object named "plane_1x1"
		this->loadMesh(Runtime::_config.dataDir + "/meshes/plane_1x1_inverted_uv_v_top_left.fbx");
		this->loadMesh(Runtime::_config.dataDir + "/meshes/cursor_plane.fbx"); // contains object named "cursor_plane"
	}

	void Scene::addCursor()
	{
		// Pointer
		material_handle cursorPointerMaterial = this->addMaterial(MTLFLG_IS_ALPHA_CUTOUT | MTLFLG_HAS_TEXTURES);
		this->materials[cursorPointerMaterial].textures.push_back(
			this->loadTexture(Runtime::_config.dataDir + "/textures/defaults/cursor_pointer.png"));

		actor_handle pointerActorHandle = this->addActor(this->ui->stage, this->getMesh("Grid"),
			{ cursorPointerMaterial }, ACTFLG_VISIBLE | ACTFLG_DYNAMIC);

		Actor& pointerActor = this->actors[pointerActorHandle];
		pointerActor.setScale({ 12, -19, 1.f });
		pointerActor.setTranslation({ this->screenCenter().x, this->screenCenter().y, 0.99f });


		// TextSelect
		material_handle textSelectMaterial = this->addMaterial(MTLFLG_IS_ALPHA_MASK | MTLFLG_HAS_TEXTURES);
		this->materials[textSelectMaterial].textures.push_back(
			this->loadTexture(Runtime::_config.dataDir + "/textures/defaults/cursor_text_select.png"));

		actor_handle textSelectActorHandle = this->addActor(this->ui->stage, this->getMesh("plane_1x1"),
			{ textSelectMaterial }, ACTFLG_DYNAMIC);

		Actor& textSelectActor = this->actors[textSelectActorHandle];
		textSelectActor.colorMultiplier = { 0.f, 0.f, 0.f, 1.f };
		textSelectActor.setScale({ 10, -18, 1.f });
		textSelectActor.setTranslation({ this->screenCenter().x, this->screenCenter().y, 0.99f });


		this->uiCursor = std::make_unique<UICursor>(this, pointerActorHandle, textSelectActorHandle);
	}

	void Scene::initText(UIText* t)
	{
		
	}

	void Scene::initUI(UIView* v)
	{
		for (size_t i = 0; i < v->elements.size(); i++)
		{
			UIElement* e = v->elements[i].get();

			if (e->isInitialized)
				continue;

			if (auto* scrollView = dynamic_cast<UIScrollView*>(e))
				this->initScrollView(scrollView);
			else if (auto* select = dynamic_cast<UISelect*>(e))
				this->initSelect(select);
			else if (auto* checkbox = dynamic_cast<UICheckbox*>(e))
				this->initCheckbox(checkbox);
			else if (auto* textBtn = dynamic_cast<UITextButton*>(e))
				this->initTextButton(textBtn);
			else if (auto* textInput = dynamic_cast<UITextInput*>(e))
				this->initTextInput(textInput);
			else if (auto* text = dynamic_cast<UIText*>(e))
				this->initText(text);
			else if (auto* image = dynamic_cast<UIImage*>(e))
				this->initImage(image);
			else if (auto* button = dynamic_cast<UIButton*>(e))
				this->initButton(button);
			else if (auto* table = dynamic_cast<UITable*>(e))
				this->initTable(table);
		}

		v->initializedElements = v->elements.size();
	}

	void Scene::setCursorOver(std::vector<std::unique_ptr<UIElement>>& elements)
	{
		for (auto& e : elements)
		{
			if (!e->isVisible)
				continue;

			if (auto* scrollView = dynamic_cast<UIScrollView*>(e.get()))
			{
				if (scrollView->containsPoint(glm::vec2(this->uiCursor->getPos())))
				{
					this->setCursorOver(scrollView->elements);
					return;
				}
			}

			if (e->containsPoint(glm::vec2(this->uiCursor->getPos())))
			{
				if (e->getZIndex() >= this->uiCursor->overZ)
				{
					this->uiCursor->overZ = e->getZIndex();
					this->uiCursor->overElement = e.get();
				}
			}
		}
	}

	void Scene::updateUI(float dt)
	{
		const InputState& is = Runtime::inputState();
		this->uiCursor->updatePos(is.mouseDX, is.mouseDY);

		if (!this->uiCursor->holdInterrupt)
		{
			this->uiCursor->overZ = -1.f;
			this->uiCursor->overElement = nullptr;
			this->setCursorOver(this->ui->elements);
		}

		//if (this->cursor->overElement)
		//	SPDLOG_TRACE("cursor over: {}", this->cursor->overElement->getId());

		for (size_t i = 0; i < this->ui->elements.size(); i++)
		{
			UIElement* e = this->ui->elements[i].get();

			if (auto* scrollView = dynamic_cast<UIScrollView*>(e))
				if (scrollView->elements.size() > scrollView->initializedElements)
					this->refreshScrollView(scrollView);

			e->update(dt, is, this->uiCursor.get());
		}
	}

}