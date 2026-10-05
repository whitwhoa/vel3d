
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

		actor_handle pointerActorHandle = this->addActor(this->ui->stage, this->getMesh("cursor_plane"),
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

		b->buttonActor = this->addActor(b->parentView->stage, this->getMesh("plane_1x1"), { buttonMaterialHandle }, ACTFLG_VISIBLE);

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

	void Scene::initSelect(UISelect* s)
	{
		//
		// Set initial visibleOptionsCursor value
		//
		s->resetVisibleOptionsCursor();


		//
		// Initialize selectButton (what shows the existing value)
		//
		s->selectButton = s->parentView->addTextButton(s->id + "SelectButton")
			->setFontType(s->fontType)
			->setFontSize(s->fontSize)
			->setWidth(s->width)
			->setText(s->optionCache.at(s->selectedIndex))
			->setTextColor(s->selectedTextColor)
			->setTextColorHover(s->selectedTextColor)
			->setTextColorClick(s->selectedTextColor)
			->setBackgroundColor(s->selectedBackgroundColor)
			->setBackgroundColorHover(s->selectedBackgroundColor)
			->setBackgroundColorClick(s->selectedBackgroundColor)
			->setPosition(s->positionCache)
			->setZIndex(s->zIndex)
			->setTextAlignment(TextButtonAlignment::LEFT)
			->setLeftPadding(this->px(10))
			//->setTopPadding(this->px(10))
			//->setBottomPadding(this->px(10))
			->setMouseDownEvent([s]() {
				if (!s->isExpanded)
					s->displayOptions();
				else
					s->hideOptions();
			});

		this->initTextButton(s->selectButton);



		//
		// Set scrollbar width here, because we need it to calculate option button widths
		//
		s->scrollbarWidth = this->px(15);


		//
		// Initialize optionButtons
		//
		int optionIndex = -1;
		for (auto& opt : s->optionCache)
		{
			optionIndex++;

			UITextButton* btn = s->parentView->addTextButton(s->id + "OptionButton_" + std::to_string(optionIndex))
				->setFontType(s->fontType)
				->setFontSize(s->fontSize)
				->setWidth(s->width - s->scrollbarWidth)
				->setText(opt)
				->setTextColor(s->optionTextColor)
				->setTextColorHover(s->optionTextColorHover)
				->setTextColorClick(s->optionTextColorHover)
				->setBackgroundColor(s->optionBackgroundColor)
				->setBackgroundColorHover(s->optionBackgroundColorHover)
				->setBackgroundColorClick(s->optionBackgroundColorHover)
				->setTextAlignment(TextButtonAlignment::LEFT)
				->setLeftPadding(this->px(10))
				//->setTopPadding(this->px(5))
				//->setBottomPadding(this->px(5))
				->setVisible(false)
				->setZIndex(s->zIndex)
				->setMouseUpEvent([s, optionIndex]() {
					s->selectedIndex = optionIndex;
					s->selectButton->setText(s->optionCache.at(optionIndex));
					s->hideOptions();
				});

			this->initTextButton(btn);
			btn->isScrollable = true;
			s->optionButtons.push_back(btn);
		}

		s->dropdownHeight = s->optionButtons[0]->getHeight() * s->visibleOptions;
		s->fullExpandedHeight = s->selectButton->getHeight() + s->dropdownHeight;


		//
		// Initialize scrollUpButton
		//
		s->scrollUpButton = s->parentView->addTextButton(s->id + "ScrollUpButton")
			->setText("V")
			->setFontSize(this->px(14))
			->setFontType("Teko-Bold")
			->setWidth(s->scrollbarWidth)
			->setHeight(s->scrollbarWidth)
			->setTextColor(s->selectedTextColor)
			->setTextColorHover(s->selectedTextColor)
			->setTextColorClick(s->selectedTextColor)
			->setBackgroundColor(s->selectedBackgroundColor)
			->setBackgroundColorHover(s->selectedBackgroundColor)
			->setBackgroundColorClick(s->scrollbarTrackColor)
			->setZIndex(s->zIndex + 0.002f)
			->setMouseDownEvent([s]() {
				s->scrollUp();
			});

		this->initTextButton(s->scrollUpButton);
		s->scrollUpButton->isScrollable = true;
		this->getTextActor(s->scrollUpButton->textActor).setRotation(180.f, { 0.f, 0.f, 1.f });
		s->scrollUpButton->setVisible(false);


		//
		// Initialize scrollDownButton
		//
		s->scrollDownButton = s->parentView->addTextButton(s->id + "ScrollDownButton")
			->setText("V")
			->setFontSize(this->px(14))
			->setFontType("Teko-Bold")
			->setWidth(s->scrollbarWidth)
			->setHeight(s->scrollbarWidth)
			->setTextColor(s->selectedTextColor)
			->setTextColorHover(s->selectedTextColor)
			->setTextColorClick(s->selectedTextColor)
			->setBackgroundColor(s->selectedBackgroundColor)
			->setBackgroundColorHover(s->selectedBackgroundColor)
			->setBackgroundColorClick(s->scrollbarTrackColor)
			->setZIndex(s->zIndex + 0.002f)
			->setMouseDownEvent([s]() {
				s->scrollDown();
			});

		this->initTextButton(s->scrollDownButton);
		s->scrollDownButton->isScrollable = true;
		s->scrollDownButton->setVisible(false);


		//
		// Initialize scrollbarTrackActor
		//
		s->scrollbarTrackActor = this->addActor(s->parentView->stage, this->getMesh("plane_1x1"), { this->emptyMaterial }, ACTFLG_NONE);

		Actor& sbta = this->actors[s->scrollbarTrackActor];
		sbta.setScale({ s->scrollbarWidth, -s->dropdownHeight, 1.f });
		sbta.colorMultiplier = s->scrollbarTrackColor;


		//
		// Initialize scrollbarHandleButton
		//
		float usableScrollingDistance = s->dropdownHeight - s->scrollDownButton->getHeight() * 2.f;
		s->handleHeight = usableScrollingDistance * ((float)s->visibleOptions / (float)s->optionButtons.size());
		s->scrollHandleIncrement = (usableScrollingDistance - s->handleHeight) / (float)(s->optionButtons.size() - s->visibleOptions);

		s->scrollbarHandleButton = s->parentView->addTextButton(s->id + "ScrollbarHandleButton")
			->setText("")
			->setFontSize(this->px(14))
			->setFontType("Teko-Bold")
			->setWidth(s->scrollbarWidth)
			->setHeight(s->handleHeight)
			->setTextColor(s->selectedTextColor)
			->setTextColorHover(s->selectedTextColor)
			->setTextColorClick(s->selectedTextColor)
			->setBackgroundColor(s->scrollbarTrackColor * glm::vec4(0.8f, 0.8f, 0.8f, 1.f))
			->setBackgroundColorHover(s->scrollbarTrackColor * glm::vec4(0.8f, 0.8f, 0.8f, 1.f))
			->setBackgroundColorClick(s->selectedBackgroundColor)
			->setZIndex(s->zIndex + 0.002f)
			->setHoldUntilMouseUp(true)
			->setMouseDownEvent([this, s]() {
				s->holdingHandle = true;
				s->handleDragYPos = this->uiCursor->getPos().y;
			})
			->setMouseUpEvent([this, s]() {
				s->holdingHandle = false;
				s->handleDragYPos = 0.f;
			})
			->setMouseOutEvent([this, s]() {
				s->holdingHandle = false;
				s->handleDragYPos = 0.f;
			});

		this->initTextButton(s->scrollbarHandleButton);
		s->scrollbarHandleButton->isScrollable = true;
		s->scrollbarHandleButton->setVisible(false);


		//
		// Initialize optionsGhostActor
		//
		s->optionsGhostActor = this->addActor(s->parentView->stage, this->getMesh("plane_1x1"), { this->emptyMaterial }, ACTFLG_NONE);

		Actor& oga = this->actors[s->optionsGhostActor];
		oga.setScale({ s->selectButton->getWidth(), -s->dropdownHeight, 1.f });
		oga.colorMultiplier = { 1.f, 1.f, 1.f, 1.f };


		//
		// Initialize expandedGhostActor
		//
		s->expandedGhostActor = this->addActor(s->parentView->stage, this->getMesh("plane_1x1"), { this->emptyMaterial }, ACTFLG_NONE);

		Actor& ega = this->actors[s->expandedGhostActor];
		ega.setScale({ s->selectButton->getWidth(), -s->fullExpandedHeight, 1.f });
		ega.colorMultiplier = { 0.5f, 0.5f, 0.5f, 0.5f };


		//
		// Set as having been initialized
		//
		s->isInitialized = true;


		//
		// Post initialization calls
		//
		s->setPosition(s->getPosition());
		s->setParent(s->getParent());
	}

	void Scene::initTextInput(UITextInput* i)
	{
		//
		// Initialize text actors
		//
		{
			Text t;
			t.updateText(i->defaultText);
			t.fontBitmap = this->loadFontBitmapVisualHeight(i->fontType, i->fontSize);
			t.originType = PlaneOrigin::LEFT_BOTTOM;

			Mesh* mesh = this->addMesh(std::move(this->loadTextMesh(t)));

			material_handle tMaterialHandle = this->addMaterial(MTLFLG_IS_TEXT | MTLFLG_HAS_TEXTURES | MTLFLG_IS_TRANSPARENT);
			Material& tMaterial = this->materials[tMaterialHandle];
			tMaterial.textures.push_back(t.fontBitmap->texture);

			t.actor = this->addActor(i->parentView->stage, mesh, { tMaterialHandle }, ACTFLG_NONE);
			Actor& ta = this->actors[t.actor];
			ta.colorMultiplier = i->textColor;
			ta.setScale({ 1.0f, -1.0f, 1.0f });

			i->ghostTextActor = this->texts.insert(t);
		}

		{
			Text t;
			t.updateText(i->defaultText);
			t.fontBitmap = this->loadFontBitmapVisualHeight(i->fontType, i->fontSize);
			t.originType = PlaneOrigin::LEFT_BOTTOM;

			Mesh* mesh = this->addMesh(std::move(this->loadTextMesh(t)));

			material_handle tMaterialHandle = this->addMaterial(MTLFLG_IS_TEXT | MTLFLG_HAS_TEXTURES | MTLFLG_IS_TRANSPARENT);
			Material& tMaterial = this->materials[tMaterialHandle];
			tMaterial.textures.push_back(t.fontBitmap->texture);

			t.actor = this->addActor(i->parentView->stage, mesh, { tMaterialHandle }, ACTFLG_VISIBLE);
			Actor& ta = this->actors[t.actor];
			ta.colorMultiplier = i->textColor;
			ta.setScale({ 1.0f, -1.0f, 1.0f });

			i->textActor = this->texts.insert(t);
		}


		//
		// Initialize i->buttonActor
		//
		Text& gta = this->texts[i->ghostTextActor];
		int buttonHeight = gta.logicalHeight + i->topPadding + i->bottomPadding;

		i->buttonActor = this->addActor(i->parentView->stage, this->getMesh("plane_1x1"), { this->emptyMaterial }, ACTFLG_VISIBLE);

		Actor& ba = this->actors[i->buttonActor];
		ba.setScale({ i->width, -buttonHeight, 1.f });
		ba.colorMultiplier = i->backgroundColor;


		//
		// Initialize i->caretActor
		//
		int caretHeight = buttonHeight * 0.8f;

		i->caretActor = this->addActor(i->parentView->stage, this->getMesh("plane_1x1"), { this->emptyMaterial }, ACTFLG_NONE);

		Actor& ca = this->actors[i->caretActor];
		ca.setScale({ 1.f, -caretHeight, 1.f });
		ca.colorMultiplier = i->caretColor;


		//
		// Implement callback logic
		//
		i->setMouseOverEvent([i, this]() {
			this->getActor(this->uiCursor->textSelectActor).colorMultiplier = i->cursorColor;
			this->uiCursor->showTextSelect();
		});

		i->setMouseOutEvent([this]() {
			this->uiCursor->showPointer();
		});

		i->setMouseDownEvent([i, this]() {

			i->hasFocus = true;

			Text& gt = this->getText(i->ghostTextActor);
			Text& t = this->getText(i->textActor);

			if (gt.text == i->defaultText)
			{
				i->caretIndex = 0;
				gt.updateText("");
				t.updateText("");
				return;
			}

			if (gt.caretPositions.size() > 1)
			{
				Actor& ta = this->getTextActor(i->textActor);

				float textStartWorldX = ta.getTranslation().x;
				if (i->getParent())
					textStartWorldX += this->getActor(i->getParent()).getTranslation().x;

				float clickWorldX = this->uiCursor->getPos().x;
				float clickLocalX = clickWorldX - textStartWorldX;

				int caretIndex = 0;
				for (; caretIndex < gt.caretPositions.size() - 1; caretIndex++)
					if (gt.caretPositions.at(caretIndex).x >= i->viewStartX + clickLocalX)
						break;

				i->caretIndex = caretIndex;
			}

		});


		//
		// Set as having been initialized
		//
		i->isInitialized = true;


		//
		// Initialize view range and caret values
		//
		i->viewWidth = i->width - (i->leftPadding + i->defaultLeftPadding);
		i->viewStartX = 0;
		i->viewEndX = i->viewWidth;


		//
		// Post initialization calls
		//
		i->setPosition(i->getPosition());
		i->setVisible(i->getIsVisible());
		i->setParent(i->getParent());
	}

	void Scene::initTable(UITable* t)
	{
		t->tableActor = HeadlessScene::addActor();

		//
		// Find each row height, and each column width
		//
		std::vector<int> rowHeights;
		std::vector<int> columnWidths;

		for (auto& row : t->rows)
		{
			int rowHeight = 0;
			int cellIndex = 0;
			for (auto& cell : row)
			{
				int cellWidth = cell.element->getWidth() + t->cellPadding * 2;

				if (columnWidths.size() >= cellIndex + 1)
				{
					if (columnWidths.at(cellIndex) < cellWidth)
						columnWidths.at(cellIndex) = cellWidth;
				}
				else
				{
					columnWidths.push_back(cellWidth);
				}

				if (cell.element->getHeight() + t->cellPadding * 2 > rowHeight)
					rowHeight = cell.element->getHeight() + t->cellPadding * 2;

				cellIndex++;
			}
			rowHeights.push_back(rowHeight);
			t->height += rowHeight;
		}

		for (auto& w : columnWidths)
			t->width += w;


		//
		// Position all elements relative to 0,0 (top left) using rowHeight and
		// columnWidth, along with the origin type, width, height, and t->cellPadding
		//
		int rowIndex = 0;
		for (auto& row : t->rows)
		{
			int preRowHeight = 0;
			for (int i = rowIndex - 1; i >= 0; i--)
				preRowHeight += rowHeights[i];

			int cellIndex = 0;
			for (auto& cell : row)
			{
				int preCellWidth = 0;
				for (int i = cellIndex - 1; i >= 0; i--)
					preCellWidth += columnWidths[i];

				glm::vec2 ePos = cell.element->getPosition();

				switch (cell.element->getOriginType())
				{
				case vel::PlaneOrigin::CENTER_BOTTOM:
					ePos.x = std::round(cell.element->getWidth() * 0.5f);
					ePos.y = cell.element->getHeight();
					break;
				case vel::PlaneOrigin::CENTER_CENTER:
					ePos.x = std::round(cell.element->getWidth() * 0.5f);
					ePos.y = std::round(cell.element->getHeight() * 0.5f);
					break;
				case vel::PlaneOrigin::CENTER_TOP:
					ePos.x = std::round(cell.element->getWidth() * 0.5f);
					break;
				case vel::PlaneOrigin::LEFT_BOTTOM:
					ePos.y = cell.element->getHeight();
					break;
				case vel::PlaneOrigin::LEFT_CENTER:
					ePos.y = std::round(cell.element->getHeight() * 0.5f);
					break;
				case vel::PlaneOrigin::LEFT_TOP:
					break;
				case vel::PlaneOrigin::RIGHT_BOTTOM:
					ePos.x = cell.element->getWidth();
					ePos.y = cell.element->getHeight();
					break;
				case vel::PlaneOrigin::RIGHT_CENTER:
					ePos.x = cell.element->getWidth();
					ePos.y = std::round(cell.element->getHeight() * 0.5f);
					break;
				case vel::PlaneOrigin::RIGHT_TOP:
					ePos.x = cell.element->getWidth();
					break;
				}

				// set position such that the element is in the top left corner of it's cell
				ePos.x += preCellWidth;
				ePos.y += preRowHeight;

				// align the element within it's cell
				int cellWidth = columnWidths[cellIndex];
				int cellHeight = rowHeights[rowIndex];
				switch (cell.align)
				{
				case UIColAlign::CENTER_BOTTOM:
					ePos.x += std::round(cellWidth * 0.5f) - std::round(cell.element->getWidth() * 0.5f);
					ePos.y += cell.element->getHeight() - t->cellPadding;
					break;
				case UIColAlign::CENTER_CENTER:
					ePos.x += std::round(cellWidth * 0.5f) - std::round(cell.element->getWidth() * 0.5f);
					ePos.y += std::round(cellHeight * 0.5f) - std::round(cell.element->getHeight() * 0.5f);
					break;
				case UIColAlign::CENTER_TOP:
					ePos.x += std::round(cellWidth * 0.5f) - std::round(cell.element->getWidth() * 0.5f);
					ePos.y += t->cellPadding;
					break;
				case UIColAlign::LEFT_BOTTOM:
					ePos.x += t->cellPadding;
					ePos.y += cell.element->getHeight() - t->cellPadding;
					break;
				case UIColAlign::LEFT_CENTER:
					ePos.x += t->cellPadding;
					ePos.y += std::round(cellHeight * 0.5f) - std::round(cell.element->getHeight() * 0.5f);
					break;
				case UIColAlign::LEFT_TOP:
					ePos.x += t->cellPadding;
					ePos.y += t->cellPadding;
					break;
				case UIColAlign::RIGHT_BOTTOM:
					ePos.x += cellWidth - cell.element->getWidth() - t->cellPadding;
					ePos.y += cellHeight - cell.element->getHeight() - t->cellPadding;
					break;
				case UIColAlign::RIGHT_CENTER:
					ePos.x += cellWidth - cell.element->getWidth() - t->cellPadding;
					ePos.y += std::round(cellHeight * 0.5f) - std::round(cell.element->getHeight() * 0.5f);
					break;
				case UIColAlign::RIGHT_TOP:
					ePos.x += cellWidth - cell.element->getWidth() - t->cellPadding;
					ePos.y += t->cellPadding;
					break;
				}
				cell.element->setPosition(ePos);


				// parent the element to the table so that it's position is relative to the table's position
				cell.element->setParent(t->tableActor);


				cellIndex++;
			}
			rowIndex++;
		}


		//
		// Post initialization
		//
		t->isInitialized = true;
		t->setPosition(t->getPosition());
		t->setVisible(t->getIsVisible());
		t->setParent(t->getParent());
		if (t->postInit)
			t->postInit();
	}

	void Scene::initScrollView(UIScrollView* sv)
	{
		//
		// Initialize scrollbar thickness and padding (may be configurable in future, but for now hardcoded)
		//
		sv->scrollbarThickness = this->px(15);
		sv->scrollContentPadding = this->px(6); // camera allowed to scroll this far past right/bottom of maxX/maxY

		//
		// Set initial usable width/height to provided width/height value
		//
		sv->viewWidth = sv->width;
		sv->viewHeight = sv->height;

		//
		// Setup stage and camera
		//
		vel::Stage* stage = this->addStage(0);
		vel::Camera* cam = this->addCamera(CameraType::SCREEN_SPACE);
		cam->finalRenderCam = false;
		cam->resolutionFixed = true;
		cam->resolution = {sv->viewWidth, sv->viewHeight};
		sv->addStage(this, stage, cam);

		//
		// Initialize all UIElements contained by this UIScrollView
		//
		this->initUI(sv);

		//
		// Find the max width/height of all contained elements, because we need to know how far to scroll,
		// and what size to make the scrollbar handles
		//
		sv->findMaxScrollPos();

		//
		// Find positions of all scroll buttons for use later, if required
		//
		glm::vec2 scrollLeftButtonPos = { 0.f, 0.f };
		glm::vec2 scrollRightButtonPos = { 0.f, 0.f };
		glm::vec2 scrollUpButtonPos = { 0.f, 0.f };
		glm::vec2 scrollDownButtonPos = { 0.f, 0.f };
		sv->setScrollButtonPositions(scrollLeftButtonPos, scrollRightButtonPos, scrollUpButtonPos, scrollDownButtonPos);

		//
		// Parent all elements to ghostParentActor
		//
		sv->ghostParentActor = this->addActor(sv->stage, this->getMesh("cursor_plane"), { this->emptyMaterial }, ACTFLG_NONE);

		for (auto& e : sv->elements)
			if (!e->getParent())
				e->setParent(sv->ghostParentActor);

		//
		// Initialize backgroundActor
		//
		material_handle backgroundMaterialHandle = this->addMaterial(MTLFLG_HAS_TEXTURES);
		Material& backgroundMaterial = this->materials[backgroundMaterialHandle];

		if (sv->backgroundImage)
			backgroundMaterial.textures.push_back(this->loadTexture(sv->backgroundImage.value()));
		else
			backgroundMaterial.textures.push_back(this->loadTexture(Runtime::_config.dataDir + "/textures/defaults/white.png"));

		uint32_t backgroundActorFlags = 0;
		if (sv->backgroundColor || sv->backgroundImage)
			backgroundActorFlags |= ACTFLG_VISIBLE;

		sv->backgroundActor = this->addActor(sv->parentView->stage, this->getMesh("cursor_plane"), { backgroundMaterialHandle }, backgroundActorFlags);
		Actor& ba = this->actors[sv->backgroundActor];
		ba.setScale({ sv->viewWidth, -sv->viewHeight, 1.f });
		if (sv->backgroundColor)
			ba.colorMultiplier = sv->backgroundColor.value();


		//
		// Initialize displayActor
		//
		material_handle displayMaterialHandle = this->addMaterial(MTLFLG_HAS_TEXTURES | MTLFLG_IS_TRANSPARENT);
		Material& displayMaterial = this->materials[displayMaterialHandle];

		displayMaterial.textures.push_back(this->createCameraTexture(sv->camera));

		sv->displayActor = this->addActor(sv->parentView->stage, this->getMesh("plane_1x1_inverted_uv_v_top_left"), { displayMaterialHandle }, ACTFLG_VISIBLE);
		Actor& da = this->actors[sv->displayActor];
		da.setScale({ sv->viewWidth, -sv->viewHeight, 1.f });


		//
		// Initialize scrollbars
		//
		if (sv->requireScrollX || sv->requireScrollY)
		{
			cam->resolution = { sv->viewWidth, sv->viewHeight };

			if (sv->requireScrollX)
			{
				//
				// Initialize scrollbarTrackXActor
				//
				sv->scrollbarTrackXActor = this->addActor(sv->parentView->stage, this->getMesh("plane_1x1"), { this->emptyMaterial }, ACTFLG_VISIBLE);
				Actor& stxa = this->actors[sv->scrollbarTrackXActor];
				stxa.setScale({ sv->width, -sv->scrollbarThickness, 1.f });
				stxa.colorMultiplier = sv->scrollbarTrackColor;


				//
				// Initialize scrollbarXHandleButton
				//
				if (!sv->hideScrollButtons)
					sv->usableXScrollingDistance = sv->viewWidth - sv->scrollbarThickness * 2.f;
				else
					sv->usableXScrollingDistance = sv->viewWidth;

				sv->xHandleMag = sv->usableXScrollingDistance * ((float)sv->viewWidth / (float)sv->maxX);

				float xScrollRange = sv->usableXScrollingDistance - sv->xHandleMag;
				float cameraXScrollRange = sv->maxX - sv->viewWidth;

				sv->cameraScrollXMultiplier = cameraXScrollRange / xScrollRange;

				sv->scrollbarXHandleButton = sv->parentView->addTextButton(sv->id + "ScrollbarXHandleButton")
					->setText("")
					->setFontSize(this->px(14))
					->setFontType("Teko-Bold")
					->setWidth(std::round(sv->xHandleMag))
					->setHeight(sv->scrollbarThickness)
					->setBackgroundColor(sv->scrollbarHandleColor)
					->setBackgroundColorHover(sv->scrollbarHandleColor)
					->setBackgroundColorClick(sv->scrollbarHandleClickColor)
					->setZIndex(sv->zIndex + 0.002f)
					->setHoldUntilMouseUp(true)
					->setMouseDownEvent([this, sv]() {
						sv->holdingXHandle = true;
						sv->handleXDragPos = this->uiCursor->getPos().x;
					})
					->setMouseUpEvent([sv]() {
						sv->holdingXHandle = false;
						sv->handleXDragPos = 0.f;
					})
					->setMouseOutEvent([sv]() {
						sv->holdingXHandle = false;
						sv->handleXDragPos = 0.f;
					});

				this->initTextButton(sv->scrollbarXHandleButton);

				if (!sv->hideScrollButtons)
				{
					//
					// Initialize scrollbar left button
					//
					sv->scrollbarLeftButton = sv->parentView->addTextButton(sv->id + "ScrollbarLeftButton")
						->setText("V")
						->setFontSize(this->px(14))
						->setFontType("Teko-Bold")
						->setWidth(sv->scrollbarThickness)
						->setHeight(sv->scrollbarThickness)
						->setTextColor(sv->scrollbarButtonTextColor)
						->setTextColorHover(sv->scrollbarButtonTextColor)
						->setTextColorClick(sv->scrollbarButtonTextColor)
						->setBackgroundColor(sv->scrollbarButtonColor)
						->setBackgroundColorHover(sv->scrollbarButtonColor)
						->setBackgroundColorClick(sv->scrollbarButtonClickColor)
						->setZIndex(sv->zIndex + 0.002f)
						->setMouseDownEvent([sv]() {
							sv->holdingScrollLeftButton = true;
						})
						->setMouseUpEvent([sv]() {
							sv->holdingScrollLeftButton = false;
						})
						->setMouseOutEvent([sv]() {
							sv->holdingScrollLeftButton = false;
						});

					this->initTextButton(sv->scrollbarLeftButton);
					this->getTextActor(sv->scrollbarLeftButton->textActor).setRotation(90.f, {0.f, 0.f, 1.f});

					//
					// Initialize scrollbar right button
					//
					sv->scrollbarRightButton = sv->parentView->addTextButton(sv->id + "ScrollbarRightButton")
						->setText("V")
						->setFontSize(this->px(14))
						->setFontType("Teko-Bold")
						->setWidth(sv->scrollbarThickness)
						->setHeight(sv->scrollbarThickness)
						->setTextColor(sv->scrollbarButtonTextColor)
						->setTextColorHover(sv->scrollbarButtonTextColor)
						->setTextColorClick(sv->scrollbarButtonTextColor)
						->setBackgroundColor(sv->scrollbarButtonColor)
						->setBackgroundColorHover(sv->scrollbarButtonColor)
						->setBackgroundColorClick(sv->scrollbarButtonClickColor)
						->setZIndex(sv->zIndex + 0.002f)
						->setMouseDownEvent([sv]() {
							sv->holdingScrollRightButton = true;
						})
						->setMouseUpEvent([sv]() {
							sv->holdingScrollRightButton = false;
						})
						->setMouseOutEvent([sv]() {
							sv->holdingScrollRightButton = false;
						});

					this->initTextButton(sv->scrollbarRightButton);
					this->getTextActor(sv->scrollbarRightButton->textActor).setRotation(-90.f, {0.f, 0.f, 1.f});
				}
			}

			if (sv->requireScrollY)
			{
				//
				// Initialize scrollbarTrackYActor
				//
				sv->scrollbarTrackYActor = this->addActor(sv->parentView->stage, this->getMesh("cursor_plane"), { this->emptyMaterial }, ACTFLG_VISIBLE);
				Actor& stya = this->actors[sv->scrollbarTrackYActor];
				stya.setScale({ sv->scrollbarThickness, -sv->viewHeight, 1.f });
				stya.colorMultiplier = sv->scrollbarTrackColor;

				//
				// Initialize scrollbarYHandleButton
				//
				if (!sv->hideScrollButtons)
					sv->usableYScrollingDistance = sv->viewHeight - sv->scrollbarThickness * 2.f;
				else
					sv->usableYScrollingDistance = sv->viewHeight;

				sv->yHandleMag = sv->usableYScrollingDistance * ((float)sv->viewHeight / (float)sv->maxY);

				float yScrollRange = sv->usableYScrollingDistance - sv->yHandleMag;
				float cameraYScrollRange = sv->maxY - sv->viewHeight;

				sv->cameraScrollYMultiplier = cameraYScrollRange / yScrollRange;

				sv->scrollbarYHandleButton = sv->parentView->addTextButton(sv->id + "ScrollbarYHandleButton")
					->setText("")
					->setFontSize(this->px(14))
					->setFontType("Teko-Bold")
					->setWidth(sv->scrollbarThickness)
					->setHeight(std::round(sv->yHandleMag))
					->setBackgroundColor(sv->scrollbarHandleColor)
					->setBackgroundColorHover(sv->scrollbarHandleColor)
					->setBackgroundColorClick(sv->scrollbarHandleClickColor)
					->setZIndex(sv->zIndex + 0.002f)
					->setHoldUntilMouseUp(true)
					->setMouseDownEvent([this, sv]() {
						sv->holdingYHandle = true;
						sv->handleYDragPos = this->uiCursor->getPos().y;
					})
					->setMouseUpEvent([sv]() {
						sv->holdingYHandle = false;
						sv->handleYDragPos = 0.f;
					})
					->setMouseOutEvent([sv]() {
						sv->holdingYHandle = false;
						sv->handleYDragPos = 0.f;
					});

				this->initTextButton(sv->scrollbarYHandleButton);

				if (!sv->hideScrollButtons)
				{
					//
					// Initialize scrollbar up button
					//
					sv->scrollbarUpButton = sv->parentView->addTextButton(sv->id + "ScrollbarUpButton")
						->setText("V")
						->setFontSize(this->px(14))
						->setFontType("Teko-Bold")
						->setWidth(sv->scrollbarThickness)
						->setHeight(sv->scrollbarThickness)
						->setTextColor(sv->scrollbarButtonTextColor)
						->setTextColorHover(sv->scrollbarButtonTextColor)
						->setTextColorClick(sv->scrollbarButtonTextColor)
						->setBackgroundColor(sv->scrollbarButtonColor)
						->setBackgroundColorHover(sv->scrollbarButtonColor)
						->setBackgroundColorClick(sv->scrollbarButtonClickColor)
						->setZIndex(sv->zIndex + 0.002f)
						->setMouseDownEvent([sv]() {
							sv->holdingScrollUpButton = true;
						})
						->setMouseUpEvent([sv]() {
							sv->holdingScrollUpButton = false;
						})
						->setMouseOutEvent([sv]() {
							sv->holdingScrollUpButton = false;
						});

					this->initTextButton(sv->scrollbarUpButton);
					this->getTextActor(sv->scrollbarUpButton->textActor).setRotation(180.f, { 0.f, 0.f, 1.f });

					//
					// Initialize scrollbar down button
					//
					sv->scrollbarDownButton = sv->parentView->addTextButton(sv->id + "ScrollbarDownButton")
						->setText("V")
						->setFontSize(this->px(14))
						->setFontType("Teko-Bold")
						->setWidth(sv->scrollbarThickness)
						->setHeight(sv->scrollbarThickness)
						->setTextColor(sv->scrollbarButtonTextColor)
						->setTextColorHover(sv->scrollbarButtonTextColor)
						->setTextColorClick(sv->scrollbarButtonTextColor)
						->setBackgroundColor(sv->scrollbarButtonColor)
						->setBackgroundColorHover(sv->scrollbarButtonColor)
						->setBackgroundColorClick(sv->scrollbarButtonClickColor)
						->setZIndex(sv->zIndex + 0.002f)
						->setMouseDownEvent([sv]() {
							sv->holdingScrollDownButton = true;
						})
						->setMouseUpEvent([sv]() {
							sv->holdingScrollDownButton = false;
						})
						->setMouseOutEvent([sv]() {
							sv->holdingScrollDownButton = false;
						});

					this->initTextButton(sv->scrollbarDownButton);
				}
			}
		}


		//
		// Post initialization
		//
		sv->isInitialized = true;
		sv->setPosition(sv->getPosition());
		sv->setVisible(sv->getIsVisible());
	}

	void Scene::refreshScrollView(UIScrollView* sv)
	{
		int topOffset = sv->getTopOffset();
		int leftOffset = sv->getLeftOffset();
		bool prevRequireScrollX = sv->requireScrollX;
		bool prevRequireScrollY = sv->requireScrollY;

		this->initUI(sv);

		sv->findMaxScrollPos();

		for (auto& e : sv->elements)
			if (!e->getParent())
				e->setParent(sv->ghostParentActor);

		if (sv->requireScrollX)
		{
			if (!prevRequireScrollX)
			{
				sv->scrollbarTrackXActor = this->addActor(sv->parentView->stage, this->getMesh("plane_1x1"), { this->emptyMaterial }, ACTFLG_VISIBLE);
				Actor& stxa = this->actors[sv->scrollbarTrackXActor];
				stxa.setScale({ sv->width, -sv->scrollbarThickness, 1.f });
				stxa.colorMultiplier = sv->scrollbarTrackColor;
			}

			if (!sv->hideScrollButtons)
				sv->usableXScrollingDistance = sv->viewWidth - sv->scrollbarThickness * 2.f;
			else
				sv->usableXScrollingDistance = sv->viewWidth;

			sv->xHandleMag = sv->usableXScrollingDistance * ((float)sv->viewWidth / (float)sv->maxX);

			float xScrollRange = sv->usableXScrollingDistance - sv->xHandleMag;
			float cameraXScrollRange = sv->maxX - sv->viewWidth;

			sv->cameraScrollXMultiplier = cameraXScrollRange / xScrollRange;

			// build new scrollbarXHandleButton using old version
			UITextButton* refreshedScrollbarXHandleButton = sv->parentView->addTextButton(sv->id + "ScrollbarXHandleButton")
				->setText("")
				->setWidth(std::round(sv->xHandleMag))
				->setHeight(sv->scrollbarThickness)
				->setBackgroundColor(sv->scrollbarHandleColor)
				->setBackgroundColorHover(sv->scrollbarHandleColor)
				->setBackgroundColorClick(sv->scrollbarHandleClickColor)
				->setZIndex(sv->zIndex + 0.002f)
				->setHoldUntilMouseUp(true)
				->setMouseDownEvent([this, sv]() {
					sv->holdingXHandle = true;
					sv->handleXDragPos = this->uiCursor->getPos().x;
				})
				->setMouseUpEvent([sv]() {
					sv->holdingXHandle = false;
					sv->handleXDragPos = 0.f;
				})
				->setMouseOutEvent([sv]() {
					sv->holdingXHandle = false;
					sv->handleXDragPos = 0.f;
				});

			this->initTextButton(refreshedScrollbarXHandleButton);


			if (sv->scrollbarXHandleButton == this->uiCursor->overElement)
			{
				refreshedScrollbarXHandleButton->setMouseOver(true);

				if (Runtime::inputState().mouseLeftButton)
					refreshedScrollbarXHandleButton->setMouseDown(true);
				else
					refreshedScrollbarXHandleButton->setMouseDown(false);

				this->uiCursor->overElement = refreshedScrollbarXHandleButton;
			}


			if (prevRequireScrollX)
			{
				this->actors.erase(sv->scrollbarXHandleButton->buttonActor);

				sv->scrollbarXHandleButton->parentView->removeElementByIndex(
					sv->scrollbarXHandleButton->parentView->getElementIndex(
						sv->scrollbarXHandleButton->getId()));
			}
				

			sv->scrollbarXHandleButton = refreshedScrollbarXHandleButton;


			if (!sv->hideScrollButtons && !prevRequireScrollX)
			{
				sv->scrollbarLeftButton = sv->parentView->addTextButton(sv->id + "ScrollbarLeftButton")
					->setText("V")
					->setFontSize(this->px(14))
					->setFontType("Teko-Bold")
					->setWidth(sv->scrollbarThickness)
					->setHeight(sv->scrollbarThickness)
					->setTextColor(sv->scrollbarButtonTextColor)
					->setTextColorHover(sv->scrollbarButtonTextColor)
					->setTextColorClick(sv->scrollbarButtonTextColor)
					->setBackgroundColor(sv->scrollbarButtonColor)
					->setBackgroundColorHover(sv->scrollbarButtonColor)
					->setBackgroundColorClick(sv->scrollbarButtonClickColor)
					->setZIndex(sv->zIndex + 0.002f)
					->setMouseDownEvent([sv]() {
						sv->holdingScrollLeftButton = true;
					})
					->setMouseUpEvent([sv]() {
						sv->holdingScrollLeftButton = false;
					})
					->setMouseOutEvent([sv]() {
						sv->holdingScrollLeftButton = false;
					});

				this->initTextButton(sv->scrollbarLeftButton);
				this->getTextActor(sv->scrollbarLeftButton->textActor).setRotation(90.f, { 0.f, 0.f, 1.f });


				sv->scrollbarRightButton = sv->parentView->addTextButton(sv->id + "ScrollbarRightButton")
					->setText("V")
					->setFontSize(this->px(14))
					->setFontType("Teko-Bold")
					->setWidth(sv->scrollbarThickness)
					->setHeight(sv->scrollbarThickness)
					->setTextColor(sv->scrollbarButtonTextColor)
					->setTextColorHover(sv->scrollbarButtonTextColor)
					->setTextColorClick(sv->scrollbarButtonTextColor)
					->setBackgroundColor(sv->scrollbarButtonColor)
					->setBackgroundColorHover(sv->scrollbarButtonColor)
					->setBackgroundColorClick(sv->scrollbarButtonClickColor)
					->setZIndex(sv->zIndex + 0.002f)
					->setMouseDownEvent([sv]() {
						sv->holdingScrollRightButton = true;
					})
					->setMouseUpEvent([sv]() {
						sv->holdingScrollRightButton = false;
					})
					->setMouseOutEvent([sv]() {
						sv->holdingScrollRightButton = false;
					});

				this->initTextButton(sv->scrollbarRightButton);
				this->getTextActor(sv->scrollbarRightButton->textActor).setRotation(-90.f, { 0.f, 0.f, 1.f });
			}
		}

		if (sv->requireScrollY)
		{
			if (!prevRequireScrollY)
			{
				sv->scrollbarTrackYActor = this->addActor(sv->parentView->stage, this->getMesh("cursor_plane"), { this->emptyMaterial }, ACTFLG_VISIBLE);
				Actor& stya = this->actors[sv->scrollbarTrackYActor];
				stya.setScale({ sv->scrollbarThickness, -sv->viewHeight, 1.f });
				stya.colorMultiplier = sv->scrollbarTrackColor;
			}

			if (!sv->hideScrollButtons)
				sv->usableYScrollingDistance = sv->viewHeight - sv->scrollbarThickness * 2.f;
			else
				sv->usableYScrollingDistance = sv->viewHeight;

			sv->yHandleMag = sv->usableYScrollingDistance * ((float)sv->viewHeight / (float)sv->maxY);

			float yScrollRange = sv->usableYScrollingDistance - sv->yHandleMag;
			float cameraYScrollRange = sv->maxY - sv->viewHeight;

			sv->cameraScrollYMultiplier = cameraYScrollRange / yScrollRange;

			UITextButton* refreshedScrollbarYHandleButton = sv->parentView->addTextButton(sv->id + "ScrollbarYHandleButton")
				->setText("")
				->setWidth(sv->scrollbarThickness)
				->setHeight(std::round(sv->yHandleMag))
				->setBackgroundColor(sv->scrollbarHandleColor)
				->setBackgroundColorHover(sv->scrollbarHandleColor)
				->setBackgroundColorClick(sv->scrollbarHandleClickColor)
				->setZIndex(sv->zIndex + 0.002f)
				->setHoldUntilMouseUp(true)
				->setMouseDownEvent([this, sv]() {
					sv->holdingYHandle = true;
					sv->handleYDragPos = this->uiCursor->getPos().y;
				})
				->setMouseUpEvent([sv]() {
					sv->holdingYHandle = false;
					sv->handleYDragPos = 0.f;
				})
				->setMouseOutEvent([sv]() {
					sv->holdingYHandle = false;
					sv->handleYDragPos = 0.f;
				});

			this->initTextButton(refreshedScrollbarYHandleButton);


			if (sv->scrollbarYHandleButton == this->uiCursor->overElement)
			{
				refreshedScrollbarYHandleButton->setMouseOver(true);

				if (Runtime::inputState().mouseLeftButton)
					refreshedScrollbarYHandleButton->setMouseDown(true);
				else
					refreshedScrollbarYHandleButton->setMouseDown(false);

				this->uiCursor->overElement = refreshedScrollbarYHandleButton;
			}


			if (prevRequireScrollY)
			{
				this->actors.erase(sv->scrollbarYHandleButton->buttonActor);

				sv->scrollbarYHandleButton->parentView->removeElementByIndex(
					sv->scrollbarYHandleButton->parentView->getElementIndex(
						sv->scrollbarYHandleButton->getId()));
			}


			sv->scrollbarYHandleButton = refreshedScrollbarYHandleButton;


			if (!sv->hideScrollButtons && !prevRequireScrollY)
			{
				sv->scrollbarUpButton = sv->parentView->addTextButton(sv->id + "ScrollbarUpButton")
					->setText("V")
					->setFontSize(this->px(14))
					->setFontType("Teko-Bold")
					->setWidth(sv->scrollbarThickness)
					->setHeight(sv->scrollbarThickness)
					->setTextColor(sv->scrollbarButtonTextColor)
					->setTextColorHover(sv->scrollbarButtonTextColor)
					->setTextColorClick(sv->scrollbarButtonTextColor)
					->setBackgroundColor(sv->scrollbarButtonColor)
					->setBackgroundColorHover(sv->scrollbarButtonColor)
					->setBackgroundColorClick(sv->scrollbarButtonClickColor)
					->setZIndex(sv->zIndex + 0.002f)
					->setMouseDownEvent([sv]() {
						sv->holdingScrollUpButton = true;
					})
					->setMouseUpEvent([sv]() {
						sv->holdingScrollUpButton = false;
					})
					->setMouseOutEvent([sv]() {
						sv->holdingScrollUpButton = false;
					});

				this->initTextButton(sv->scrollbarUpButton);
				this->getTextActor(sv->scrollbarUpButton->textActor).setRotation(180.f, { 0.f, 0.f, 1.f });


				sv->scrollbarDownButton = sv->parentView->addTextButton(sv->id + "ScrollbarDownButton")
					->setText("V")
					->setFontSize(this->px(14))
					->setFontType("Teko-Bold")
					->setWidth(sv->scrollbarThickness)
					->setHeight(sv->scrollbarThickness)
					->setTextColor(sv->scrollbarButtonTextColor)
					->setTextColorHover(sv->scrollbarButtonTextColor)
					->setTextColorClick(sv->scrollbarButtonTextColor)
					->setBackgroundColor(sv->scrollbarButtonColor)
					->setBackgroundColorHover(sv->scrollbarButtonColor)
					->setBackgroundColorClick(sv->scrollbarButtonClickColor)
					->setZIndex(sv->zIndex + 0.002f)
					->setMouseDownEvent([sv]() {
						sv->holdingScrollDownButton = true;
					})
					->setMouseUpEvent([sv]() {
						sv->holdingScrollDownButton = false;
					})
					->setMouseOutEvent([sv]() {
						sv->holdingScrollDownButton = false;
					});

				this->initTextButton(sv->scrollbarDownButton);
			}
		}

		sv->setPosition(sv->getPosition());
		sv->setVisible(sv->getIsVisible());
		sv->initializedElements = sv->elements.size();


		/*
		> updatedScrollbarYPos = :
				> if scroll buttons are present:
					> sv->position().y + scrollButtonThickness + (topOffset / cameraScrollYMultiplier) + (yHandleMag * 0.5f)
				> if scroll buttons are NOT present:
					> sv->position().y + (topOffset / cameraScrollYMultiplier) + (yHandleMag * 0.5f)
		*/
		float updatedScrollbarYPos = 0.f;
		float updatedScrollbarXPos = 0.f;
		if (sv->hideScrollButtons)
		{
			updatedScrollbarYPos = sv->getPosition().y +
				(topOffset / sv->cameraScrollYMultiplier) + (sv->yHandleMag * 0.5f);

			updatedScrollbarXPos = sv->getPosition().x +
				(leftOffset / sv->cameraScrollXMultiplier) + (sv->xHandleMag * 0.5f);
		}
		else
		{
			updatedScrollbarYPos = sv->getPosition().y + sv->scrollbarThickness +
				(topOffset / sv->cameraScrollYMultiplier) + (sv->yHandleMag * 0.5f);

			updatedScrollbarXPos = sv->getPosition().x + sv->scrollbarThickness +
				(leftOffset / sv->cameraScrollXMultiplier) + (sv->xHandleMag * 0.5f);
		}

		sv->scrollbarYHandleButton->setPosition({
			sv->scrollbarYHandleButton->getPosition().x,
			updatedScrollbarYPos
		});

		sv->scrollbarXHandleButton->setPosition({
			updatedScrollbarXPos,
			sv->scrollbarXHandleButton->getPosition().y,
		});
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