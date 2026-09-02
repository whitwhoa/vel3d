#include <fstream>

#include <spdlog/spdlog.h>

#include <glm/gtx/string_cast.hpp>

#include <nlohmann/json.hpp>

#include <stb_headers/stb_image.h>
#include <stb_headers/stb_truetype.h>

#include <vel/Runtime.h>
#include <vel/Util/functions.h>
#include <vel/Scene/CollisionWorld/CollisionObjectTemplate.h>
#include <vel/Scene/Scene.h>
#include <vel/Scene/Texture/Texture.h>

using json = nlohmann::json;


namespace vel
{
	Scene::Scene() :
		HeadlessScene(),
		sceneRenderTarget(nullptr),
		audioGroupKey(-1),
		screenTint(glm::vec4(1.0f, 1.0f, 1.0f, 0.0f)),
		frameTime(0.0),
		frameRate(0.0)
	{
		this->renderGeoPools.emplace(VtxLayout::VTX_POS_NRML, std::make_unique<GeoPoolT<VtxPosNrml>>());
		this->renderGeoPools.emplace(VtxLayout::VTX_POS_NRML_TX, std::make_unique<GeoPoolT<VtxPosNrmlTx>>());
		this->renderGeoPools.emplace(VtxLayout::VTX_POS_NRML_TX_LM, std::make_unique<GeoPoolT<VtxPosNrmlTxLm>>());
		this->renderGeoPools.emplace(VtxLayout::VTX_POS_NRML_TX_SKN, std::make_unique<GeoPoolT<VtxPosNrmlTxSkn>>());


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
		SPDLOG_DEBUG("Freeing assets for scene: {}", this->id);

		// Note: we repeat some code here to optimize for destruction. If we called the
		// explicit removal methods of each asset type, that's an additional method call,
		// plus validation check, plus logic, plus another hash lookup for the erase. Doing
		// it this way, we clear only what needs cleared, then let memory be freed when Scene
		// goes out of scope (ie is removed from App)

		// Note: materials do not require explicit removal

		for (auto& t : this->textures)
		{
			Runtime::_gpu->clearTexture(t.second.get());

			if (t.second->options & TXT_OPT_CPU_AND_GPU)
				for (auto& td : t.second->frames)
					stbi_image_free(td.primaryImageData.data);
		}
		
		for (auto& fb : this->fontBitmaps)
			Runtime::_gpu->clearTexture(&fb.second->texture);

		for (auto& s : this->shaders)
			Runtime::_gpu->clearShader(s.second.get());

		for (auto& m : this->meshes)
		{
			if ((m.second->flags & MESHFLAG_RENDERABLE) && !(m.second->flags & MESHFLAG_POOLED))
				Runtime::_gpu->clearGeoPool(m.second->gp->gpuGeoPool.value());
		}

		for (auto& s : this->soundsInUse)
			Runtime::_audioDevice->removeSound(s);

		for (auto& c : this->cameras)
			Runtime::_gpu->clearRenderTarget(&c->getRenderTarget());

		Runtime::_gpu->freeFinalRenderTarget(this->sceneRenderTarget.get());
	}

	bool Scene::internalLoad()
	{
		if (HeadlessScene::internalLoad())
		{
			for (auto& renderGeoPoolKV : this->renderGeoPools)
				Runtime::_gpu->loadGeoPool(renderGeoPoolKV.second.get());

			for (auto& renderSoloGeoPoolKV : this->renderSoloGeoPools)
				Runtime::_gpu->loadGeoPool(renderSoloGeoPoolKV.second.get());

			return true;
		}

		return false;
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

	void Scene::setScreenTint(glm::vec4 c)
	{
		this->screenTint = c;
	}

	void Scene::clearScreenTint()
	{
		this->screenTint = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);
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

		Material* pMaterial = this->addMaterial(std::move(m));

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

		Material* pMaterial = this->addMaterial(std::move(m));

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

		Material* pMaterial = this->addMaterial(std::move(m));

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

		Material* pMaterial = this->addMaterial(std::move(m));

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

		Material* pMaterial = this->addMaterial(std::move(m));

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

		Material* pMaterial = this->addMaterial(std::move(m));

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

		Material* pMaterial = this->addMaterial(std::move(m));

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

		Material* pMaterial = this->addMaterial(std::move(m));

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

		Material* pMaterial = this->addMaterial(std::move(m));

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

		Material* pMaterial = this->addMaterial(std::move(m));

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

		Material* pMaterial = this->addMaterial(std::move(m));

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

		Material* pMaterial = this->addMaterial(std::move(m));

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

		Material* pMaterial = this->addMaterial(std::move(m));

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

		Material* pMaterial = this->addMaterial(std::move(m));

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

		Material* pMaterial = this->addMaterial(std::move(m));

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

		Material* pMaterial = this->addMaterial(std::move(m));

		return static_cast<AnimatedBillboardMaterial*>(pMaterial);
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
		Mesh* pTam = this->addMesh(std::move(this->loadTextActorMesh(ta.get())));

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
		Mesh* m = this->addMesh(std::move(this->loadBillboardMesh(name + "_mesh", width, height)));

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
		Mesh* m = ta->actor->getMesh();

		this->buildTextActorGeometry(ta, m);

		Runtime::_gpu->updateGeoPool(m->gp);

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
			SPDLOG_DEBUG("Scene::loadShaderFile(): could not open shader file: {}", shaderPath);
			return std::nullopt;
		}

		std::stringstream shaderStream;
		shaderStream << shaderFile.rdbuf();
		shaderFile.close();

		if (shaderStream.str().empty())
		{
			SPDLOG_DEBUG("Scene::loadShaderFile(): Shader file is empty: {}", shaderPath);
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
			SPDLOG_DEBUG("Scene::loadShader(): Existing Shader, bypass reload: {}", name);

			return this->shaders.at(name).get();
		}

		SPDLOG_DEBUG("Scene::loadShader(): Loading new Shader: {}", name);


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
			SPDLOG_DEBUG("Scene::getShader(): Attempting to get shader that does not exist: {}", name);
			return nullptr;
		}

		return it->second.get();
	}

	void Scene::removeShader(Shader* pShader)
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
	std::vector<Mesh*> Scene::loadMesh(const std::string& path, uint32_t meshFlags)
	{
		if (!(meshFlags & MESHFLAG_RENDERABLE))
			return HeadlessScene::loadMesh(path, meshFlags);

		const std::vector<std::pair<std::string, VtxLayout>>& preLoadData = this->meshLoader->preload(path);
		if (preLoadData.size() == 0)
		{
			SPDLOG_DEBUG("Scene::loadMesh(): preload failed");
			return {};
		}

		std::vector<Mesh*> out;

		// check for duplicates
		std::vector<std::pair<std::string, GeoPool*>> requiredData;
		for (auto& pld : preLoadData)
		{
			auto it = this->meshes.find(pld.first);

			if (it == this->meshes.end())
			{
				SPDLOG_DEBUG("Scene::loadMesh(): new mesh load: {}", pld);

				if (meshFlags & MESHFLAG_POOLED)
				{
					requiredData.push_back({ pld.first, this->renderGeoPools[pld.second].get() });
				}
				else
				{
					std::unique_ptr<GeoPool> renderSoloGeoPool = nullptr;
					if (pld.second == VtxLayout::VTX_POS_NRML)
						renderSoloGeoPool = std::make_unique<GeoPoolT<VtxPosNrml>>(true);
					else if (pld.second == VtxLayout::VTX_POS_NRML_TX)
						renderSoloGeoPool = std::make_unique<GeoPoolT<VtxPosNrmlTx>>(true);
					else if (pld.second == VtxLayout::VTX_POS_NRML_TX_LM)
						renderSoloGeoPool = std::make_unique<GeoPoolT<VtxPosNrmlTxLm>>(true);
					else if (pld.second == VtxLayout::VTX_POS_NRML_TX_SKN)
						renderSoloGeoPool = std::make_unique<GeoPoolT<VtxPosNrmlTxSkn>>(true);

					GeoPool* standAlonePoolRawPtr = renderSoloGeoPool.get();

					this->renderSoloGeoPools.emplace(pld.first, std::move(renderSoloGeoPool));

					requiredData.push_back({ pld.first, standAlonePoolRawPtr });
				}
			}
			else
			{
				SPDLOG_DEBUG("Scene::loadMesh(): existing mesh (load bypassed): {}", pld);
				out.push_back(it->second.get());
			}
		}

		std::vector<std::unique_ptr<Mesh>> loadedAssets = this->meshLoader->load(&requiredData);

		for (auto& m : loadedAssets)
		{
			Mesh* rawPtr = m.get();
			this->meshes.emplace(m->name, std::move(m));

			out.push_back(rawPtr);
		}

		this->meshLoader->reset();

		return out;
	}

	// This method assumes that the caller understands no duplication checks are occuring
	Mesh* Scene::addMesh(std::unique_ptr<Mesh> m)
	{
		Mesh* rawPtr = HeadlessScene::addMesh(std::move(m));
		Runtime::_gpu->loadGeoPool(rawPtr->gp);

		return rawPtr;
	}

	void Scene::removeMesh(Mesh* m)
	{
		auto it = this->meshes.find(m->name);

		if (it == this->meshes.end())
			return;

		if ((m->flags & MESHFLAG_RENDERABLE) && !(m->flags & MESHFLAG_POOLED))
			Runtime::_gpu->clearGeoPool(m->gp->gpuGeoPool.value());

		this->meshes.erase(it);
	}

	void Scene::buildTextActorGeometry(TextActor* ta, Mesh* mesh)
	{
		GeoPoolT<VtxPosNrmlTx>* gp = static_cast<GeoPoolT<VtxPosNrmlTx>*>(mesh->gp);
		gp->vertices.clear();
		gp->indices.clear();


		ta->caretPositions.clear();
		ta->logicalWidth = 0.0f;

		unsigned int lastIndex = 0;
		unsigned int lineCount = 1;

		float offsetX = 0.0f;
		float offsetY = 0.0f;

		ta->caretPositions.push_back({ offsetX, -offsetY });

		for (auto c : ta->text)
		{
			if (c == '\n')
			{
				if (offsetX > ta->logicalWidth)
					ta->logicalWidth = offsetX;

				++lineCount;
				offsetX = 0.0f;
				offsetY += ta->fontBitmap->lineHeight;

				ta->caretPositions.push_back({ offsetX, -offsetY });

				continue;
			}

			const auto glyphInfo = this->getFontGlyphInfo(c, offsetX, offsetY, ta->fontBitmap);
			offsetX = glyphInfo.offsetX;
			offsetY = glyphInfo.offsetY;

			VtxPosNrmlTx v1;
			v1.position = glyphInfo.positions[0];
			v1.normal = glm::vec3(0.0f, 0.0f, 1.0f);
			v1.textureCoords = glyphInfo.uvs[0];
			v1.materialUBOIndex = 0;
			gp->vertices.push_back(v1);

			VtxPosNrmlTx v2;
			v2.position = glyphInfo.positions[1];
			v2.normal = glm::vec3(0.0f, 0.0f, 1.0f);
			v2.textureCoords = glyphInfo.uvs[1];
			v2.materialUBOIndex = 0;
			gp->vertices.push_back(v2);

			VtxPosNrmlTx v3;
			v3.position = glyphInfo.positions[2];
			v3.normal = glm::vec3(0.0f, 0.0f, 1.0f);
			v3.textureCoords = glyphInfo.uvs[2];
			v3.materialUBOIndex = 0;
			gp->vertices.push_back(v3);

			VtxPosNrmlTx v4;
			v4.position = glyphInfo.positions[3];
			v4.normal = glm::vec3(0.0f, 0.0f, 1.0f);
			v4.textureCoords = glyphInfo.uvs[3];
			v4.materialUBOIndex = 0;
			gp->vertices.push_back(v4);

			// Add indices with correct winding
			gp->indices.push_back(lastIndex + 1); // 1
			gp->indices.push_back(lastIndex); // 0
			gp->indices.push_back(lastIndex + 3); // 3
			gp->indices.push_back(lastIndex + 1); // 1
			gp->indices.push_back(lastIndex + 3); // 3
			gp->indices.push_back(lastIndex + 2); // 2

			lastIndex += 4;

			ta->caretPositions.push_back({ offsetX, -offsetY });
		}

		mesh->indexCount = gp->indices.size();
		mesh->refreshAABB();

		float minX = mesh->aabb.getMinEdge().x;
		float maxX = mesh->aabb.getMaxEdge().x;
		float logicalMaxY = ta->fontBitmap->ascent;
		float logicalMinY = ta->fontBitmap->descent - static_cast<float>(lineCount - 1) * ta->fontBitmap->lineHeight;

		float xOffset = 0.0f;
		float yOffset = 0.0f;

		// Horizontal alignment can remain based on actual geometry.
		if (ta->originType == PlaneOrigin::RIGHT_BOTTOM ||
			ta->originType == PlaneOrigin::RIGHT_CENTER ||
			ta->originType == PlaneOrigin::RIGHT_TOP)
		{
			xOffset = maxX;
		}
		else if (ta->originType == PlaneOrigin::CENTER_BOTTOM ||
			ta->originType == PlaneOrigin::CENTER_CENTER ||
			ta->originType == PlaneOrigin::CENTER_TOP)
		{
			xOffset = (minX + maxX) * 0.5f;
		}

		// Vertical alignment uses stable font metrics.
		if (ta->originType == PlaneOrigin::LEFT_TOP ||
			ta->originType == PlaneOrigin::CENTER_TOP ||
			ta->originType == PlaneOrigin::RIGHT_TOP)
		{
			yOffset = logicalMaxY;
		}
		else if (ta->originType == PlaneOrigin::LEFT_CENTER ||
			ta->originType == PlaneOrigin::CENTER_CENTER ||
			ta->originType == PlaneOrigin::RIGHT_CENTER)
		{
			yOffset = (logicalMinY + logicalMaxY) * 0.5f;
		}
		else // BOTTOM
		{
			yOffset = logicalMinY;
		}

		for (auto& v : gp->vertices)
		{
			v.position.x -= xOffset;
			v.position.y -= yOffset;
		}
		mesh->refreshAABB();

		for (auto& p : ta->caretPositions)
		{
			p.x -= xOffset;
			p.y -= yOffset;
		}

		ta->logicalHeight = logicalMaxY - logicalMinY;
		if (offsetX > ta->logicalWidth)
			ta->logicalWidth = offsetX;
		//ta->logicalWidth = maabb.getSize().x;
	}

	std::unique_ptr<Mesh> Scene::loadTextActorMesh(TextActor* ta)
	{
		std::unique_ptr<GeoPoolT<VtxPosNrmlTx>> gp = std::make_unique<GeoPoolT<VtxPosNrmlTx>>();

		std::unique_ptr<Mesh> m = std::make_unique<Mesh>(ta->name + "_mesh");
		m->gp = gp.get();
		m->firstIndex = 0;
		m->baseVertex = 0;
		m->flags = MESHFLAG_RENDERABLE;
		
		this->buildTextActorGeometry(ta, m.get());

		this->renderSoloGeoPools.emplace(m->name, std::move(gp));

		return m;
	}

	std::unique_ptr<Mesh> Scene::loadBillboardMesh(std::string name, float width, float height)
	{
		if (!(width > 0.0f && height > 0.0f))
		{
			SPDLOG_DEBUG("Scene::loadBillboardMesh(): Billboard width and height must be positive.");
			return nullptr;
		}

		GeoPoolT<VtxPosNrmlTx>* gp = static_cast<GeoPoolT<VtxPosNrmlTx>*>(this->renderGeoPools[VTX_POS_NRML_TX].get());

		std::unique_ptr<Mesh> mesh = std::make_unique<Mesh>(name);
		mesh->gp = gp;
		mesh->firstIndex = gp->indices.size();
		mesh->baseVertex = gp->vertexCount();
		mesh->flags = MESHFLAG_RENDERABLE | MESHFLAG_POOLED;


		const float halfWidth = width * 0.5f;
		const float halfHeight = height * 0.5f;

		// Front face is toward -Z (matches billboard code using forward = -dir)
		const glm::vec3 n(0.0f, 0.0f, -1.0f);

		VtxPosNrmlTx v0;
		v0.position = glm::vec3(-halfWidth, halfHeight, 0.0f);
		v0.normal = n;
		v0.textureCoords = glm::vec2(0.0f, 0.0f);
		v0.materialUBOIndex = 0;
		gp->vertices.push_back(v0);

		VtxPosNrmlTx v1;
		v1.position = glm::vec3(-halfWidth, -halfHeight, 0.0f);
		v1.normal = n;
		v1.textureCoords = glm::vec2(0.0f, 1.0f);
		v1.materialUBOIndex = 0;
		gp->vertices.push_back(v1);

		VtxPosNrmlTx v2;
		v2.position = glm::vec3(halfWidth, -halfHeight, 0.0f);
		v2.normal = n;
		v2.textureCoords = glm::vec2(1.0f, 1.0f);
		v2.materialUBOIndex = 0;
		gp->vertices.push_back(v2);

		VtxPosNrmlTx v3;
		v3.position = glm::vec3(halfWidth, halfHeight, 0.0f);
		v3.normal = n;
		v3.textureCoords = glm::vec2(1.0f, 0.0f);
		v3.materialUBOIndex = 0;
		gp->vertices.push_back(v3);

		// Flip indices so -Z is front (CCW when viewed from -Z)
		gp->indices.push_back(0);
		gp->indices.push_back(2);
		gp->indices.push_back(1);
		gp->indices.push_back(0);
		gp->indices.push_back(3);
		gp->indices.push_back(2);


		mesh->indexCount = mesh->gp->indices.size() - mesh->firstIndex;

		mesh->refreshAABB();

		return mesh;
	}

	/***********************************************************************************************
	* LOAD TEXTURES
	************************************************************************************************/
	std::optional<TextureData> Scene::generateTextureData(const std::string& path)
	{
		TextureData td;
		td.primaryImageData.data = stbi_load(
			path.c_str(),
			&td.primaryImageData.width,
			&td.primaryImageData.height,
			&td.primaryImageData.nrComponents,
			0
		);

		if (!td.primaryImageData.data)
			return std::nullopt;

		return td;
	}

	Texture* Scene::loadTexture(const std::string& name, const std::string& path, int options)
	{
		if (this->textures.contains(name))
		{
			SPDLOG_DEBUG("Scene::loadTexture(): Existing Texture, bypass reload: {}", name);

			return this->textures.at(name).get();
		}

		SPDLOG_DEBUG("Scene::loadTexture(): Loading new Texture: {}", name);


		std::unique_ptr<Texture> texture = std::make_unique<Texture>();
		texture->name = name;
		texture->options = options;

		// Determine if path is a directory or file, if directory then load each file in the directory as a texture frame
		if (std::filesystem::is_directory(path))
		{
			std::map<int, std::string> orderedFiles;

			for (const auto& entry : std::filesystem::directory_iterator(path))
				orderedFiles[std::stoi(vel::explode_string(entry.path().filename().string(), '.')[0])] = entry.path().string();

			for (auto& of : orderedFiles)
			{
				std::optional<TextureData> td = this->generateTextureData(of.second);

				if (!td)
				{
					SPDLOG_DEBUG("Scene::loadTexture(): failed to load all files in directory: {}", path);
					return nullptr;
				}

				texture->frames.push_back(td.value());
			}
		}
		else
		{
			std::optional<TextureData> td = this->generateTextureData(path);

			if (!td)
			{
				SPDLOG_DEBUG("Scene::loadTexture(): Unable to load texture at path: {}", path);
				return nullptr;
			}

			texture->frames.push_back(td.value());
		}


		// loop over all frames and if any of them have alpha channel, set HAS_ALPHA of texture to true
		for (auto& f : texture->frames)
		{
			if (f.alphaChannel)
			{
				texture->options |= TXT_OPT_HAS_ALPHA;
				break;
			}
		}

		Texture* rawPtr = texture.get();

		this->textures.emplace(name, std::move(texture));

		Runtime::_gpu->loadTexture(rawPtr);

		return rawPtr;
	}

	Texture* Scene::getTexture(const std::string& name)
	{
		auto it = this->textures.find(name);

		if (it == this->textures.end())
		{
			SPDLOG_ERROR("Scene::getTexture(): Attempting to get texture that does not exist: {}", name);
			return nullptr;
		}

		return it->second.get();
	}

	void Scene::removeTexture(Texture* pTexture)
	{
		auto it = this->textures.find(pTexture->name);

		if (it == this->textures.end())
			return;

		SPDLOG_DEBUG("Scene::removeTexture(): Remove Texture: {}", pTexture->name);

		Runtime::_gpu->clearTexture(pTexture);

		// texture remained in system ram after gpu load for use within engine, free it now
		if (pTexture->options & TXT_OPT_CPU_AND_GPU)
			for (auto& td : pTexture->frames)
				stbi_image_free(td.primaryImageData.data);

		this->textures.erase(pTexture->name);
	}

	/***********************************************************************************************
	* LOAD MATERIALS
	************************************************************************************************/
	Material* Scene::addMaterial(std::unique_ptr<Material> m)
	{
		if (this->materials.contains(m->getName()))
		{
			SPDLOG_DEBUG("Scene::addMaterial(): Existing Material, bypass reload: {}", m->getName());

			return this->materials.at(m->getName()).get();
		}

		SPDLOG_DEBUG("Scene::addMaterial(): Loading new Material: {}", m->getName());

		Material* rawPtr = m.get();
		this->materials.emplace(m->getName(), std::move(m));

		return rawPtr;
	}

	Material* Scene::getMaterial(const std::string& name)
	{
		auto it = this->materials.find(name);

		if (it == this->materials.end())
		{
			SPDLOG_ERROR("Scene::getMaterial(): Attempting to get material that does not exist: {}", name);
			return nullptr;
		}

		return it->second.get();
	}

	void Scene::removeMaterial(Material* pMaterial)
	{
		auto it = this->materials.find(pMaterial->getName());

		if (it == this->materials.end())
			return;

		SPDLOG_DEBUG("Scene::removeMaterial(): Remove Material: {}", pMaterial->getName());

		this->materials.erase(pMaterial->getName());
	}

	/***********************************************************************************************
	* FONTBITMAPS
	************************************************************************************************/
	float Scene::measureFontHeight(const std::string& text, FontBitmap* fb)
	{
		if (!fb)
			return 0.f;

		float offsetX = 0.0f;
		float offsetY = 0.0f;

		bool hasVisibleGlyph = false;

		float minY = 0.0f;
		float maxY = 0.0f;

		for (char c : text)
		{
			if (c == '\n')
			{
				offsetX = 0.0f;
				continue;
			}

			uint32_t character = static_cast<uint32_t>(static_cast<unsigned char>(c));

			if (character < fb->firstChar || character >= fb->firstChar + fb->charCount)
				continue;

			stbtt_aligned_quad quad;

			stbtt_GetPackedQuad(
				reinterpret_cast<stbtt_packedchar*>(fb->charInfo.get()),
				fb->textureWidth,
				fb->textureHeight,
				character - fb->firstChar,
				&offsetX,
				&offsetY,
				&quad,
				1
			);

			// Match existing text mesh convention.
			const float ymin = -quad.y1;
			const float ymax = -quad.y0;

			if (!hasVisibleGlyph)
			{
				minY = ymin;
				maxY = ymax;
				hasVisibleGlyph = true;
			}
			else
			{
				minY = std::min(minY, ymin);
				maxY = std::max(maxY, ymax);
			}
		}

		if (!hasVisibleGlyph)
			return 0.0f;

		return maxY - minY;
	}

	FontGlyphInfo Scene::getFontGlyphInfo(uint32_t character, float offsetX, float offsetY, FontBitmap* fb)
	{
		stbtt_aligned_quad quad;

		stbtt_GetPackedQuad((stbtt_packedchar*)fb->charInfo.get(), fb->textureWidth, fb->textureHeight,
			character - fb->firstChar, &offsetX, &offsetY, &quad, 1);
		const auto xmin = quad.x0;
		const auto xmax = quad.x1;
		const auto ymin = -quad.y1;
		const auto ymax = -quad.y0;

		FontGlyphInfo info{};
		info.offsetX = offsetX;
		info.offsetY = offsetY;
		info.positions[0] = { xmin, ymin, 0 };
		info.positions[1] = { xmin, ymax, 0 };
		info.positions[2] = { xmax, ymax, 0 };
		info.positions[3] = { xmax, ymin, 0 };
		info.uvs[0] = { quad.s0, quad.t1 };
		info.uvs[1] = { quad.s0, quad.t0 };
		info.uvs[2] = { quad.s1, quad.t0 };
		info.uvs[3] = { quad.s1, quad.t1 };

		return info;
	}

	FontBitmap* Scene::loadFontBitmapRaw(const std::string& fontName, int stbFontSize, const std::string& fontPath)
	{
		if (this->fontBitmaps.contains(fontName))
		{
			SPDLOG_DEBUG("Scene::loadFontBitmapRaw(): Existing FontBitmap, bypass reload: {}", fontName);

			return this->fontBitmaps.at(fontName).get();
		}

		SPDLOG_DEBUG("Scene::loadFontBitmapRaw(): Loading new FontBitmap: {}", fontName);


		std::ifstream file(fontPath, std::ios::binary | std::ios::ate);
		if (!file.is_open())
		{
			SPDLOG_DEBUG("Scene::loadFontBitmapRaw(): Failed to open file: {}", fontPath);
			return nullptr;
		}

		const auto size = file.tellg();
		file.seekg(0, std::ios::beg);
		auto bytes = std::vector<uint8_t>(size);
		file.read(reinterpret_cast<char*>(&bytes[0]), size);
		file.close();


		//
		// Build texture data
		//
		auto fontData = bytes;

		std::unique_ptr<FontBitmap> fb = std::make_unique<FontBitmap>();
		fb->fontName = fontName;
		fb->fontSize = stbFontSize;
		fb->fontPath = fontPath;
		fb->data = std::make_unique<unsigned char[]>(fb->textureWidth * fb->textureHeight);
		fb->charInfo = std::make_unique<fb_packedchar[]>(fb->charCount);


		//
		// Gather font statistics
		//
		stbtt_fontinfo fontInfo;

		int fontOffset = stbtt_GetFontOffsetForIndex(fontData.data(), 0);

		if (fontOffset < 0 || !stbtt_InitFont(&fontInfo, fontData.data(), fontOffset))
		{
			SPDLOG_DEBUG("Scene::loadFontBitmapRaw(): Failed to initialize font metrics: {}", fontPath);
			return nullptr;
		}

		int ascent;
		int descent;
		int lineGap;

		stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, &lineGap);

		float fontScale = stbtt_ScaleForPixelHeight(&fontInfo, static_cast<float>(stbFontSize));

		fb->ascent = static_cast<float>(ascent) * fontScale;
		fb->descent = static_cast<float>(descent) * fontScale;
		fb->lineGap = static_cast<float>(lineGap) * fontScale;

		fb->fontHeight = fb->ascent - fb->descent;
		fb->lineHeight = fb->fontHeight + fb->lineGap;


		//
		// Pack Atlas
		//
		int maxAtlasSize = 4096;
		bool fontPacked = false;

		while (fb->textureWidth <= maxAtlasSize)
		{
			stbtt_pack_context context;
			bool fontInitialized = stbtt_PackBegin(&context, fb->data.get(), fb->textureWidth, fb->textureHeight, 0, 1, nullptr);

			if (!fontInitialized)
			{
				SPDLOG_DEBUG("Scene::loadFontBitmapRaw(): Failed to initialize font");
				return nullptr;
			}

			stbtt_PackSetOversampling(&context, fb->oversampleX, fb->oversampleY);
			fontPacked = stbtt_PackFontRange(&context, fontData.data(), 0, fb->fontSize, fb->firstChar, fb->charCount, (stbtt_packedchar*)fb->charInfo.get());

			stbtt_PackEnd(&context);

			if (fontPacked)
				break;

			fb->textureWidth *= 2;
			fb->textureHeight *= 2;
			fb->data = std::make_unique<unsigned char[]>(fb->textureWidth * fb->textureHeight);
		}

		if (!fontPacked)
		{
			SPDLOG_DEBUG("Scene::loadFontBitmapRaw(): Failed to pack font");
			return nullptr;
		}


		FontBitmap* rawPtr = fb.get();
		this->fontBitmaps.emplace(fb->fontName, std::move(fb));

		Runtime::_gpu->loadFontBitmapTexture(rawPtr);

		return rawPtr;
	}

	FontBitmap* Scene::loadFontBitmap(const std::string& fontName, int fontSize, const std::string& fontPath)
	{
		return this->loadFontBitmapRaw(fontName, fontSize, fontPath);
	}

	FontBitmap* Scene::loadFontBitmapVisualHeight(const std::string& fontName, int desiredVisiblePx, const std::string& fontPath)
	{
		if (this->fontBitmaps.contains(fontName))
		{
			SPDLOG_DEBUG("Scene::loadFontBitmapVisualHeight(): Existing FontBitmap, bypass reload: {}", fontName);

			return this->fontBitmaps.at(fontName).get();
		}

		SPDLOG_DEBUG("Scene::loadFontBitmapVisualHeight(): Loading new FontBitmap: {}", fontName);


		const std::string referenceText = "Hg";

		FontBitmap* testFont = this->loadFontBitmapRaw(fontName + "_calibration", desiredVisiblePx, fontPath);

		float measuredHeight = this->measureFontHeight(referenceText, testFont);

		if (measuredHeight <= 0.0f)
			return testFont;

		float correction = desiredVisiblePx / measuredHeight;
		int correctedSize = (int)std::round(desiredVisiblePx * correction);

		this->removeFontBitmap(testFont);

		return this->loadFontBitmapRaw(fontName, correctedSize, fontPath);
	}

	FontBitmap* Scene::getFontBitmap(const std::string& name)
	{
		auto it = this->fontBitmaps.find(name);

		if (it == this->fontBitmaps.end())
		{
			SPDLOG_ERROR("Scene::getFontBitmap(): Attempting to get FontBitmap that does not exist: {}", name);
			return nullptr;
		}

		return it->second.get();
	}

	void Scene::removeFontBitmap(FontBitmap* pFontBitmap)
	{
		auto it = this->fontBitmaps.find(pFontBitmap->fontName);

		if (it == this->fontBitmaps.end())
			return;

		SPDLOG_DEBUG("Scene::removeFontBitmap(): Remove FontBitmap: {}", pFontBitmap->fontName);

		Runtime::_gpu->clearTexture(&pFontBitmap->texture);

		this->fontBitmaps.erase(pFontBitmap->fontName);
	}



} // END VEL NAMESPACE