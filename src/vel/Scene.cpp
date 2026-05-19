#include <iostream>
#include <fstream>

#include "spdlog/spdlog.h"

#include "glm/gtx/string_cast.hpp"

#include "nlohmann/json.hpp"

#include "vel/MaterialOptions.h"
#include "vel/Scene.h"
#include "vel/Vertex.h"
#include "vel/Texture.h"
#include "vel/CollisionObjectTemplate.h"
#include "vel/functions.h"



using json = nlohmann::json;

namespace vel
{
	Scene::Scene(const std::string& dataDir, GPU* gpu) :
		HeadlessScene(dataDir),
		gpu(gpu),
		sceneRenderTarget(nullptr),
		inputState(nullptr),
		audioDevice(nullptr),
		audioGroupKey(-1),
		animationTime(0.0f),
		screenTint(glm::vec4(1.0f, 1.0f, 1.0f, 0.0f)),
		frameTime(0.0),
		frameRate(0.0)
	{

	}
	
	Scene::~Scene()
	{
		this->freeAssets();
	}

	void Scene::setFrameTime(double ft)
	{
		this->frameTime = ft;
	}

	double Scene::getFrameTime() const
	{
		return this->frameTime;
	}

	void Scene::setFrameRate(double fr)
	{
		this->frameRate = fr;
	}

	double Scene::getFrameRate() const
	{
		return this->frameRate;
	}

	void Scene::initRenderTarget()
	{
		this->sceneRenderTarget = this->gpu->createFinalRenderTarget(
			"finalRenderTarget_" + this->name, 
			this->getWindowSize().x, 
			this->getWindowSize().y
		);
	}

	FinalRenderTarget* Scene::getSceneRenderTarget()
	{
		return this->sceneRenderTarget.get();
	}

	void Scene::loadBGMSound(const std::string& path)
	{
		this->soundsInUse.push_back(this->audioDevice->loadBGM(path));
	}

	bool Scene::loadSFXSound(const std::string& path)
	{
		std::optional<std::string> sfxOpt = this->audioDevice->loadSFX(path);
		if (!sfxOpt)
			return false;

		this->soundsInUse.push_back(sfxOpt.value());
		return true;
	}

	void Scene::setAudioDevice(AudioDevice* ad)
	{
		if (ad == nullptr)
			return;

		this->audioDevice = ad;
		this->audioGroupKey = ad->generateGroupKey();
	}

	int Scene::getAudioDeviceGroupKey()
	{
		return this->audioGroupKey;
	}

	void Scene::setInputState(const InputState* is)
	{
		this->inputState = is;
	}

	glm::ivec2 Scene::getResolution()
	{
		return this->resolution;
	}

	void Scene::setResolution(int x, int y)
	{
		this->resolution = glm::ivec2(x, y);
	}

	glm::ivec2 Scene::getWindowSize()
	{
		return this->windowSize;
	}

	void Scene::setWindowSize(int x, int y)
	{
		this->windowSize = glm::ivec2(x, y);
	}

	Camera* Scene::addCamera(const std::string& name, CameraType type)
	{
		std::unique_ptr<Camera> c = std::make_unique<Camera>(name, type);
		c->setResolution(this->resolution.x, this->resolution.y);

		Camera* cameraPtr = c.get();
		this->cameras.push_back(std::move(c));

		cameraPtr->setGpu(this->gpu);
		cameraPtr->setRenderTarget(this->gpu->createRenderTarget(
			(cameraPtr->getName() + "_RT"),
			cameraPtr->getResolution().x,
			cameraPtr->getResolution().y
		));

		return cameraPtr;
	}

	Camera* Scene::getCamera(const std::string& name)
	{
		for (auto& c : this->cameras)
			if (c->getName() == name)
				return c.get();

		SPDLOG_WARN("Attempting to get camera that does not exist: {}", name);
		return nullptr;
	}

	void Scene::freeAssets()
	{
		SPDLOG_DEBUG("Freeing assets for scene: {}", this->name);
		
		for (auto& pMaterial : this->materialsInUse)
			this->assetManager->removeMaterial(pMaterial);
		
		for (auto& pTexture : this->texturesInUse)
			this->assetManager->removeTexture(pTexture);

		for (auto& pFontBitmap : this->fontBitmapsInUse)
			this->assetManager->removeFontBitmap(pFontBitmap);
		
		for (auto& pMesh : this->meshesInUse)
			this->assetManager->removeMesh(pMesh);
        
		for (auto& pShader : this->shadersInUse)
			this->assetManager->removeShader(pShader);

		for (auto& cw : this->collisionWorlds)
			delete cw;

		for (auto& s : this->soundsInUse)
			this->audioDevice->removeSound(s);

		for (auto& s : this->skeletonsInUse)
			this->assetManager->removeSkeleton(s);

		for (auto& a : this->animationsInUse)
			this->assetManager->removeAnimation(a);


		for (auto& c : this->cameras)
			this->gpu->clearRenderTarget(c->getRenderTarget());
		this->cameras.clear();

		this->gpu->freeFinalRenderTarget(this->sceneRenderTarget.get());
	}

	void Scene::setScreenTint(glm::vec4 c)
	{
		this->screenTint = c;
	}

	void Scene::clearScreenTint()
	{
		this->screenTint = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);
	}

	FontBitmap* Scene::loadFontBitmap(const std::string& fontName, int fontSize, const std::string& fontPath)
	{
		FontBitmap* fb = this->assetManager->loadFontBitmap(fontName, fontSize, fontPath);

		this->fontBitmapsInUse.push_back(fb);

		return fb;
	}

	FontBitmap* Scene::loadFontBitmapVisualHeight(const std::string& fontName, int desiredVisiblePx, const std::string& fontPath)
	{
		FontBitmap* fb = this->assetManager->loadFontBitmapVisualHeight(fontName, desiredVisiblePx, fontPath);

		this->fontBitmapsInUse.push_back(fb);

		return fb;
	}

	Texture* Scene::loadTexture(const std::string& name, const std::string& path, int options)
	{
		Texture* t = this->assetManager->loadTexture(name, path, options);

		this->texturesInUse.push_back(t);

		return t;
	}
	
	void Scene::addShaderInUse(Shader* s)
	{
		this->shadersInUse.push_back(s);
	}

	void Scene::addMaterialInUse(Material* m)
	{
		this->materialsInUse.push_back(m);
	}

	void Scene::setShaderOpts(int opts, std::vector<std::string>& defs, std::string& shaderName)
	{
		if (opts & MTRL_OPT_TRANSLUCENT)
		{
			defs.push_back("HAS_ALPHA");
			shaderName += "Alpha";
		}

		if (opts & MTRL_OPT_CUTOUT)
		{
			defs.push_back("IS_CUTOUT");
			shaderName += "Cutout";
		}
	}

	DiffuseMaterial* Scene::addDiffuseMaterial(const std::string& name, int opts)
	{
		std::vector<std::string> defs = DiffuseMaterial::shaderDefs;
		std::string shaderName = "diffuseMaterialShader";

		this->setShaderOpts(opts, defs, shaderName);

		Shader* diffuseMaterialShader = this->assetManager->loadShader(shaderName, "uber.vert", "", "uber.frag", defs); // returns existing if already loaded
		this->shadersInUse.push_back(diffuseMaterialShader);

		std::unique_ptr<DiffuseMaterial> m = std::make_unique<DiffuseMaterial>(name, diffuseMaterialShader);

		if (opts & MTRL_OPT_TRANSLUCENT)
			m->setHasAlphaChannel(true);

		Material* pMaterial = this->assetManager->addMaterial(std::move(m));
		this->materialsInUse.push_back(pMaterial);

		return static_cast<DiffuseMaterial*>(pMaterial);
	}

	DiffuseLightmapMaterial* Scene::addDiffuseLightmapMaterial(const std::string& name, int opts)
	{
		std::vector<std::string> defs = DiffuseLightmapMaterial::shaderDefs;
		std::string shaderName = "diffuseLightmapMaterialShader";

		this->setShaderOpts(opts, defs, shaderName);

		Shader* diffuseLightmapMaterialShader = this->assetManager->loadShader(shaderName, "uber.vert", "", "uber.frag", defs); // returns existing if already loaded
		this->shadersInUse.push_back(diffuseLightmapMaterialShader);

		std::unique_ptr<DiffuseLightmapMaterial> m = std::make_unique<DiffuseLightmapMaterial>(name, diffuseLightmapMaterialShader);

		if (opts & MTRL_OPT_TRANSLUCENT)
			m->setHasAlphaChannel(true);

		Material* pMaterial = this->assetManager->addMaterial(std::move(m));
		this->materialsInUse.push_back(pMaterial);

		return static_cast<DiffuseLightmapMaterial*>(pMaterial);
	}

	DiffuseAnimatedMaterial* Scene::addDiffuseAnimatedMaterial(const std::string& name, int opts)
	{
		std::vector<std::string> defs = DiffuseAnimatedMaterial::shaderDefs;
		std::string shaderName = "diffuseAnimatedMaterialShader";

		this->setShaderOpts(opts, defs, shaderName);

		Shader* diffuseAnimatedMaterialShader = this->assetManager->loadShader(shaderName, "uber.vert", "", "uber.frag", defs); // returns existing if already loaded
		this->shadersInUse.push_back(diffuseAnimatedMaterialShader);

		std::unique_ptr<DiffuseAnimatedMaterial> m = std::make_unique<DiffuseAnimatedMaterial>(name, diffuseAnimatedMaterialShader);

		if (opts & MTRL_OPT_TRANSLUCENT)
			m->setHasAlphaChannel(true);

		Material* pMaterial = this->assetManager->addMaterial(std::move(m));
		this->materialsInUse.push_back(pMaterial);

		return static_cast<DiffuseAnimatedMaterial*>(pMaterial);
	}

	DiffuseAnimatedLightmapMaterial* Scene::addDiffuseAnimatedLightmapMaterial(const std::string& name, int opts)
	{
		std::vector<std::string> defs = DiffuseAnimatedLightmapMaterial::shaderDefs;
		std::string shaderName = "diffuseAnimatedLightmapMaterialShader";

		this->setShaderOpts(opts, defs, shaderName);

		Shader* diffuseAnimatedLightmapMaterialShader = this->assetManager->loadShader(shaderName, "uber.vert", "", "uber.frag", defs); // returns existing if already loaded
		this->shadersInUse.push_back(diffuseAnimatedLightmapMaterialShader);

		std::unique_ptr<DiffuseAnimatedLightmapMaterial> m = std::make_unique<DiffuseAnimatedLightmapMaterial>(name, diffuseAnimatedLightmapMaterialShader);

		if (opts & MTRL_OPT_TRANSLUCENT)
			m->setHasAlphaChannel(true);

		Material* pMaterial = this->assetManager->addMaterial(std::move(m));
		this->materialsInUse.push_back(pMaterial);

		return static_cast<DiffuseAnimatedLightmapMaterial*>(pMaterial);
	}

	DiffuseSkinnedMaterial* Scene::addDiffuseSkinnedMaterial(const std::string& name, int opts)
	{
		std::vector<std::string> defs = DiffuseSkinnedMaterial::shaderDefs;
		std::string shaderName = "diffuseSkinnedMaterialShader";

		this->setShaderOpts(opts, defs, shaderName);

		Shader* diffuseSkinnedMaterialShader = this->assetManager->loadShader(shaderName, "uber.vert", "", "uber.frag", defs); // returns existing if already loaded
		this->shadersInUse.push_back(diffuseSkinnedMaterialShader);

		std::unique_ptr<DiffuseSkinnedMaterial> m = std::make_unique<DiffuseSkinnedMaterial>(name, diffuseSkinnedMaterialShader);

		if (opts & MTRL_OPT_TRANSLUCENT)
			m->setHasAlphaChannel(true);

		Material* pMaterial = this->assetManager->addMaterial(std::move(m));
		this->materialsInUse.push_back(pMaterial);

		return static_cast<DiffuseSkinnedMaterial*>(pMaterial);
	}

	TextMaterial* Scene::addTextMaterial(const std::string& name, int opts)
	{
		std::vector<std::string> defs = TextMaterial::shaderDefs;
		std::string shaderName = "textMaterialShader";

		this->setShaderOpts(opts, defs, shaderName);

		Shader* textMaterialShader = this->assetManager->loadShader(shaderName, "uber.vert", "", "uber.frag", defs); // returns existing if already loaded
		this->shadersInUse.push_back(textMaterialShader);

		std::unique_ptr<TextMaterial> m = std::make_unique<TextMaterial>(name, textMaterialShader);
		m->setHasAlphaChannel(true);

		Material* pMaterial = this->assetManager->addMaterial(std::move(m));
		this->materialsInUse.push_back(pMaterial);

		return static_cast<TextMaterial*>(pMaterial);
	}

	DiffuseAmbientCubeMaterial* Scene::addDiffuseAmbientCubeMaterial(const std::string& name, int opts)
	{
		std::vector<std::string> defs = DiffuseAmbientCubeMaterial::shaderDefs;
		std::string shaderName = "diffuseAmbientCubeMaterialShader";

		this->setShaderOpts(opts, defs, shaderName);

		Shader* diffuseAmbientCubeMaterialShader = this->assetManager->loadShader(shaderName, "uber.vert", "", "uber.frag", defs); // returns existing if already loaded
		this->shadersInUse.push_back(diffuseAmbientCubeMaterialShader);

		std::unique_ptr<DiffuseAmbientCubeMaterial> m = std::make_unique<DiffuseAmbientCubeMaterial>(name, diffuseAmbientCubeMaterialShader);

		if (opts & MTRL_OPT_TRANSLUCENT)
			m->setHasAlphaChannel(true);

		Material* pMaterial = this->assetManager->addMaterial(std::move(m));
		this->materialsInUse.push_back(pMaterial);

		return static_cast<DiffuseAmbientCubeMaterial*>(pMaterial);
	}

	DiffuseAmbientCubeSkinnedMaterial* Scene::addDiffuseAmbientCubeSkinnedMaterial(const std::string& name, int opts)
	{
		std::vector<std::string> defs = DiffuseAmbientCubeSkinnedMaterial::shaderDefs;
		std::string shaderName = "diffuseAmbientCubeSkinnedMaterialShader";

		this->setShaderOpts(opts, defs, shaderName);

		Shader* diffuseAmbientCubeSkinnedMaterialShader = this->assetManager->loadShader(shaderName, "uber.vert", "", "uber.frag", defs); // returns existing if already loaded
		this->shadersInUse.push_back(diffuseAmbientCubeSkinnedMaterialShader);

		std::unique_ptr<DiffuseAmbientCubeSkinnedMaterial> m = std::make_unique<DiffuseAmbientCubeSkinnedMaterial>(name, diffuseAmbientCubeSkinnedMaterialShader);

		if (opts & MTRL_OPT_TRANSLUCENT)
			m->setHasAlphaChannel(true);

		Material* pMaterial = this->assetManager->addMaterial(std::move(m));
		this->materialsInUse.push_back(pMaterial);

		return static_cast<DiffuseAmbientCubeSkinnedMaterial*>(pMaterial);
	}

	RGBAMaterial* Scene::addRGBAMaterial(const std::string& name, int opts)
	{
		std::vector<std::string> defs = RGBAMaterial::shaderDefs;
		std::string shaderName = "RGBAMaterialShader";

		this->setShaderOpts(opts, defs, shaderName);

		Shader* RGBAMaterialShader = this->assetManager->loadShader(shaderName, "uber.vert", "", "uber.frag", defs); // returns existing if already loaded
		this->shadersInUse.push_back(RGBAMaterialShader);

		std::unique_ptr<RGBAMaterial> m = std::make_unique<RGBAMaterial>(name, RGBAMaterialShader);

		if (opts & MTRL_OPT_TRANSLUCENT)
			m->setHasAlphaChannel(true);

		Material* pMaterial = this->assetManager->addMaterial(std::move(m));
		this->materialsInUse.push_back(pMaterial);

		return static_cast<RGBAMaterial*>(pMaterial);
	}

	RGBALineMaterial* Scene::addRGBALineMaterial(const std::string& name, int opts)
	{
		std::vector<std::string> defs = RGBALineMaterial::shaderDefs;
		std::string shaderName = "RGBALineMaterialShader";

		this->setShaderOpts(opts, defs, shaderName);

		Shader* RGBALineMaterialShader = this->assetManager->loadShader(shaderName, "line.vert", "line.geom", "line.frag", defs); // returns existing if already loaded
		this->shadersInUse.push_back(RGBALineMaterialShader);

		std::unique_ptr<RGBALineMaterial> m = std::make_unique<RGBALineMaterial>(name, RGBALineMaterialShader);

		if (opts & MTRL_OPT_TRANSLUCENT)
			m->setHasAlphaChannel(true);

		Material* pMaterial = this->assetManager->addMaterial(std::move(m));
		this->materialsInUse.push_back(pMaterial);

		return static_cast<RGBALineMaterial*>(pMaterial);
	}

	RGBALightmapMaterial* Scene::addRGBALightmapMaterial(const std::string& name, int opts)
	{
		std::vector<std::string> defs = RGBALightmapMaterial::shaderDefs;
		std::string shaderName = "RGBALightmapMaterialShader";

		this->setShaderOpts(opts, defs, shaderName);

		Shader* RGBALightmapMaterialShader = this->assetManager->loadShader(shaderName, "uber.vert", "", "uber.frag", defs); // returns existing if already loaded
		this->shadersInUse.push_back(RGBALightmapMaterialShader);

		std::unique_ptr<RGBALightmapMaterial> m = std::make_unique<RGBALightmapMaterial>(name, RGBALightmapMaterialShader);

		if (opts & MTRL_OPT_TRANSLUCENT)
			m->setHasAlphaChannel(true);

		Material* pMaterial = this->assetManager->addMaterial(std::move(m));
		this->materialsInUse.push_back(pMaterial);

		return static_cast<RGBALightmapMaterial*>(pMaterial);
	}

	DiffuseCausticMaterial* Scene::addDiffuseCausticMaterial(const std::string& name, int opts)
	{
		std::vector<std::string> defs = DiffuseCausticMaterial::shaderDefs;
		std::string shaderName = "diffuseCausticMaterialShader";

		this->setShaderOpts(opts, defs, shaderName);

		Shader* diffuseCausticMaterialShader = this->assetManager->loadShader(shaderName, "uber.vert", "", "uber.frag", defs); // returns existing if already loaded
		this->shadersInUse.push_back(diffuseCausticMaterialShader);

		std::unique_ptr<DiffuseCausticMaterial> m = std::make_unique<DiffuseCausticMaterial>(name, diffuseCausticMaterialShader);

		if (opts & MTRL_OPT_TRANSLUCENT)
			m->setHasAlphaChannel(true);

		Material* pMaterial = this->assetManager->addMaterial(std::move(m));
		this->materialsInUse.push_back(pMaterial);

		return static_cast<DiffuseCausticMaterial*>(pMaterial);
	}

	DiffuseCausticLightmapMaterial* Scene::addDiffuseCausticLightmapMaterial(const std::string& name, int opts)
	{
		std::vector<std::string> defs = DiffuseCausticLightmapMaterial::shaderDefs;
		std::string shaderName = "diffuseCausticLightmapMaterialShader";

		this->setShaderOpts(opts, defs, shaderName);

		Shader* diffuseCausticLightmapMaterialShader = this->assetManager->loadShader(shaderName, "uber.vert", "", "uber.frag", defs); // returns existing if already loaded
		this->shadersInUse.push_back(diffuseCausticLightmapMaterialShader);

		std::unique_ptr<DiffuseCausticLightmapMaterial> m = std::make_unique<DiffuseCausticLightmapMaterial>(name, diffuseCausticLightmapMaterialShader);

		if (opts & MTRL_OPT_TRANSLUCENT)
			m->setHasAlphaChannel(true);

		Material* pMaterial = this->assetManager->addMaterial(std::move(m));
		this->materialsInUse.push_back(pMaterial);

		return static_cast<DiffuseCausticLightmapMaterial*>(pMaterial);
	}



	Shader* Scene::getShader(const std::string& name)
	{
		return this->assetManager->getShader(name);
	}

	Texture* Scene::getTexture(const std::string& name)
	{
		return this->assetManager->getTexture(name);
	}

	FontBitmap* Scene::getFontBitmap(const std::string& name)
	{
		return this->assetManager->getFontBitmap(name);
	}

	Material* Scene::getMaterial(const std::string& name)
	{
		return this->assetManager->getMaterial(name);
	}


	/* Misc
	--------------------------------------------------*/
	void Scene::lerpAnimators(float alpha)
	{
		for (auto& a : this->animators)
			a->renderLerp(alpha);
	}


	//
	// LineActors
	//

	int Scene::getLineActorIndex(const std::string& name)
	{
		for (int i = 0; i < this->lineActors.size(); i++)
			if (this->lineActors.at(i)->name == name)
				return i;

		return -1;
	}

	int Scene::getLineActorIndex(const LineActor* a)
	{
		for (int i = 0; i < this->lineActors.size(); i++)
			if (this->lineActors.at(i).get() == a)
				return i;

		return -1;
	}

	void Scene::_removeLineActor(int lineActorIndex)
	{
		LineActor* la = this->lineActors.at(lineActorIndex).get();

		this->_removeActor(this->getActorLocation(la->actor));

		this->lineActors.erase(this->lineActors.begin() + lineActorIndex);
	}

	//LineActor* Scene::addLineActor(Stage* stage, const std::string& name, std::vector<glm::vec2> points, glm::vec4 color)
	LineActor* Scene::addLineActor(Stage* stage, const std::string& name, const std::vector<std::tuple<glm::vec2, glm::vec2, unsigned int>>& points, std::vector<glm::vec4> colors)
	{
		// create the LineActor
		std::unique_ptr<LineActor> la = std::make_unique<LineActor>(name);

		// create the mesh
		Mesh* pMesh = this->assetManager->addMesh(std::move(LineActor::segmentsToMesh(name, points)));
		this->meshesInUse.push_back(pMesh);

		bool hasAlpha = false;
		for (auto& c : colors)
		{
			if (c.w < 0.999f)
			{
				hasAlpha = true;
				break;
			}
		}

		// create material
		RGBALineMaterial* pMaterial = this->addRGBALineMaterial(name + "_material", hasAlpha);
		for (size_t i = 0; i < colors.size(); i++)
			pMaterial->setLineColor(i, colors[i]);

		// create actor
		la->actor = this->addActor(name, pMesh, pMaterial);

		this->lineActors.push_back(std::move(la));
		return this->lineActors.back().get();
	}

	LineActor* Scene::addContinuousLineActor(Stage* stage, const std::string& name, const std::vector<glm::vec2>& points, glm::vec4 color)
	{
		std::unique_ptr<LineActor> la = std::make_unique<LineActor>(name);

		Mesh* pMesh = this->assetManager->addMesh(std::move(LineActor::pointsToMesh(name, points)));
		this->meshesInUse.push_back(pMesh);

		bool hasAlpha = color.w < 0.999f;

		RGBALineMaterial* pMaterial = this->addRGBALineMaterial(name + "_material", hasAlpha);
		pMaterial->setLineColor(0, color);

		la->actor = this->addActor(name, pMesh, pMaterial);

		this->lineActors.push_back(std::move(la));
		return this->lineActors.back().get();
	}

	LineActor* Scene::getLineActor(const std::string& name)
	{
		return this->lineActors.at(this->getLineActorIndex(name)).get();
	}

	void Scene::removeLineActor(LineActor* la)
	{
		this->_removeLineActor(this->getLineActorIndex(la));
	}

	void Scene::removeLineActor(const std::string& name)
	{
		this->_removeLineActor(this->getLineActorIndex(name));
	}



	//
	// TextActors
	//

	int Scene::getTextActorIndex(const std::string& name)
	{
		for (int i = 0; i < this->textActors.size(); i++)
			if (this->textActors.at(i)->name == name)
				return i;

		return -1;
	}

	int Scene::getTextActorIndex(const TextActor* a)
	{
		for (int i = 0; i < this->textActors.size(); i++)
			if (this->textActors.at(i).get() == a)
				return i;

		return -1;
	}

	void Scene::_removeTextActor(int textActorIndex)
	{
		TextActor* ta = this->textActors.at(textActorIndex).get();

		this->_removeActor(this->getActorLocation(ta->actor));

		this->textActors.erase(this->textActors.begin() + textActorIndex);
	}

	TextActor* Scene::addTextActor(const std::string& name, const std::string& theText, FontBitmap* fb,
		glm::vec4 color, TextActorOriginType originType)
	{
		std::unique_ptr<TextActor> ta = std::make_unique<TextActor>();
		ta->name = name;
		ta->text = theText;
		ta->fontBitmap = fb;
		ta->originType = originType;

		// create the mesh using provided FontBitmap and text string
		Mesh* pTam = this->assetManager->addMesh(std::move(this->assetManager->loadTextActorMesh(ta.get())));
		this->meshesInUse.push_back(pTam);

		// create material
		Material* taMaterial = this->addTextMaterial(name + "_material");
		taMaterial->addTexture(&fb->texture);
		taMaterial->setColor(color);

		// create actor
		ta->actor = this->addActor(name, pTam, taMaterial);


		this->textActors.push_back(std::move(ta));
		return this->textActors.back().get();
	}

	TextActor* Scene::getTextActor(const std::string& name)
	{
		return this->textActors.at(this->getTextActorIndex(name)).get();
	}

	void Scene::removeTextActor(TextActor* ta)
	{
		this->_removeTextActor(this->getTextActorIndex(ta));
	}

	void Scene::removeTextActor(const std::string& name)
	{
		this->_removeTextActor(this->getTextActorIndex(name));
	}

	void Scene::updateTextActors()
	{
		for (auto& ta : this->textActors)
		{
			if (ta->requiresUpdate)
			{
				// update the mesh data associated with text actor
				std::unique_ptr<Mesh> updatedMesh = std::move(this->assetManager->loadTextActorMesh(ta.get()));
				ta->actor->getMesh()->setVertices(updatedMesh->getVertices());
				ta->actor->getMesh()->setIndices(updatedMesh->getIndices());

				this->assetManager->updateMesh(ta->actor->getMesh());
				ta->requiresUpdate = false;
			}
		}
	}


	//
	// Billboards
	//

	int Scene::getBillboardIndex(const std::string& name)
	{
		for (int i = 0; i < this->billboards.size(); i++)
			if (this->billboards.at(i)->getActor()->getName() == name)
				return i;

		return -1;
	}

	int Scene::getBillboardIndex(const Billboard* a)
	{
		for (int i = 0; i < this->billboards.size(); i++)
			if (this->billboards.at(i).get() == a)
				return i;

		return -1;
	}

	void Scene::_removeBillboard(int billboardIndex)
	{
		Billboard* b = this->billboards.at(billboardIndex).get();

		this->_removeActor(this->getActorLocation(b->getActor()));

		this->billboards.erase(this->billboards.begin() + billboardIndex);
	}

	Billboard* Scene::addBillboard(const std::string& name, vel::Material* material, vel::Camera* parentCamera, float width, float height)
	{
		// generate mesh, send it to gpu, track it for managment by scene
		std::unique_ptr<Mesh> tmpM = std::make_unique<Mesh>(name + "_mesh");
		tmpM->initBillboardQuad(width, height);

		Mesh* m = this->assetManager->addMesh(std::move(tmpM));

		this->meshesInUse.push_back(m);

		// create actor
		Actor* a = this->addActor(name, m, material);
		a->setDynamic(true);

		// create the billboard, return pointer
		this->billboards.emplace_back(std::make_unique<Billboard>(a, parentCamera));

		return this->billboards.back().get();
	}

	Billboard* Scene::addBillboard(const std::string& name, Material* material, Camera* parentCamera, Mesh* mesh)
	{
		this->assetManager->incrementMeshUsage(mesh);
		this->meshesInUse.push_back(mesh);

		Actor* a = this->addActor(name, mesh, material);
		a->setDynamic(true);

		this->billboards.emplace_back(std::make_unique<Billboard>(a, parentCamera));

		return this->billboards.back().get();
	}

	Billboard* Scene::getBillboard(const std::string& name)
	{
		return this->billboards.at(this->getBillboardIndex(name)).get();
	}

	void Scene::removeBillboard(Billboard* b)
	{
		this->_removeBillboard(this->getBillboardIndex(b));
	}

	void Scene::removeBillboard(const std::string& name)
	{
		this->_removeBillboard(this->getBillboardIndex(name));
	}

	void Scene::updateBillboards()
	{
		for (auto& b : this->billboards)
			b->update();
	}















	

	void Scene::updatePreviousTransforms()
	{
		for (auto& pair : this->actors)
			for (auto& actor : pair.second)
				actor->updatePreviousTransform();
	}

	

	void Scene::draw(float frameTime, float alpha)
	{
		bool actorsFirstPass = true;

		for (auto& c : this->cameras)
		{
			// update stage camera (view/projection matrices), update scene's camera data to this stage's camera data
			c->update();
			this->cameraPosition = c->getPosition();
			this->cameraProjectionMatrix = c->getProjectionMatrix();
			this->cameraViewMatrix = c->getViewMatrix();

			gpu->updateCameraViewportSize(c->getResolution().x, c->getResolution().y); // different cameras can have different resolutions
				
			gpu->setRenderTarget(c->getRenderTarget());
				
			gpu->setOpaqueRenderState();

			bool foundFirstAlpha = false;

			for (auto& pair : this->getActors())
			{
				for (auto& a : pair.second)
				{
					if (!a->getMesh() || !a->isVisible() || !a->getMaterial()->getShader())
						continue;

					if (a->getMaterial()->getHasAlphaChannel() && !foundFirstAlpha)
					{
						foundFirstAlpha = true;
						gpu->setAlphaRenderState();
					}

					if (actorsFirstPass)
						a->getMaterial()->preDraw(frameTime);

					gpu->useShader(a->getMaterial()->getShader()); // only alters gpu state if necessary
					gpu->useMesh(a->getMesh()); // only alters gpu state if necessary
					gpu->setActiveMaterial(a->getMaterial());

					a->getMaterial()->draw(alpha, gpu, a.get(), this->cameraViewMatrix, this->cameraProjectionMatrix);

				}
			}



			actorsFirstPass = false;

			gpu->composeFBOs();



		} // end for each camera


		// all stage camera's framebuffers are now updated, loop through each stage camera and check if it should display it's contents 

		// now bind the scene's FinalRenderTarget. It's viewport size should always be the full size of the window, or screen in fullscreen mode
		std::unique_ptr<FinalRenderTarget> updatedFRT =  gpu->updateFinalRenderTargetVPSize(
			this->sceneRenderTarget.get(), 
			this->getWindowSize().x, 
			this->getWindowSize().y
		);

		if (updatedFRT)
			this->sceneRenderTarget = std::move(updatedFRT);


		gpu->setFinalRenderTarget(this->sceneRenderTarget.get());


		for (auto& c : this->cameras)
			if (c->isFinalRenderCam())
				gpu->drawToFinalRenderTarget(c->getRenderTarget()->opaqueTexture.frames.at(0).dsaHandle);
		

		// call post process to apply post process shader while drawing into the default framebuffer for display to screen
		gpu->setDefaultFrameBuffer();
		gpu->drawToScreen(this->sceneRenderTarget.get(), this->screenTint);

		// If you don't set glviewport back to the scene's render resolution (vs leaving it at the window resolution), mouse
		// movement gets jacked up (scene resolution is not the same as the resolution value in the sceneRenderTarget, resolution
		// in that object is for tracking the last rendered resolution of that framebuffer which should always be the size of the
		// window (or screen in fullsreen mode) so that all previously rendered camera textures encompass the entire window)
		gpu->setViewportSize(this->resolution.x, this->resolution.y);


		// moving collision debug draw event as final thing as it draws directly to the screen buffer, and I don't want to have to 
		// think about updating it right now
//#ifdef DEBUG_LOG
		for (auto& cw : this->collisionWorlds)
		{
			if (cw->getIsActive() && cw->getDebugDrawer() != nullptr)
			{
				cw->getDynamicsWorld()->debugDrawWorld(); // load vertices into associated CollisionDebugDrawer
				gpu->useShader(cw->getDebugDrawer()->getShaderProgram());
				gpu->setShaderMat4("vp", cw->getCamera()->getProjectionMatrix() * cw->getCamera()->getViewMatrix());
				gpu->debugDrawCollisionWorld(cw->getDebugDrawer()); // draw all loaded vertices with a single call and clear
			}
		}
//#endif
	}

	void Scene::clearAllRenderTargetBuffers(GPU* gpu)
	{
		for (auto& c : this->cameras)
		{
			gpu->setRenderTarget(c->getRenderTarget());
			gpu->clearRenderTargetBuffers(0.0f, 0.0f, 0.0f, 0.0f);
		}
		
		gpu->clearFinalRenderTarget(this->sceneRenderTarget.get(), glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

		// calling this here SOMEHOW allowed the cleared color to bleed into the final render whenever the update rate was
		// uncapped, and the screen window size was small. Moving it into the gpu::drawToScreen() method seems to have
		// resolved the issue. However, I'm not really satisfied with not knowing 100% why this was happening (hints this
		// comment). 
		//gpu->clearScreenBuffer(0.0f, 1.0f, 0.0f, 1.0f); 
	}


// END VEL NAMESPACE
}