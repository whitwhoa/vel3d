#include <iostream>
#include <fstream>

#define GLM_FORCE_ALIGNED_GENTYPES
#include <glm/gtx/string_cast.hpp>
#include "imgui/imgui.h"


#include "vel/Scene.h"
#include "vel/Vertex.h"
#include "vel/Texture.h"
#include "nlohmann/json.hpp"
#include "vel/CollisionObjectTemplate.h"
#include "vel/functions.h"
#include "vel/Log.h"


using json = nlohmann::json;

namespace vel
{
	Scene::Scene() :
		inputState(nullptr),
		animationTime(0.0f),
		screenColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)),
		HeadlessScene()
	{
		
	}
	
	Scene::~Scene()
	{
		this->freeAssets();
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

		Camera* rawPtrCamera = this->assetManager->addCamera(std::move(c));

		this->camerasInUse.push_back(rawPtrCamera);

		return rawPtrCamera;
	}

	Camera* Scene::getCamera(const std::string& name)
	{
		return this->assetManager->getCamera(name);
	}


	void Scene::freeAssets()
	{
		LOG_TO_CLI_AND_FILE("Freeing assets for scene: " + this->name);

		for(auto& pArm : this->armaturesInUse)
			this->assetManager->removeArmature(pArm);
        
		for (auto& pCam : this->camerasInUse)
			this->assetManager->removeCamera(pCam);
		
		for(auto& pMaterial : this->materialsInUse)
			this->assetManager->removeMaterial(pMaterial);
		
		for(auto& pTexture : this->texturesInUse)
			this->assetManager->removeTexture(pTexture);

		for (auto& pFontBitmap : this->fontBitmapsInUse)
			this->assetManager->removeFontBitmap(pFontBitmap);
		
		for(auto& pMesh : this->meshesInUse)
			this->assetManager->removeMesh(pMesh);
        
		for(auto& pShader : this->shadersInUse)
			this->assetManager->removeShader(pShader);

		for (auto& cw : this->collisionWorlds)
			delete cw;

	}

	void Scene::setScreenColor(glm::vec4 c)
	{
		this->screenColor = c;
	}

	void Scene::clearScreenColor()
	{
		this->screenColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	}

	FontBitmap* Scene::loadFontBitmap(const std::string& fontName, int fontSize, const std::string& fontPath)
	{
		FontBitmap* fb = this->assetManager->loadFontBitmap(fontName, fontSize, fontPath);

		this->fontBitmapsInUse.push_back(fb);

		return fb;
	}

	void Scene::loadShader(const std::string& name, const std::string& vertFile, const std::string& fragFile)
	{
		this->shadersInUse.push_back(this->assetManager->loadShader(name, vertFile, fragFile));
	}
	
	Texture* Scene::loadTexture(const std::string& name, const std::string& path, bool freeAfterGPULoad, unsigned int uvWrapping)
	{
		Texture* t = this->assetManager->loadTexture(name, path, freeAfterGPULoad, uvWrapping);

		this->texturesInUse.push_back(t);

		return t;
	}
	
	Material* Scene::addMaterial(const std::string& name)
	{
		std::unique_ptr<Material> m = std::make_unique<Material>(name);
		Material* pMaterial = this->assetManager->addMaterial(std::move(m));
		this->materialsInUse.push_back(pMaterial);
		return pMaterial;
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
	void Scene::updateAnimations(float delta)
	{
		this->animationTime += delta;
		for (auto& s : this->stages)
			s->updateArmatureAnimations(this->animationTime);
	}

	// Has to be this way, otherwise Scene can't track the generated mesh. Live with it.
	TextActor* Scene::addTextActor(Stage* stage, const std::string& name, const std::string& theText,
		FontBitmap* fb, TextActorAlignment alignment, glm::vec4 color)
	{
		// create the TextActor
		std::unique_ptr<TextActor> ta = std::make_unique<TextActor>();
		ta->name = name;
		ta->text = theText;
		ta->fontBitmap = fb;
		ta->alignment = alignment;

		// create the mesh using provided FontBitmap and text string
		Mesh* pTam = this->assetManager->addMesh(std::move(this->assetManager->loadTextActorMesh(ta.get())));
		this->meshesInUse.push_back(pTam);

		// create material
		Material* taMaterial = this->addMaterial(name + "_material");
		taMaterial->addTexture(&fb->texture);

		// create actor
		Actor* pTextActor = stage->addActor(name);
		pTextActor->setColor(color);
		pTextActor->setDynamic(false);
		pTextActor->setVisible(true);
		pTextActor->setShader(this->getShader("textShader"));
		pTextActor->setMesh(pTam);
		pTextActor->setMaterial(*taMaterial);
		
		// add actor pointer to TextActor.actor
		ta->actor = pTextActor;

		// add new text actor to stage and return pointer
		return stage->addTextActor(std::move(ta));
	}

	void Scene::updateTextActors()
	{
		for (auto& s : this->stages)
			s->updateTextActors();
	}

	void Scene::updatePreviousTransforms()
	{
		for (auto& s : this->stages)
			s->updatePreviousTransforms();
	}

	void Scene::draw(GPU* gpu, float frameTime, float alpha)
	{
		gpu->enableDepthTest(); // insure depth buffer is active
		gpu->enableBackfaceCulling(); // insure backface culling is occurring
        gpu->disableBlend(); // disable blending for opaque objects

		// loop through all stages
		for (auto& s : this->stages)
		{
			bool actorsFirstPass = true;

			if (!s->isVisible())
				continue;

			if (s->getClearDepthBuffer())
				gpu->clearDepthBuffer();

			for (auto c : s->getCameras())
			{
				// update stage camera (view/projection matrices), update scene's camera data to this stage's camera data
				c->update();
				this->cameraPosition = c->getPosition();
				this->cameraProjectionMatrix = c->getProjectionMatrix();
				this->cameraViewMatrix = c->getViewMatrix();


				// setup gl state to render to framebuffer
				gpu->setRenderTarget(c->getRenderTarget()->FBO, true); // should always write to depth buffer here
				gpu->updateViewportSize(c->getResolution().x, c->getResolution().y);
				
				// loop through all renderables and build a vector of actors which use an alpha channel, draw opaques
				this->transparentActors.clear();

				for (auto& a : s->getActors())
				{
					if (!a->isVisible())
						continue;

					if (actorsFirstPass && a->getMaterial().has_value() && a->getMaterial()->getMaterialAnimator().has_value())
						a->getMaterial()->getMaterialAnimator()->update(frameTime);

					// Pool transparents/translucents for render after opaques
					if ((a->getColor().w < 1.0f) || (a->getMaterial().has_value() && a->getMaterial()->getHasAlphaChannel()))
					{
						float dist = glm::length(this->cameraPosition - a->getTransform().getTranslation());
						this->transparentActors.push_back(std::pair<float, Actor*>(dist, a.get()));

						continue;
					}

					// Draw Opaques

					// Reset Gpu state for this actor
					gpu->useShader(a->getShader());
					gpu->useMesh(a->getMesh());

					this->drawActor(gpu, a.get(), alpha);
				}

				actorsFirstPass = false;

				// Draw transparents/translucents
				gpu->enableBlend2();

				// not proud of this, but it gets the job done for the time being, loop through all transparent actors and sort by their distance
				// from the current camera position
				std::sort(transparentActors.begin(), transparentActors.end(), [](auto &left, auto &right) {
					return left.first < right.first;
				});

				// Draw all transparent/translucent actors
				for (std::vector<std::pair<float, Actor*>>::reverse_iterator it = transparentActors.rbegin(); it != transparentActors.rend(); ++it)
				{
					// Reset gpu state for this ACTOR and draw
					auto a = it->second;;

					gpu->useShader(a->getShader());
					gpu->useMesh(a->getMesh());

					this->drawActor(gpu, it->second, alpha);
				}

			} // end for each camera

		} // end for each stage

		// all stage camera's framebuffers are now updated, loop through each stage camera and check if it should display it's contents 

		// now bind back to default framebuffer and draw a quad plane with the attached framebuffer texture
		// enable blending of each renderable stage "layer"
		gpu->setRenderTarget(0, false);
		gpu->enableBlend2();
		gpu->updateViewportSize(this->getWindowSize().x, this->getWindowSize().y);

		for (auto& s : this->stages)
		{
			if (!s->isVisible())
				continue;

			for (auto c : s->getCameras())
				if (c->isFinalRenderCam())
					gpu->drawScreen(c->getRenderTarget()->texture.frames.at(0).dsaHandle, this->screenColor);
		}

		// moving collision debug draw event as final thing as it draws directly to the screen buffer, and I don't want to have to 
		// think about updating it right now
#ifdef DEBUG_LOG
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
#endif


	}

	void Scene::drawActor(GPU* gpu, Actor* a, float alphaTime)
	{
		//if (a->isVisible())
		//{
			if (a->getMaterial().has_value())
				gpu->useMaterial(&a->getMaterial().value());

			gpu->setShaderVec4("color", a->getColor());

			//gpu->setShaderMat4("mvp", this->cameraProjectionMatrix * this->cameraViewMatrix * a->getWorldRenderMatrix(alphaTime));
			gpu->setShaderMat4("model", a->getWorldRenderMatrix(alphaTime));
			gpu->setShaderMat4("view", this->cameraViewMatrix);
			gpu->setShaderMat4("projection", this->cameraProjectionMatrix);

			if (a->getGIColors().size() == 6)
				gpu->setShaderVec3Array("giColors", a->getGIColors());
				
			if (a->getLightMapTexture() == nullptr)
				gpu->updateLightMapTextureUBO(this->assetManager->getTexture("defaultWhite")->frames.at(0).dsaHandle);	
			else
				gpu->updateLightMapTextureUBO(a->getLightMapTexture()->frames.at(0).dsaHandle);
				

			// If this actor is animated, send the bone transforms of it's armature to the shader
			if (a->isAnimated())
			{
				auto mesh = a->getMesh();
				auto armature = a->getArmature();
				bool armInterp = armature->getShouldInterpolate();

				size_t boneIndex = 0;
				std::vector<std::pair<unsigned int, glm::mat4>> boneData;

				glm::mat4 meshBoneTransform;
				for (auto& activeBone : a->getActiveBones())
				{
					if(armInterp)
						// global inverse matrix does not seem to make any difference
						//meshBoneTransform = mesh->getGlobalInverseMatrix() * armature->getBone(activeBone.first).getRenderMatrixInterpolated(alphaTime) * mesh->getBone(boneIndex).offsetMatrix;
						meshBoneTransform = armature->getBone(activeBone.first).getRenderMatrixInterpolated(alphaTime) * mesh->getBone(boneIndex).offsetMatrix;
					else
						meshBoneTransform = armature->getBone(activeBone.first).getRenderMatrix() * mesh->getBone(boneIndex).offsetMatrix;

					boneData.push_back(std::pair<unsigned int, glm::mat4>(activeBone.second, meshBoneTransform));
					boneIndex++;
				}

				gpu->updateBonesUBO(boneData);
			}
			
			gpu->drawGpuMesh();
		//}
	}

	void Scene::clearAllRenderTargetBuffers(GPU* gpu)
	{
		for (auto& s : this->stages)
		{
			for (auto c : s->getCameras())
			{
				gpu->setRenderTarget(c->getRenderTarget()->FBO, true);
				gpu->clearBuffers(0.0f, 0.0f, 0.0f, 0.0f);
			}
		}

		// clear default screen buffer
		gpu->setRenderTarget(0, false);
		gpu->clearBuffers(0.0f, 0.0f, 0.0f, 1.0f);
	}

// END VEL NAMESPACE
}