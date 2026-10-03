
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

	void Scene::initText(UIText* uiT)
	{
		Text t;
		t.text = uiT->text;
		t.fontBitmap = this->loadFontBitmapVisualHeight(uiT->fontType, uiT->fontSize);
		t.originType = uiT->originType;

		Mesh* mesh = this->addMesh(std::move(this->loadTextMesh(t)));

		material_handle tMaterialHandle = this->addMaterial(MTLFLG_IS_TEXT | MTLFLG_HAS_TEXTURES | MTLFLG_IS_TRANSPARENT);
		Material& tMaterial = this->materials[tMaterialHandle];
		tMaterial.textures.push_back(t.fontBitmap->texture);

		t.actor = this->addActor(uiT->parentView->stage, mesh, {tMaterialHandle}, ACTFLG_VISIBLE);
		Actor& ta = this->actors[t.actor];
		ta.colorMultiplier = uiT->color;
		ta.setScale({ 1.0f, -1.0f, 1.0f });

		uiT->textActor = this->texts.insert(t);
		uiT->isInitialized = true;

		uiT->setPosition(uiT->getPosition());
		uiT->setVisible(uiT->getIsVisible());
		uiT->setParent(uiT->getParent());
	}

	void Scene::initImage(UIImage* i)
	{
		material_handle iMaterialHandle = this->addMaterial(vel::MTLFLG_HAS_TEXTURES);
		Material& iMaterial = this->materials[iMaterialHandle];

		uint32_t textureFlags = 0;

		if (i->filter)
			textureFlags |= TXTRFLG_FILTER;

		texture_handle iTextureHandle = this->loadTexture(i->src, textureFlags);
		Texture& iTexture = this->textures[iTextureHandle];

		iMaterial.textures.push_back(iTextureHandle);

		i->imageActor = this->addActor(i->parentView->stage, this->getMesh("plane_1x1"), { iMaterialHandle }, ACTFLG_VISIBLE);
		Actor& iActor = this->actors[i->imageActor];

		float width = iTexture.width;
		float height = iTexture.height;

		if (!i->size)
		{
			width *= i->scale;
			height *= i->scale;
		}
		else
		{
			width = i->size->first;
			height = i->size->second;
		}

		iActor.setScale({ width, -height, 1.f });


		i->isInitialized = true;
		i->setPosition(i->getPosition());
		i->setVisible(i->getIsVisible());
		i->setParent(i->getParent());
	}

	void Scene::initButton(UIButton* b)
	{
		material_handle buttonMaterialHandle = this->addMaterial(MTLFLG_HAS_TEXTURES);
		Material& buttonMaterial = this->materials[buttonMaterialHandle];

		for (int i = 0; i < b->backgroundImages.size(); i++)
		{
			auto& bi = b->backgroundImages[i];

			if (bi.second)
				buttonMaterial.textures.push_back(this->loadTexture(bi.first, TXTRFLG_FILTER));
			else
				buttonMaterial.textures.push_back(this->loadTexture(bi.first));
		}

		if(buttonMaterial.textures.size() == 0)
			buttonMaterial.textures.push_back(this->loadTexture(Runtime::_config.dataDir + "/textures/defaults/white.png"));

		b->buttonActor = this->addActor(b->parentView->stage, this->getMesh("plane_1x1"), { buttonMaterialHandle }, ACTFLG_NONE);

		Actor& buttonActor = this->actors[b->buttonActor];
		buttonActor.setScale({ b->width, -b->height, 1.f });


		if (!b->isMouseOverSet())
			b->setMouseOverEvent([]() {});
		if (!b->isMouseOutSet())
			b->setMouseOutEvent([]() {});
		if (!b->isMouseDownSet())
			b->setMouseDownEvent([]() {});
		if (!b->isMouseUpSet())
			b->setMouseUpEvent([]() {});

		b->isInitialized = true;

		b->setPosition(b->getPosition());
		b->setVisible(b->getIsVisible());
		b->setParent(b->getParent());
	}

	void Scene::initTextButton(UITextButton* b)
	{
		Text t;
		t.text = b->text;
		t.fontBitmap = this->loadFontBitmapVisualHeight(b->fontType, b->fontSize);
		t.originType = PlaneOrigin::CENTER_CENTER;

		Mesh* mesh = this->addMesh(std::move(this->loadTextMesh(t)));

		material_handle tMaterialHandle = this->addMaterial(MTLFLG_IS_TEXT | MTLFLG_HAS_TEXTURES | MTLFLG_IS_TRANSPARENT);
		Material& tMaterial = this->materials[tMaterialHandle];
		tMaterial.textures.push_back(t.fontBitmap->texture);

		t.actor = this->addActor(b->parentView->stage, mesh, { tMaterialHandle }, ACTFLG_VISIBLE);
		Actor& ta = this->actors[t.actor];
		ta.colorMultiplier = b->textColor;
		ta.setScale({ 1.0f, -1.0f, 1.0f });

		b->textActor = this->texts.insert(t);


		this->initButton(b);

		b->updateButtonActorSize();

		this->actors[b->buttonActor].colorMultiplier = b->getBackgroundColor();



		b->isInitialized = true;

		b->setPosition(b->getPosition());
		b->setVisible(b->getIsVisible());
		b->setParent(b->getParent());
	}

	void Scene::initCheckbox(UICheckbox* c)
	{
		c->uncheckedButton = c->parentView->addTextButton(c->id + "UncheckedButton")
			->setFontType(c->fontType)
			->setFontSize(c->fontSize)
			->setText("[  ]")
			->setTopPadding(this->px(-4))
			->setBottomPadding(this->px(-6))
			->setTextColor(c->color)
			->setTextColorHover(c->color)
			->setTextColorClick(c->color)
			->setBackgroundColor(c->backgroundColor)
			->setBackgroundColorHover(c->backgroundColor)
			->setBackgroundColorClick(c->backgroundColor)
			->setPosition(c->positionCache)
			->setVisible(!c->isChecked)
			->setMouseUpEvent([c]() {
				c->toggleState();
			});

		this->initTextButton(c->uncheckedButton);


		c->checkedButton = c->parentView->addTextButton(c->id + "CheckedButton")
			->setFontType(c->fontType)
			->setFontSize(c->fontSize)
			->setText("[*]")
			->setTopPadding(this->px(-4))
			->setBottomPadding(this->px(-6))
			->setTextColor(c->color)
			->setTextColorHover(c->color)
			->setTextColorClick(c->color)
			->setBackgroundColor(c->backgroundColor)
			->setBackgroundColorHover(c->backgroundColor)
			->setBackgroundColorClick(c->backgroundColor)
			->setPosition(c->positionCache)
			->setVisible(c->isChecked)
			->setMouseUpEvent([c]() {
				c->toggleState();
			});

		this->initTextButton(c->checkedButton);


		c->isInitialized = true;

		c->setPosition(c->getPosition());
		c->setVisible(c->getIsVisible());
		c->setParent(c->getParent());
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