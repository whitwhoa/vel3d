
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
		audioGroupKey(-1),
		sceneRenderTarget(),
		bufferIds()
	{
		// TODO: did we forget one?
		this->renderGeoPools.emplace(VtxLayout::VTX_POS_NRML, std::make_unique<GeoPoolT<VtxPosNrml>>());
		this->renderGeoPools.emplace(VtxLayout::VTX_POS_NRML_TX, std::make_unique<GeoPoolT<VtxPosNrmlTx>>());
		this->renderGeoPools.emplace(VtxLayout::VTX_POS_NRML_TX_LM, std::make_unique<GeoPoolT<VtxPosNrmlTxLm>>());
		this->renderGeoPools.emplace(VtxLayout::VTX_POS_NRML_TX_SKN, std::make_unique<GeoPoolT<VtxPosNrmlTxSkn>>());


		this->sceneRenderTarget = Runtime::_gpu->createFinalRenderTarget(
			Runtime::_window->getWindowSize().x, Runtime::_window->getWindowSize().y);

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
		SPDLOG_DEBUG("Freeing assets for scene: {}", this->getId());

		Runtime::_gpu->freeSceneBuffers(this->bufferIds);

		for (auto& s : this->stages)
		{
			for (auto& ob : s->opaqueBuckets)
				Runtime::_gpu->deleteBuffer(&ob.indirectBuffer);

			for (auto& tb : s->transparentBuckets)
				Runtime::_gpu->deleteBuffer(&tb.indirectBuffer);
		}

		for (auto& t : this->textures)
		{
			if (t.flags & TXTRFLG_RT_WRAPPER)
				continue;

			Runtime::_gpu->clearTexture(t);

			if (t.flags & TXTRFLG_CPU_AND_GPU)
				stbi_image_free(t.data);
		}

		for (auto& s : this->shaders)
			Runtime::_gpu->clearShader(s.programId);

		for (auto& m : this->meshes)
		{
			if ((m.second->flags & MESHFLAG_RENDERABLE) && !(m.second->flags & MESHFLAG_POOLED))
				Runtime::_gpu->clearGeoPool(m.second->gp->gpuGeoPool.value());
		}

		for (auto& s : this->soundsInUse)
			Runtime::_audioDevice->removeSound(s);

		for (auto& c : this->cameras)
			Runtime::_gpu->clearRenderTarget(c->renderTarget);

		Runtime::_gpu->freeFinalRenderTarget(this->sceneRenderTarget);
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

			// TODO: load immutable scene buffers

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
		std::optional<FinalRenderTarget> updatedFRT = Runtime::_gpu->updateFinalRenderTargetVPSize(
			this->sceneRenderTarget, 
			Runtime::_window->getWindowSize().x,
			Runtime::_window->getWindowSize().y
		);

		if (updatedFRT)
			this->sceneRenderTarget = updatedFRT.value();


		Runtime::_gpu->setFinalRenderTarget(this->sceneRenderTarget);


		for (auto& c : this->cameras)
			if (c->finalRenderCam)
				Runtime::_gpu->drawToFinalRenderTarget(c->renderTarget.opaqueDsaHandle);
		

		// call post process to apply post process shader while drawing into the default framebuffer for display to screen
		Runtime::_gpu->setDefaultFrameBuffer();
		Runtime::_gpu->drawToScreen(this->sceneRenderTarget);

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

} // END VEL NAMESPACE