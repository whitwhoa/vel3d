
#include <stb_headers/stb_image.h>

#include <spdlog/spdlog.h>

#include <glm/gtx/string_cast.hpp>

#include <nlohmann/json.hpp>

#include <vel/Util/Assert.h>
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
		this->renderGeoPools.emplace(VtxLayout::VTX_POS, std::make_unique<GeoPoolT<VtxPos>>());
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

		for (auto& rsp : this->renderSoloGeoPools)
		{
			if (rsp.second->gpuGeoPool)
				Runtime::_gpu->clearGeoPool(rsp.second->gpuGeoPool.value());
		}
			
		for (auto& rp : this->renderGeoPools)
		{
			if (rp.second->gpuGeoPool)
				Runtime::_gpu->clearGeoPool(rp.second->gpuGeoPool.value());
		}

		Runtime::_audioDevice->removeGroup(static_cast<unsigned int>(this->audioGroupKey));

		for (const auto& name : this->sfxInUse)
			Runtime::_audioDevice->removeSfx(name);

		for (const auto& name : this->bgmInUse)
			Runtime::_audioDevice->removeBgm(name);


		for (auto& c : this->cameras)
			Runtime::_gpu->clearRenderTarget(c->renderTarget);

		Runtime::_gpu->freeFinalRenderTarget(this->sceneRenderTarget);
	}

	unsigned int HeadlessScene::getId() const
	{
		return this->id;
	}

	int Scene::getAudioGroupKey() const
	{
		return this->audioGroupKey;
	}

	void HeadlessScene::internalFixedLoop(float deltaTime)
	{
		this->fixedLoop(deltaTime);
	}

	void Scene::internalImmediateLoop(float frameTime, float renderLerpInterval)
	{
		this->immediateLoop(frameTime, renderLerpInterval);

		for (auto& camera : this->cameras)
		{
			camera->update();
			this->refreshCameraTexture(*camera);
		}
	}

	void Scene::initMaterialData()
	{ 
		this->materialsGpu.clear();
		this->materialsGpu.reserve(this->materials.size());

		this->materialTexturesGpu.clear();
		this->materialTextureSlots.clear();

		for (const Material& material : this->materials)
		{
			MaterialGpuData gpuData = {
				.flags = material.flags,
				.f1 = material.f1,
				.f2 = material.f2
			};

			gpuData.textureOffset = this->materialTexturesGpu.size();
			gpuData.textureCount = material.textures.size();

			VEL_ASSERT(
				(
					((material.flags & MTLFLG_HAS_TEXTURES) && gpuData.textureCount > 0) || 
					(!(material.flags & MTLFLG_HAS_TEXTURES) && gpuData.textureCount == 0)
				),
			"Attempting to upload material flagged as having textures, that has no textures");

			for (texture_handle th : material.textures)
			{
				this->materialTextureSlots[th].push_back(static_cast<uint32_t>(this->materialTexturesGpu.size()));
				this->materialTexturesGpu.push_back(this->textures[th].dsaHandle);
			}
				
			this->materialsGpu.push_back(gpuData);
		}

		if (!this->materialsGpu.empty())
		{
			Runtime::_gpu->uploadStaticBufferData(
				this->bufferIds.materialDataSsbo,
				this->materialsGpu.size() * sizeof(MaterialGpuData),
				this->materialsGpu.data()
			);
		}

		if (!this->materialTexturesGpu.empty())
		{
			Runtime::_gpu->uploadStaticBufferData(
				this->bufferIds.materialTextureHandlesSsbo,
				this->materialTexturesGpu.size() * sizeof(uint64_t),
				this->materialTexturesGpu.data()
			);
		}
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

			this->initMaterialData();

			for (auto& renderGeoPoolKV : this->renderGeoPools)
				Runtime::_gpu->loadGeoPool(renderGeoPoolKV.second.get());

			for (auto& renderSoloGeoPoolKV : this->renderSoloGeoPools)
				Runtime::_gpu->loadGeoPool(renderSoloGeoPoolKV.second.get());

			this->initActorDrawBuckets();

			return true;
		}

		return false;
	}

	DrawBucket& Scene::getDrawBucket(Stage& stage, const DrawBucketLocation& location)
	{
		switch (location.pass)
		{
		case RENDER_PASS_OPAQUE:
			return stage.opaqueBuckets[location.index];
		case RENDER_PASS_TRANSPARENT:
			return stage.transparentBuckets[location.index];
		default:
			VEL_ASSERT(false, "Scene::getDrawBucket(): Invalid render pass.");
		}
	}

	void Scene::uploadDrawBuckets(std::vector<DrawBucket>& buckets)
	{
		for (DrawBucket& bucket : buckets)
		{
			if (bucket.drawCommands.empty())
				continue;

			Runtime::_gpu->uploadStreamBufferData(
				bucket.indirectBuffer, 
				bucket.drawCommands.size() * sizeof(DrawBucketCommand),
				bucket.drawCommands.data()
			);
		}
	}

	void Scene::draw(float frameTime, float alpha)
	{
		Runtime::_gpu->bindSceneBuffers(this->bufferIds);

		// ---------------------------------------------------------
		// 1. Clear CPU-side frame data.
		// ---------------------------------------------------------
		this->actorsGpu.clear();
		this->actorAmbientCube.clear();
		this->actorBoneMatrices.clear();

		for (auto& stage : this->stages)
		{
			for (DrawBucket& bucket : stage->opaqueBuckets)
				bucket.drawCommands.clear();

			for (DrawBucket& bucket : stage->transparentBuckets)
				bucket.drawCommands.clear();
		}

		// ---------------------------------------------------------
		// 2. Iterate each visible Actor ONCE.
		// ---------------------------------------------------------
		for (auto& actor : this->actors)
		{
			if (!(actor.flags & ACTFLG_VISIBLE))
				continue;

			Stage& stage = *actor.stage;

			if (!stage.enabled)
				continue;

			if (actor.flags & ACTFLG_ANIMATED_MATERIAL)
				for (auto& materialAnimator : actor.materialAnimators)
					materialAnimator.update(frameTime);

			const uint32_t actorDataIndex = static_cast<uint32_t>(this->actorsGpu.size());
			const uint32_t ambientCubeOffset = static_cast<uint32_t>(this->actorAmbientCube.size());
			const uint32_t boneMatrixOffset = static_cast<uint32_t>(this->actorBoneMatrices.size());

			for (const glm::vec3& value : actor.ambientCube)
				this->actorAmbientCube.push_back(glm::vec4(value, 1.0f));

			if (actor.animator != nullptr)
			{
				this->actorBoneMatrices.resize(this->actorBoneMatrices.size() + actor.activeBones.size(), glm::mat4(1.0f));

				for (auto& activeBone : actor.activeBones)
				{
					this->actorBoneMatrices[boneMatrixOffset + activeBone.second] =
						ozzFloat4x4ToGlmMat4(actor.animator->getRenderBoneMatrix(activeBone.first)) *
						actor.mesh->bones[activeBone.second].offsetMatrix;
				}
			}

			ActorGpuData agd;
			agd.model = this->getActorWorldRenderMatrix(actor, alpha);
			agd.colorMultiplier = actor.colorMultiplier;
			agd.lightmapHandle = actor.lightmapTexture != INVALID_TEXTURE_HANDLE ? this->textures[actor.lightmapTexture].dsaHandle : 0;
			agd.flags = actor.flags;
			agd.ambientCubeOffset = ambientCubeOffset;
			agd.boneMatrixOffset = boneMatrixOffset;

			this->actorsGpu.push_back(agd);

			//this->actorsGpu.push_back({
			//	.model                  = this->getActorWorldRenderMatrix(actor, alpha),
			//	.colorMultiplier        = actor.colorMultiplier,
			//	.lightmapHandle         = actor.lightmapTexture ? this->textures[actor.lightmapTexture].dsaHandle : 0,
			//	.flags                  = actor.flags,
			//	.ambientCubeOffset      = ambientCubeOffset,
			//	.boneMatrixOffset       = boneMatrixOffset
			//});

			const Mesh& mesh = *actor.mesh;

			for (const MeshSection& section : mesh.sections)
			{
				if (section.indexCount == 0)
					continue;

				DrawBucket& bucket = getDrawBucket(stage, actor.drawBuckets[section.actorMaterialIndex]);

				DrawBucketCommand drawCommand = {
					.count = section.indexCount,
					.instanceCount = 1,
					.firstIndex = section.firstIndex,
					.baseVertex = mesh.baseVertex,
					.baseInstance = actorDataIndex,

					.materialIndex = actor.materialIndices[section.actorMaterialIndex],
					.activeFrame = (actor.flags & ACTFLG_ANIMATED_MATERIAL) ? actor.materialAnimators[section.actorMaterialIndex].getCurrentFrame() : 0
				};

				bucket.drawCommands.push_back(drawCommand);
			}
		}

		// ---------------------------------------------------------
		// 3. Upload Actor data.
		// ---------------------------------------------------------
		if (!this->actorsGpu.empty())
		{
			Runtime::_gpu->uploadStreamBufferData(
				this->bufferIds.actorDataSsbo,
				this->actorsGpu.size() * sizeof(ActorGpuData),
				this->actorsGpu.data()
			);
		}

		// ---------------------------------------------------------
		// 4. Upload Actor ambient cube data.
		// ---------------------------------------------------------
		if (!this->actorAmbientCube.empty())
		{
			Runtime::_gpu->uploadStreamBufferData(
				this->bufferIds.actorAmbientCubeSsbo,
				this->actorAmbientCube.size() * sizeof(glm::vec4),
				this->actorAmbientCube.data()
			);
		}

		// ---------------------------------------------------------
		// 5. Upload Actor bone matrix data.
		// ---------------------------------------------------------
		if (!this->actorBoneMatrices.empty())
		{
			Runtime::_gpu->uploadStreamBufferData(
				this->bufferIds.actorBoneMatricesSsbo,
				this->actorBoneMatrices.size() * sizeof(glm::mat4),
				this->actorBoneMatrices.data()
			);
		}

		// ---------------------------------------------------------
		// 6. Upload every bucket's combined indirect commands + Material indices + active frames.
		// ---------------------------------------------------------
		for (auto& stage : this->stages)
		{
			if (!stage->enabled)
				continue;

			this->uploadDrawBuckets(stage->opaqueBuckets);
			this->uploadDrawBuckets(stage->transparentBuckets);
		}

		// ---------------------------------------------------------
		// 7. Render every Stage through every camera.
		// ---------------------------------------------------------
		for (auto& s : this->stages)
		{
			Stage& stage = *s;

			if (!stage.enabled)
				continue;


			for (auto& c : stage.cameras)
			{
				Camera& camera = *c;

				Runtime::_gpu->uploadBufferSubData(this->bufferIds.cameraUbo, 0, sizeof(CameraGpuData), &camera.gpuData);

				Runtime::_gpu->setOpaqueRenderState(camera.renderTarget);
				for (const DrawBucket& bucket : stage.opaqueBuckets)
					Runtime::_gpu->submitDrawBucket(bucket);
				
				Runtime::_gpu->setTransparentRenderState(camera.renderTarget);
				for (const DrawBucket& bucket : stage.transparentBuckets)
					Runtime::_gpu->submitDrawBucket(bucket);

				Runtime::_gpu->composeFBOs(camera.renderTarget);
			}

		}
		
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
		glm::ivec2 windowSize = Runtime::_window->getWindowSize();
		Runtime::_gpu->setDefaultFrameBuffer(windowSize.x, windowSize.y);
		Runtime::_gpu->drawToScreen(this->sceneRenderTarget);
	}

} // END VEL NAMESPACE