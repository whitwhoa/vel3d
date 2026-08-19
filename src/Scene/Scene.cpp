#include <fstream>

#include <spdlog/spdlog.h>

#include <glm/gtx/string_cast.hpp>

#include <nlohmann/json.hpp>

#include <vel/Runtime.h>
#include <vel/Util/functions.h>
#include <vel/Physics/CollisionObjectTemplate.h>
#include <vel/Scene/Scene.h>
#include <vel/Scene/Stage/Actor/Material/MaterialOptions.h>
#include <vel/Scene/Stage/Actor/Mesh/Vertex.h>
#include <vel/Scene/Stage/Actor/Material/Texture/Texture.h>

using json = nlohmann::json;


namespace vel
{
	Scene::Scene() :
		HeadlessScene(),
		sceneRenderTarget(nullptr),
		audioGroupKey(-1),
		animationTime(0.0f),
		screenTint(glm::vec4(1.0f, 1.0f, 1.0f, 0.0f)),
		frameTime(0.0),
		frameRate(0.0)
	{
		this->sceneRenderTarget = Runtime::_gpu->createFinalRenderTarget(
			"sceneRenderTarget_" + this->id,
			Runtime::_window->getWindowSize().x,
			Runtime::_window->getWindowSize().y
		);

		if(Runtime::_audioDevice)
			this->audioGroupKey = Runtime::_audioDevice->generateGroupKey();
	}
	
	Scene::~Scene()
	{
		this->freeAssets();
	}

	void Scene::internalImmediateLoop(float frameTime, float renderLerpInterval)
	{
		this->immediateLoop(frameTime, renderLerpInterval);
	}

	FinalRenderTarget* Scene::getSceneRenderTarget()
	{
		return this->sceneRenderTarget.get();
	}

	void Scene::loadBGMSound(const std::string& path)
	{
		this->soundsInUse.push_back(Runtime::_audioDevice->loadBGM(path));
	}

	bool Scene::loadSFXSound(const std::string& path)
	{
		std::optional<std::string> sfxOpt = Runtime::_audioDevice->loadSFX(path);
		if (!sfxOpt)
			return false;

		this->soundsInUse.push_back(sfxOpt.value());
		return true;
	}

	int Scene::getAudioDeviceGroupKey()
	{
		return this->audioGroupKey;
	}

	Camera* Scene::addCamera(const std::string& name, CameraType type)
	{
		std::unique_ptr<Camera> c = std::make_unique<Camera>(name, type);

		Camera* cameraPtr = c.get();
		this->cameras.push_back(std::move(c));

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

	void Scene::updateAllCameraResolutions(int x, int y)
	{
		for (auto& c : this->cameras)
			if (!c->getFixedResolution())
				c->setResolution(x, y);
	}

	void Scene::freeAssets()
	{
		SPDLOG_DEBUG("Freeing assets for scene: {}", this->id);
		
		for (auto& pMaterial : this->materialsInUse)
			Runtime::_assetManager->removeMaterial(pMaterial);
		
		for (auto& pTexture : this->texturesInUse)
			Runtime::_assetManager->removeTexture(pTexture);

		for (auto& pFontBitmap : this->fontBitmapsInUse)
			Runtime::_assetManager->removeFontBitmap(pFontBitmap);
        
		for (auto& shaderKV : this->shaders)
			this->removeShader(shaderKV.second.get());

		for (auto& s : this->soundsInUse)
			Runtime::_audioDevice->removeSound(s);


		// THESE GO IN HeadlessScene once we move transfer the corresponding functionality
		for (auto& cw : this->collisionWorlds)
			delete cw;

		for (auto& s : this->skeletonsInUse)
			Runtime::_assetManager->removeSkeleton(s);

		for (auto& a : this->animationsInUse)
			Runtime::_assetManager->removeAnimation(a);
		// END


		for (auto& c : this->cameras)
			Runtime::_gpu->clearRenderTarget(&c->getRenderTarget());
		this->cameras.clear();

		Runtime::_gpu->freeFinalRenderTarget(this->sceneRenderTarget.get());


		HeadlessScene::freeAssets();

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
		FontBitmap* fb = Runtime::_assetManager->loadFontBitmap(fontName, fontSize, fontPath);

		this->fontBitmapsInUse.push_back(fb);

		return fb;
	}

	FontBitmap* Scene::loadFontBitmapVisualHeight(const std::string& fontName, int desiredVisiblePx, const std::string& fontPath)
	{
		FontBitmap* fb = Runtime::_assetManager->loadFontBitmapVisualHeight(fontName, desiredVisiblePx, fontPath);

		this->fontBitmapsInUse.push_back(fb);

		return fb;
	}

	Texture* Scene::loadTexture(const std::string& name, const std::string& path, int options)
	{
		Texture* t = Runtime::_assetManager->loadTexture(name, path, options);

		this->texturesInUse.push_back(t);

		return t;
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

		Shader* diffuseMaterialShader = this->loadShader(shaderName, "uber.vert", "", "uber.frag", defs); // returns existing if already loaded

		std::unique_ptr<DiffuseMaterial> m = std::make_unique<DiffuseMaterial>(name, diffuseMaterialShader);

		if (opts & MTRL_OPT_TRANSLUCENT)
			m->setHasAlphaChannel(true);

		Material* pMaterial = Runtime::_assetManager->addMaterial(std::move(m));
		this->materialsInUse.push_back(pMaterial);

		return static_cast<DiffuseMaterial*>(pMaterial);
	}

	DiffuseLightmapMaterial* Scene::addDiffuseLightmapMaterial(const std::string& name, int opts)
	{
		std::vector<std::string> defs = DiffuseLightmapMaterial::shaderDefs;
		std::string shaderName = "diffuseLightmapMaterialShader";

		this->setShaderOpts(opts, defs, shaderName);

		Shader* diffuseLightmapMaterialShader = this->loadShader(shaderName, "uber.vert", "", "uber.frag", defs); // returns existing if already loaded

		std::unique_ptr<DiffuseLightmapMaterial> m = std::make_unique<DiffuseLightmapMaterial>(name, diffuseLightmapMaterialShader);

		if (opts & MTRL_OPT_TRANSLUCENT)
			m->setHasAlphaChannel(true);

		Material* pMaterial = Runtime::_assetManager->addMaterial(std::move(m));
		this->materialsInUse.push_back(pMaterial);

		return static_cast<DiffuseLightmapMaterial*>(pMaterial);
	}

	DiffuseAnimatedMaterial* Scene::addDiffuseAnimatedMaterial(const std::string& name, int opts)
	{
		std::vector<std::string> defs = DiffuseAnimatedMaterial::shaderDefs;
		std::string shaderName = "diffuseAnimatedMaterialShader";

		this->setShaderOpts(opts, defs, shaderName);

		Shader* diffuseAnimatedMaterialShader = this->loadShader(shaderName, "uber.vert", "", "uber.frag", defs); // returns existing if already loaded

		std::unique_ptr<DiffuseAnimatedMaterial> m = std::make_unique<DiffuseAnimatedMaterial>(name, diffuseAnimatedMaterialShader);

		if (opts & MTRL_OPT_TRANSLUCENT)
			m->setHasAlphaChannel(true);

		Material* pMaterial = Runtime::_assetManager->addMaterial(std::move(m));
		this->materialsInUse.push_back(pMaterial);

		return static_cast<DiffuseAnimatedMaterial*>(pMaterial);
	}

	DiffuseAnimatedLightmapMaterial* Scene::addDiffuseAnimatedLightmapMaterial(const std::string& name, int opts)
	{
		std::vector<std::string> defs = DiffuseAnimatedLightmapMaterial::shaderDefs;
		std::string shaderName = "diffuseAnimatedLightmapMaterialShader";

		this->setShaderOpts(opts, defs, shaderName);

		Shader* diffuseAnimatedLightmapMaterialShader = this->loadShader(shaderName, "uber.vert", "", "uber.frag", defs); // returns existing if already loaded

		std::unique_ptr<DiffuseAnimatedLightmapMaterial> m = std::make_unique<DiffuseAnimatedLightmapMaterial>(name, diffuseAnimatedLightmapMaterialShader);

		if (opts & MTRL_OPT_TRANSLUCENT)
			m->setHasAlphaChannel(true);

		Material* pMaterial = Runtime::_assetManager->addMaterial(std::move(m));
		this->materialsInUse.push_back(pMaterial);

		return static_cast<DiffuseAnimatedLightmapMaterial*>(pMaterial);
	}

	DiffuseSkinnedMaterial* Scene::addDiffuseSkinnedMaterial(const std::string& name, int opts)
	{
		std::vector<std::string> defs = DiffuseSkinnedMaterial::shaderDefs;
		std::string shaderName = "diffuseSkinnedMaterialShader";

		this->setShaderOpts(opts, defs, shaderName);

		Shader* diffuseSkinnedMaterialShader = this->loadShader(shaderName, "uber.vert", "", "uber.frag", defs); // returns existing if already loaded

		std::unique_ptr<DiffuseSkinnedMaterial> m = std::make_unique<DiffuseSkinnedMaterial>(name, diffuseSkinnedMaterialShader);

		if (opts & MTRL_OPT_TRANSLUCENT)
			m->setHasAlphaChannel(true);

		Material* pMaterial = Runtime::_assetManager->addMaterial(std::move(m));
		this->materialsInUse.push_back(pMaterial);

		return static_cast<DiffuseSkinnedMaterial*>(pMaterial);
	}

	AlphaMaskMaterial* Scene::addAlphaMaskMaterial(const std::string& name)
	{
		std::vector<std::string> defs = AlphaMaskMaterial::shaderDefs;
		std::string shaderName = "alphaMaskMaterialShader";

		int opts = MTRL_OPT_TRANSLUCENT;

		this->setShaderOpts(opts, defs, shaderName);

		Shader* alphaMaskMaterialShader = this->loadShader(shaderName, "uber.vert", "", "uber.frag", defs); // returns existing if already loaded

		std::unique_ptr<AlphaMaskMaterial> m = std::make_unique<AlphaMaskMaterial>(name, alphaMaskMaterialShader);

		if (opts & MTRL_OPT_TRANSLUCENT)
			m->setHasAlphaChannel(true);

		Material* pMaterial = Runtime::_assetManager->addMaterial(std::move(m));
		this->materialsInUse.push_back(pMaterial);

		return static_cast<AlphaMaskMaterial*>(pMaterial);
	}

	TextMaterial* Scene::addTextMaterial(const std::string& name, int opts)
	{
		std::vector<std::string> defs = TextMaterial::shaderDefs;
		std::string shaderName = "textMaterialShader";

		this->setShaderOpts(opts, defs, shaderName);

		Shader* textMaterialShader = this->loadShader(shaderName, "uber.vert", "", "uber.frag", defs); // returns existing if already loaded

		std::unique_ptr<TextMaterial> m = std::make_unique<TextMaterial>(name, textMaterialShader);

		if (opts & MTRL_OPT_TRANSLUCENT)
			m->setHasAlphaChannel(true);

		Material* pMaterial = Runtime::_assetManager->addMaterial(std::move(m));
		this->materialsInUse.push_back(pMaterial);

		return static_cast<TextMaterial*>(pMaterial);
	}

	DiffuseAmbientCubeMaterial* Scene::addDiffuseAmbientCubeMaterial(const std::string& name, int opts)
	{
		std::vector<std::string> defs = DiffuseAmbientCubeMaterial::shaderDefs;
		std::string shaderName = "diffuseAmbientCubeMaterialShader";

		this->setShaderOpts(opts, defs, shaderName);

		Shader* diffuseAmbientCubeMaterialShader = this->loadShader(shaderName, "uber.vert", "", "uber.frag", defs); // returns existing if already loaded

		std::unique_ptr<DiffuseAmbientCubeMaterial> m = std::make_unique<DiffuseAmbientCubeMaterial>(name, diffuseAmbientCubeMaterialShader);

		if (opts & MTRL_OPT_TRANSLUCENT)
			m->setHasAlphaChannel(true);

		Material* pMaterial = Runtime::_assetManager->addMaterial(std::move(m));
		this->materialsInUse.push_back(pMaterial);

		return static_cast<DiffuseAmbientCubeMaterial*>(pMaterial);
	}

	DiffuseAmbientCubeSkinnedMaterial* Scene::addDiffuseAmbientCubeSkinnedMaterial(const std::string& name, int opts)
	{
		std::vector<std::string> defs = DiffuseAmbientCubeSkinnedMaterial::shaderDefs;
		std::string shaderName = "diffuseAmbientCubeSkinnedMaterialShader";

		this->setShaderOpts(opts, defs, shaderName);

		Shader* diffuseAmbientCubeSkinnedMaterialShader = this->loadShader(shaderName, "uber.vert", "", "uber.frag", defs); // returns existing if already loaded

		std::unique_ptr<DiffuseAmbientCubeSkinnedMaterial> m = std::make_unique<DiffuseAmbientCubeSkinnedMaterial>(name, diffuseAmbientCubeSkinnedMaterialShader);

		if (opts & MTRL_OPT_TRANSLUCENT)
			m->setHasAlphaChannel(true);

		Material* pMaterial = Runtime::_assetManager->addMaterial(std::move(m));
		this->materialsInUse.push_back(pMaterial);

		return static_cast<DiffuseAmbientCubeSkinnedMaterial*>(pMaterial);
	}

	RGBAMaterial* Scene::addRGBAMaterial(const std::string& name, int opts)
	{
		std::vector<std::string> defs = RGBAMaterial::shaderDefs;
		std::string shaderName = "RGBAMaterialShader";

		this->setShaderOpts(opts, defs, shaderName);

		Shader* RGBAMaterialShader = this->loadShader(shaderName, "uber.vert", "", "uber.frag", defs); // returns existing if already loaded

		std::unique_ptr<RGBAMaterial> m = std::make_unique<RGBAMaterial>(name, RGBAMaterialShader);

		if (opts & MTRL_OPT_TRANSLUCENT)
			m->setHasAlphaChannel(true);

		Material* pMaterial = Runtime::_assetManager->addMaterial(std::move(m));
		this->materialsInUse.push_back(pMaterial);

		return static_cast<RGBAMaterial*>(pMaterial);
	}

	RGBALineMaterial* Scene::addRGBALineMaterial(const std::string& name, int opts)
	{
		std::vector<std::string> defs = RGBALineMaterial::shaderDefs;
		std::string shaderName = "RGBALineMaterialShader";

		this->setShaderOpts(opts, defs, shaderName);

		Shader* RGBALineMaterialShader = this->loadShader(shaderName, "line.vert", "line.geom", "line.frag", defs); // returns existing if already loaded

		std::unique_ptr<RGBALineMaterial> m = std::make_unique<RGBALineMaterial>(name, RGBALineMaterialShader);

		if (opts & MTRL_OPT_TRANSLUCENT)
			m->setHasAlphaChannel(true);

		Material* pMaterial = Runtime::_assetManager->addMaterial(std::move(m));
		this->materialsInUse.push_back(pMaterial);

		return static_cast<RGBALineMaterial*>(pMaterial);
	}

	RGBALightmapMaterial* Scene::addRGBALightmapMaterial(const std::string& name, int opts)
	{
		std::vector<std::string> defs = RGBALightmapMaterial::shaderDefs;
		std::string shaderName = "RGBALightmapMaterialShader";

		this->setShaderOpts(opts, defs, shaderName);

		Shader* RGBALightmapMaterialShader = this->loadShader(shaderName, "uber.vert", "", "uber.frag", defs); // returns existing if already loaded

		std::unique_ptr<RGBALightmapMaterial> m = std::make_unique<RGBALightmapMaterial>(name, RGBALightmapMaterialShader);

		if (opts & MTRL_OPT_TRANSLUCENT)
			m->setHasAlphaChannel(true);

		Material* pMaterial = Runtime::_assetManager->addMaterial(std::move(m));
		this->materialsInUse.push_back(pMaterial);

		return static_cast<RGBALightmapMaterial*>(pMaterial);
	}

	DiffuseCausticMaterial* Scene::addDiffuseCausticMaterial(const std::string& name, int opts)
	{
		std::vector<std::string> defs = DiffuseCausticMaterial::shaderDefs;
		std::string shaderName = "diffuseCausticMaterialShader";

		this->setShaderOpts(opts, defs, shaderName);

		Shader* diffuseCausticMaterialShader = this->loadShader(shaderName, "uber.vert", "", "uber.frag", defs); // returns existing if already loaded

		std::unique_ptr<DiffuseCausticMaterial> m = std::make_unique<DiffuseCausticMaterial>(name, diffuseCausticMaterialShader);

		if (opts & MTRL_OPT_TRANSLUCENT)
			m->setHasAlphaChannel(true);

		Material* pMaterial = Runtime::_assetManager->addMaterial(std::move(m));
		this->materialsInUse.push_back(pMaterial);

		return static_cast<DiffuseCausticMaterial*>(pMaterial);
	}

	DiffuseCausticLightmapMaterial* Scene::addDiffuseCausticLightmapMaterial(const std::string& name, int opts)
	{
		std::vector<std::string> defs = DiffuseCausticLightmapMaterial::shaderDefs;
		std::string shaderName = "diffuseCausticLightmapMaterialShader";

		this->setShaderOpts(opts, defs, shaderName);

		Shader* diffuseCausticLightmapMaterialShader = this->loadShader(shaderName, "uber.vert", "", "uber.frag", defs); // returns existing if already loaded

		std::unique_ptr<DiffuseCausticLightmapMaterial> m = std::make_unique<DiffuseCausticLightmapMaterial>(name, diffuseCausticLightmapMaterialShader);

		if (opts & MTRL_OPT_TRANSLUCENT)
			m->setHasAlphaChannel(true);

		Material* pMaterial = Runtime::_assetManager->addMaterial(std::move(m));
		this->materialsInUse.push_back(pMaterial);

		return static_cast<DiffuseCausticLightmapMaterial*>(pMaterial);
	}

	DiffuseSingleSelectableMaterial* Scene::addDiffuseSingleSelectableMaterial(const std::string& name, int opts)
	{
		std::vector<std::string> defs = DiffuseSingleSelectableMaterial::shaderDefs;
		std::string shaderName = "diffuseSingleSelectableMaterialShader";

		this->setShaderOpts(opts, defs, shaderName);

		Shader* theShader = this->loadShader(shaderName, "uber.vert", "", "uber.frag", defs); // returns existing if already loaded

		std::unique_ptr<DiffuseSingleSelectableMaterial> m = std::make_unique<DiffuseSingleSelectableMaterial>(name, theShader);

		if (opts & MTRL_OPT_TRANSLUCENT)
			m->setHasAlphaChannel(true);

		Material* pMaterial = Runtime::_assetManager->addMaterial(std::move(m));
		this->materialsInUse.push_back(pMaterial);

		return static_cast<DiffuseSingleSelectableMaterial*>(pMaterial);
	}

	AnimatedBillboardMaterial* Scene::addAnimatedBillboardMaterial(const std::string& name, int opts)
	{
		std::vector<std::string> defs = AnimatedBillboardMaterial::shaderDefs;
		std::string shaderName = "animatedBillboardMaterial";

		this->setShaderOpts(opts, defs, shaderName);

		Shader* animatedBillboardMaterialShader = this->loadShader("NPCBillboardShader", "uber.vert", "", "uber.frag", defs); // returns existing if already loaded

		std::unique_ptr<AnimatedBillboardMaterial> m = std::make_unique<AnimatedBillboardMaterial>(name, animatedBillboardMaterialShader);

		if (opts & MTRL_OPT_TRANSLUCENT)
			m->setHasAlphaChannel(true);

		Material* pMaterial = Runtime::_assetManager->addMaterial(std::move(m));
		this->materialsInUse.push_back(pMaterial);

		return static_cast<AnimatedBillboardMaterial*>(pMaterial);
	}

	Texture* Scene::getTexture(const std::string& name)
	{
		return Runtime::_assetManager->getTexture(name);
	}

	FontBitmap* Scene::getFontBitmap(const std::string& name)
	{
		return Runtime::_assetManager->getFontBitmap(name);
	}

	Material* Scene::getMaterial(const std::string& name)
	{
		return Runtime::_assetManager->getMaterial(name);
	}

	TextActor* Scene::addTextActor(Stage* stage, const std::string& name, const std::string& theText, FontBitmap* fb,
		glm::vec4 color, PlaneOrigin originType)
	{
		std::unique_ptr<TextActor> ta = std::make_unique<TextActor>();
		ta->name = name;
		ta->text = theText;
		ta->fontBitmap = fb;
		ta->originType = originType;

		// create the mesh using provided FontBitmap and text string
		Mesh* pTam = this->addMesh(std::move(Runtime::_assetManager->loadTextActorMesh(ta.get())));

		// create material
		Material* taMaterial = this->addTextMaterial(name + "_material", MTRL_OPT_TRANSLUCENT);
		taMaterial->addTexture(&fb->texture);
		taMaterial->setColor(color);

		// create actor
		Actor* pTextActor = stage->addActor(name, pTam, taMaterial);

		// add actor pointer to TextActor.actor
		ta->actor = pTextActor;

		// add new text actor to stage and return pointer
		return stage->addTextActor(std::move(ta));
	}

	LineActor* Scene::addLineActor(Stage* stage, const std::string& name, const std::vector<std::tuple<glm::vec2, glm::vec2, unsigned int>>& points, std::vector<glm::vec4> colors)
	{
		// create the LineActor
		std::unique_ptr<LineActor> la = std::make_unique<LineActor>(name);

		// create the mesh
		Mesh* pMesh = this->addMesh(std::move(LineActor::segmentsToMesh(name, points)));

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
		Actor* pActor = stage->addActor(name, pMesh, pMaterial);

		// add actor pointer to LineActor
		la->actor = pActor;

		return stage->addLineActor(std::move(la));
	}

	LineActor* Scene::addContinuousLineActor(Stage* stage, const std::string& name, const std::vector<glm::vec2>& points, glm::vec4 color)
	{
		std::unique_ptr<LineActor> la = std::make_unique<LineActor>(name);

		Mesh* pMesh = this->addMesh(std::move(LineActor::pointsToMesh(name, points)));

		bool hasAlpha = color.w < 0.999f;

		RGBALineMaterial* pMaterial = this->addRGBALineMaterial(name + "_material", hasAlpha);
		pMaterial->setLineColor(0, color);

		Actor* pActor = stage->addActor(name, pMesh, pMaterial);

		la->actor = pActor;

		return stage->addLineActor(std::move(la));
	}

	Billboard* Scene::addBillboard(Stage* stage, const std::string& name, vel::Material* material, vel::Camera* parentCamera, float width, float height)
	{
		// generate mesh, send it to gpu, track it for managment by scene
		std::unique_ptr<Mesh> tmpM = std::make_unique<Mesh>(name + "_mesh");
		tmpM->initBillboardQuad(width, height);

		Mesh* m = this->addMesh(std::move(tmpM));

		// create actor
		Actor* a = stage->addActor(name, m, material);
		a->setDynamic(true);

		// create the billboard, add to stage, return pointer
		return stage->addBillboard(std::make_unique<Billboard>(a, parentCamera));
	}

	Billboard* Scene::addBillboard(Stage* stage, const std::string& name, Material* material, Camera* parentCamera, Mesh* mesh)
	{
		Actor* a = stage->addActor(name, mesh, material);
		a->setDynamic(true);

		return stage->addBillboard(std::make_unique<Billboard>(a, parentCamera));
	}


	void Scene::lerpAnimators(float alpha)
	{
		for (auto& s : this->stages)
			s->lerpAnimators(alpha);
	}

	void Scene::updateTextActor(TextActor* ta)
	{
		std::unique_ptr<Mesh> updatedMesh = std::move(Runtime::_assetManager->loadTextActorMesh(ta));
		ta->actor->getMesh()->setVertices(updatedMesh->getVertices());
		ta->actor->getMesh()->setIndices(updatedMesh->getIndices());

		Runtime::_gpu->updateMesh(ta->actor->getMesh());

		ta->requiresUpdate = false;
	}

	void Scene::updateTextActors()
	{
		for (auto& s : this->stages)
		{
			for (auto& ta : s->getTextActors())
			{
				if (!ta->requiresUpdate)
					continue;

				this->updateTextActor(ta.get());
			}
		}
	}

	void Scene::updateBillboards()
	{
		for (auto& s : this->stages)
			s->updateBillboards();
	}
	

	void Scene::draw(float frameTime, float alpha)
	{
		for (auto& s : this->stages)
		{
			if (!s->getVisible())
				continue;

			bool actorsFirstPass = true;

			for (auto& c : s->getCameras())
			{
				c->update();

				Runtime::_gpu->updateCameraViewportSize(c->getResolution().x, c->getResolution().y); // different cameras can have different resolutions

				Runtime::_gpu->setRenderTarget(&c->getRenderTarget());

				Runtime::_gpu->setOpaqueRenderState();

				bool foundFirstAlpha = false;

				for (auto& pair : s->getActors())
				{
					for (auto& a : pair.second)
					{
						if (!a->getMesh() || !a->isVisible() || !a->getMaterial()->getShader())
							continue;

						if (a->getMaterial()->getHasAlphaChannel() && !foundFirstAlpha)
						{
							foundFirstAlpha = true;
							Runtime::_gpu->setAlphaRenderState();
						}

						if (actorsFirstPass)
							a->getMaterial()->preDraw(frameTime);

						Runtime::_gpu->useShader(a->getMaterial()->getShader()); // only alters gpu state if necessary
						Runtime::_gpu->useMesh(a->getMesh()); // only alters gpu state if necessary
						Runtime::_gpu->setActiveMaterial(a->getMaterial());

						a->getMaterial()->draw(alpha, Runtime::_gpu.get(), a.get(), c->getViewMatrix(), c->getProjectionMatrix());
					}
				}

				actorsFirstPass = false;

				Runtime::_gpu->composeFBOs();
			}
		}


		// all stage camera's framebuffers are now updated, loop through each stage camera and check if it should display it's contents 

		// now bind the scene's FinalRenderTarget. It's viewport size should always be the full size of the window, or screen in fullscreen mode
		std::unique_ptr<FinalRenderTarget> updatedFRT = Runtime::_gpu->updateFinalRenderTargetVPSize(
			this->sceneRenderTarget.get(), 
			Runtime::_window->getWindowSize().x,
			Runtime::_window->getWindowSize().y
		);

		if (updatedFRT)
			this->sceneRenderTarget = std::move(updatedFRT);


		Runtime::_gpu->setFinalRenderTarget(this->sceneRenderTarget.get());


		for (auto& c : this->cameras)
			if (c->isFinalRenderCam())
				Runtime::_gpu->drawToFinalRenderTarget(c->getRenderTarget().opaqueTexture.frames.at(0).dsaHandle);
		

		// call post process to apply post process shader while drawing into the default framebuffer for display to screen
		Runtime::_gpu->setDefaultFrameBuffer();
		Runtime::_gpu->drawToScreen(this->sceneRenderTarget.get(), this->screenTint);

		// If you don't set glviewport back to the render resolution (vs leaving it at the window size), mouse movement gets jacked up 
		Runtime::_gpu->setViewportSize(Runtime::_window->getResolution().x, Runtime::_window->getResolution().y);


		// moving collision debug draw event as final thing as it draws directly to the screen buffer, and I don't want to have to 
		// think about updating it right now
		for (auto& cw : this->collisionWorlds)
		{
			if (cw->getIsActive() && cw->getDebugDrawer() != nullptr)
			{
				cw->getDynamicsWorld()->debugDrawWorld(); // load vertices into associated CollisionDebugDrawer
				Runtime::_gpu->useShader(cw->getDebugDrawer()->getShaderProgram());
				Runtime::_gpu->setShaderMat4("vp", cw->getCamera()->getProjectionMatrix() * cw->getCamera()->getViewMatrix());
				Runtime::_gpu->debugDrawCollisionWorld(cw->getDebugDrawer()); // draw all loaded vertices with a single call and clear
			}
		}

	}

	void Scene::clearAllRenderTargetBuffers()
	{
		for (auto& c : this->cameras)
		{
			Runtime::_gpu->setRenderTarget(&c->getRenderTarget());
			Runtime::_gpu->clearRenderTargetBuffers(0.0f, 0.0f, 0.0f, 0.0f);
		}
		
		Runtime::_gpu->clearFinalRenderTarget(this->sceneRenderTarget.get(), glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

		// calling this here SOMEHOW allowed the cleared color to bleed into the final render whenever the update rate was
		// uncapped, and the screen window size was small. Moving it into the gpu::drawToScreen() method seems to have
		// resolved the issue. However, I'm not really satisfied with not knowing 100% why this was happening (hince this
		// comment). 
		//gpu->clearScreenBuffer(0.0f, 1.0f, 0.0f, 1.0f); 
	}

	/***********************************************************************************************
	* LOAD SHADERS (this will not be required after we refactor for MDI)
	************************************************************************************************/
	std::optional<std::string> Scene::loadShaderFile(const std::string& shaderPath)
	{
		std::ifstream shaderFile(shaderPath);

		if (!shaderFile.is_open())
		{
			SPDLOG_DEBUG("AssetManager::loadShaderFile(): could not open shader file: {}", shaderPath);
			return std::nullopt;
		}

		std::stringstream shaderStream;
		shaderStream << shaderFile.rdbuf();
		shaderFile.close();

		if (shaderStream.str().empty())
		{
			SPDLOG_DEBUG("AssetManager::loadShaderFile(): Shader file is empty: {}", shaderPath);
			return std::nullopt;
		}

		return shaderStream.str();
	}

	std::string Scene::getTopShaderLines(const std::string& shaderCode, int numLinesToGet)
	{
		std::istringstream shaderStream(shaderCode);
		std::string line;
		std::string firstLines;

		for (int i = 0; i < numLinesToGet; ++i)
		{
			std::getline(shaderStream, line);
			firstLines += line + "\n";
		}

		return firstLines;
	}

	std::string Scene::getBottomShaderLines(const std::string& shaderCode, int numLinesToSkip)
	{
		std::istringstream shaderStream(shaderCode);
		std::string line;

		// Skip the specified number of lines
		for (int i = 0; i < numLinesToSkip; ++i)
			std::getline(shaderStream, line);

		// Return the remaining shader code
		std::stringstream remainingShaderCode;
		remainingShaderCode << shaderStream.rdbuf();

		return remainingShaderCode.str();
	}

	Shader* Scene::loadShader(const std::string& name, const std::string& vertFile, const std::string& geomFile,
		const std::string& fragFile, std::vector<std::string> defs)
	{

		if (this->shaders.contains(name))
		{
			SPDLOG_DEBUG("Existing Shader, bypass reload: {}", name);

			return this->shaders.at(name).get();
		}

		SPDLOG_DEBUG("Loading new Shader: {}", name);


		// Process vertex shader script
		std::optional<std::string> vcOpt = this->loadShaderFile(Runtime::_config.dataDir + "/shaders/" + vertFile);
		if (!vcOpt)
			return nullptr;

		std::string vertexCode = vcOpt.value();
		std::string topVertexLines = this->getTopShaderLines(vertexCode, 10);
		std::string bottomVertexLines = this->getBottomShaderLines(vertexCode, 10);
		std::stringstream preprocessedVertexCode;
		preprocessedVertexCode << topVertexLines;

		for (const auto& def : defs) // preload defs into scripts
			preprocessedVertexCode << "#define " << def << "\n";

		preprocessedVertexCode << bottomVertexLines;

		vertexCode = preprocessedVertexCode.str();


		// Process Geometry shader script
		std::string geomCode = "";
		if (geomFile != "")
		{
			std::optional<std::string> gcOpt = this->loadShaderFile(Runtime::_config.dataDir + "/shaders/" + geomFile);
			if (!gcOpt)
				return nullptr;

			geomCode = gcOpt.value();
			std::string topGeomLines = this->getTopShaderLines(geomCode, 10);
			std::string bottomGeomLines = this->getBottomShaderLines(geomCode, 10);
			std::stringstream preprocessedGeomCode;
			preprocessedGeomCode << topGeomLines;

			for (const auto& def : defs) // preload defs into scripts
				preprocessedGeomCode << "#define " << def << "\n";

			preprocessedGeomCode << bottomGeomLines;

			geomCode = preprocessedGeomCode.str();
		}


		// Process fragment shader script
		std::optional<std::string> fcOpt = this->loadShaderFile(Runtime::_config.dataDir + "/shaders/" + fragFile);
		if (!fcOpt)
			return nullptr;

		std::string fragmentCode = fcOpt.value();
		std::string topFragmentLines = this->getTopShaderLines(fragmentCode, 10);
		std::string bottomFragmentLines = this->getBottomShaderLines(fragmentCode, 10);
		std::stringstream preprocessedFragmentCode;
		preprocessedFragmentCode << topFragmentLines;

		for (const auto& def : defs) // preload defs into scripts
			preprocessedFragmentCode << "#define " << def << "\n";

		preprocessedFragmentCode << bottomFragmentLines;
		fragmentCode = preprocessedFragmentCode.str();


		// Build the shader
		std::unique_ptr<Shader> s = std::make_unique<Shader>();
		s->name = name;
		s->vertCode = vertexCode;
		s->geomCode = geomCode;
		s->fragCode = fragmentCode;

		Shader* rawPtr = s.get();

		this->shaders.emplace(name, std::move(s));

		Runtime::_gpu->loadShader(rawPtr);

		return rawPtr;
	}

	Shader* Scene::getShader(const std::string& name)
	{
		auto it = this->shaders.find(name);

		if (it == shaders.end())
		{
			SPDLOG_DEBUG("AssetManager::getShader(): Attempting to get shader that does not exist: {}", name);
			return nullptr;
		}

		return it->second.get();
	}

	void Scene::removeShader(const Shader* pShader)
	{
		auto it = this->shaders.find(pShader->name);

		if (it == shaders.end())
			return;

		Runtime::_gpu->clearShader(it->second.get());
		this->shaders.erase(pShader->name);
	}

	/***********************************************************************************************
	* LOAD MESHES
	************************************************************************************************/
	std::vector<Mesh*> Scene::loadMesh(const std::string& path)
	{
		std::vector<Mesh*> out = HeadlessScene::loadMesh(path);
		for(auto& m : out)
			Runtime::_gpu->loadMesh(m);

		return out;
	}

	// This method assumes that the caller understands no duplication checks are occuring
	Mesh* Scene::addMesh(std::unique_ptr<Mesh> m)
	{
		Mesh* rawPtr = HeadlessScene::addMesh(std::move(m));
		Runtime::_gpu->loadMesh(rawPtr);

		return rawPtr;
	}

	void Scene::updateRenderMesh(Mesh* m)
	{
		Runtime::_gpu->updateMesh(m);
	}

	void Scene::removeMesh(Mesh* m)
	{
		auto it = this->meshes.find(m->getName());

		if (it == this->meshes.end())
			return;

		Runtime::_gpu->clearMesh(m);

		this->meshes.erase(m->getName());
	}



} // END VEL NAMESPACE