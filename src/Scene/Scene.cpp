
#include <stb_headers/stb_image.h>

#include <spdlog/spdlog.h>

#include <glm/gtx/string_cast.hpp>

#include <nlohmann/json.hpp>

#include <vel/Runtime.h>
#include <vel/Util/functions.h>
#include <vel/Scene/CollisionWorld/CollisionObjectTemplate.h>
#include <vel/Scene/Scene.h>
#include <vel/Scene/Texture.h>
#include <vel/Scene/MeshLoader/AssimpMeshLoader.h>

using json = nlohmann::json;


namespace vel
{
	HeadlessScene::HeadlessScene() :
		id(Runtime::_nextId++),
		meshLoader(std::make_unique<AssimpMeshLoader>()) {
	}

	Scene::Scene() :
		HeadlessScene(),
		sceneRenderTarget(nullptr),
		audioGroupKey(-1),
		bufferIds()
	{
		// TODO: did we forget one?
		this->renderGeoPools.emplace(VtxLayout::VTX_POS_NRML, std::make_unique<GeoPoolT<VtxPosNrml>>());
		this->renderGeoPools.emplace(VtxLayout::VTX_POS_NRML_TX, std::make_unique<GeoPoolT<VtxPosNrmlTx>>());
		this->renderGeoPools.emplace(VtxLayout::VTX_POS_NRML_TX_LM, std::make_unique<GeoPoolT<VtxPosNrmlTxLm>>());
		this->renderGeoPools.emplace(VtxLayout::VTX_POS_NRML_TX_SKN, std::make_unique<GeoPoolT<VtxPosNrmlTxSkn>>());


		this->sceneRenderTarget = Runtime::_gpu->createFinalRenderTarget(
			"sceneRenderTarget_" + this->getId(),
			Runtime::_window->getWindowSize().x,
			Runtime::_window->getWindowSize().y
		);

		if(Runtime::_audioDevice)
			this->audioGroupKey = Runtime::_audioDevice->generateGroupKey();
	}
	
	HeadlessScene::~HeadlessScene()
	{
		for (auto& cw : this->collisionWorlds)
			delete cw;
	}

	Scene::~Scene()
	{
		/*
			!!!!!! TODO: YOU MUST REMEMBER TO UPDATE THIS ONCE WE HAVE EVERYTHING ELSE FIGURED OUT !!!!!!!!!
		*/

		SPDLOG_DEBUG("Freeing assets for scene: {}", this->getId());

		// Note: we repeat some code here to optimize for destruction. If we called the
		// explicit removal methods of each asset type, that's an additional method call,
		// plus validation check, plus logic, plus another hash lookup for the erase. Doing
		// it this way, we clear only what needs cleared, then let memory be freed when Scene
		// goes out of scope (ie is removed from App)

		// Note: materials do not require explicit removal

		for (auto& t : this->textures)
		{
			Runtime::_gpu->clearTexture(t.second.get());

			if (t.second->flags & TXT_OPT_CPU_AND_GPU)
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

	unsigned int HeadlessScene::getId() const
	{
		return this->id;
	}

	void HeadlessScene::internalFixedLoop(float deltaTime)
	{
		this->fixedLoop(deltaTime);
	}

	void Scene::internalImmediateLoop(float frameTime, float renderLerpInterval)
	{
		this->immediateLoop(frameTime, renderLerpInterval);
	}

	bool HeadlessScene::internalLoad()
	{
		return this->load();
	}

	bool Scene::internalLoad()
	{
		if (HeadlessScene::internalLoad())
		{
			Runtime::_gpu->initSceneBuffers(this->bufferIds);

			for (auto& renderGeoPoolKV : this->renderGeoPools)
				Runtime::_gpu->loadGeoPool(renderGeoPoolKV.second.get());

			for (auto& renderSoloGeoPoolKV : this->renderSoloGeoPools)
				Runtime::_gpu->loadGeoPool(renderSoloGeoPoolKV.second.get());

			return true;
		}

		return false;
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

	

	/***********************************************************************************************
	* SHADERS (this will not be required after we refactor for MDI)
	************************************************************************************************/
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

	void Scene::removeShader(Shader* pShader)
	{
		auto it = this->shaders.find(pShader->name);

		if (it == shaders.end())
			return;

		Runtime::_gpu->clearShader(it->second.get());
		this->shaders.erase(pShader->name);
	}


	

	

	




} // END VEL NAMESPACE