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
	


	DiffuseMaterial* Scene::addDiffuseMaterial(const std::string& name)
	{
		Shader* diffuseMaterialShader = this->assetManager->loadShader("diffuseMaterialShader", 
			"uber.vert", "uber.frag", DiffuseMaterial::shaderDefs); // returns existing if already loaded

		std::unique_ptr<DiffuseMaterial> m = std::make_unique<DiffuseMaterial>(name, diffuseMaterialShader);

		Material* pMaterial = this->assetManager->addMaterial(std::move(m));
		this->materialsInUse.push_back(pMaterial);

		return static_cast<DiffuseMaterial*>(pMaterial);
	}

	DiffuseLightmapMaterial* Scene::addDiffuseLightmapMaterial(const std::string& name)
	{
		Shader* diffuseLightmapMaterialShader = this->assetManager->loadShader("diffuseLightmapMaterialShader",
			"uber.vert", "uber.frag", DiffuseLightmapMaterial::shaderDefs); // returns existing if already loaded

		std::unique_ptr<DiffuseLightmapMaterial> m = std::make_unique<DiffuseLightmapMaterial>(name, diffuseLightmapMaterialShader);

		Material* pMaterial = this->assetManager->addMaterial(std::move(m));
		this->materialsInUse.push_back(pMaterial);

		return static_cast<DiffuseLightmapMaterial*>(pMaterial);
	}

	DiffuseAnimatedMaterial* Scene::addDiffuseAnimatedMaterial(const std::string& name)
	{
		Shader* diffuseAnimatedMaterialShader = this->assetManager->loadShader("diffuseAnimatedMaterialShader",
			"uber.vert", "uber.frag", DiffuseAnimatedMaterial::shaderDefs); // returns existing if already loaded

		std::unique_ptr<DiffuseAnimatedMaterial> m = std::make_unique<DiffuseAnimatedMaterial>(name, diffuseAnimatedMaterialShader);

		Material* pMaterial = this->assetManager->addMaterial(std::move(m));
		this->materialsInUse.push_back(pMaterial);

		return static_cast<DiffuseAnimatedMaterial*>(pMaterial);
	}

	DiffuseAnimatedLightmapMaterial* Scene::addDiffuseAnimatedLightmapMaterial(const std::string& name)
	{
		Shader* diffuseAnimatedLightmapMaterialShader = this->assetManager->loadShader("diffuseAnimatedLightmapMaterialShader",
			"uber.vert", "uber.frag", DiffuseAnimatedLightmapMaterial::shaderDefs); // returns existing if already loaded

		std::unique_ptr<DiffuseAnimatedLightmapMaterial> m = std::make_unique<DiffuseAnimatedLightmapMaterial>(name, diffuseAnimatedLightmapMaterialShader);

		Material* pMaterial = this->assetManager->addMaterial(std::move(m));
		this->materialsInUse.push_back(pMaterial);

		return static_cast<DiffuseAnimatedLightmapMaterial*>(pMaterial);
	}

	DiffuseSkinnedMaterial* Scene::addDiffuseSkinnedMaterial(const std::string& name)
	{
		Shader* diffuseSkinnedMaterialShader = this->assetManager->loadShader("diffuseSkinnedMaterialShader",
			"uber.vert", "uber.frag", DiffuseSkinnedMaterial::shaderDefs); // returns existing if already loaded

		std::unique_ptr<DiffuseSkinnedMaterial> m = std::make_unique<DiffuseSkinnedMaterial>(name, diffuseSkinnedMaterialShader);

		Material* pMaterial = this->assetManager->addMaterial(std::move(m));
		this->materialsInUse.push_back(pMaterial);

		return static_cast<DiffuseSkinnedMaterial*>(pMaterial);
	}

	TextMaterial* Scene::addTextMaterial(const std::string& name)
	{
		Shader* textMaterialShader = this->assetManager->loadShader("textMaterialShader",
			"uber.vert", "uber.frag", TextMaterial::shaderDefs); // returns existing if already loaded

		std::unique_ptr<TextMaterial> m = std::make_unique<TextMaterial>(name, textMaterialShader);

		Material* pMaterial = this->assetManager->addMaterial(std::move(m));
		this->materialsInUse.push_back(pMaterial);

		return static_cast<TextMaterial*>(pMaterial);
	}

	DiffuseAmbientCubeMaterial* Scene::addDiffuseAmbientCubeMaterial(const std::string& name)
	{
		Shader* diffuseAmbientCubeMaterialShader = this->assetManager->loadShader("diffuseAmbientCubeMaterialShader",
			"uber.vert", "uber.frag", DiffuseAmbientCubeMaterial::shaderDefs); // returns existing if already loaded

		std::unique_ptr<DiffuseAmbientCubeMaterial> m = std::make_unique<DiffuseAmbientCubeMaterial>(name, diffuseAmbientCubeMaterialShader);

		Material* pMaterial = this->assetManager->addMaterial(std::move(m));
		this->materialsInUse.push_back(pMaterial);

		return static_cast<DiffuseAmbientCubeMaterial*>(pMaterial);
	}

	DiffuseAmbientCubeSkinnedMaterial* Scene::addDiffuseAmbientCubeSkinnedMaterial(const std::string& name)
	{
		Shader* diffuseAmbientCubeSkinnedMaterialShader = this->assetManager->loadShader("diffuseAmbientCubeSkinnedMaterialShader",
			"uber.vert", "uber.frag", DiffuseAmbientCubeSkinnedMaterial::shaderDefs); // returns existing if already loaded

		std::unique_ptr<DiffuseAmbientCubeSkinnedMaterial> m = std::make_unique<DiffuseAmbientCubeSkinnedMaterial>(name, diffuseAmbientCubeSkinnedMaterialShader);

		Material* pMaterial = this->assetManager->addMaterial(std::move(m));
		this->materialsInUse.push_back(pMaterial);

		return static_cast<DiffuseAmbientCubeSkinnedMaterial*>(pMaterial);
	}

	RGBAMaterial* Scene::addRGBAMaterial(const std::string& name)
	{
		Shader* RGBAMaterialShader = this->assetManager->loadShader("RGBAMaterialShader",
			"uber.vert", "uber.frag", RGBAMaterial::shaderDefs); // returns existing if already loaded

		std::unique_ptr<RGBAMaterial> m = std::make_unique<RGBAMaterial>(name, RGBAMaterialShader);

		Material* pMaterial = this->assetManager->addMaterial(std::move(m));
		this->materialsInUse.push_back(pMaterial);

		return static_cast<RGBAMaterial*>(pMaterial);
	}

	RGBALightmapMaterial* Scene::addRGBALightmapMaterial(const std::string& name)
	{
		Shader* RGBALightmapMaterialShader = this->assetManager->loadShader("RGBALightmapMaterialShader",
			"uber.vert", "uber.frag", RGBALightmapMaterial::shaderDefs); // returns existing if already loaded

		std::unique_ptr<RGBALightmapMaterial> m = std::make_unique<RGBALightmapMaterial>(name, RGBALightmapMaterialShader);

		Material* pMaterial = this->assetManager->addMaterial(std::move(m));
		this->materialsInUse.push_back(pMaterial);

		return static_cast<RGBALightmapMaterial*>(pMaterial);
	}

	DiffuseCausticMaterial* Scene::addDiffuseCausticMaterial(const std::string& name)
	{
		Shader* diffuseCausticMaterialShader = this->assetManager->loadShader("diffuseCausticMaterialShader",
			"uber.vert", "uber.frag", DiffuseCausticMaterial::shaderDefs); // returns existing if already loaded

		std::unique_ptr<DiffuseCausticMaterial> m = std::make_unique<DiffuseCausticMaterial>(name, diffuseCausticMaterialShader);

		Material* pMaterial = this->assetManager->addMaterial(std::move(m));
		this->materialsInUse.push_back(pMaterial);

		return static_cast<DiffuseCausticMaterial*>(pMaterial);
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
		Material* taMaterial = this->addTextMaterial(name + "_material");
		taMaterial->addTexture(&fb->texture);
		taMaterial->setColor(color);

		// create actor
		Actor* pTextActor = stage->addActor(name);
		
		pTextActor->setDynamic(false);
		pTextActor->setVisible(true);
		pTextActor->setMesh(pTam);
		pTextActor->setMaterial(taMaterial);
		
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


				for (auto& shaderBucket : s->getActors())
				{
					for (auto& vaoBucket : shaderBucket.second)
					{
						for (unsigned int i = 0; i < vaoBucket.second.size(); i++)
						{
							Actor* a = vaoBucket.second.at(i).get();

							if (!a->getMesh() || !a->isVisible() || !a->getMaterial()->getShader())
								continue;

							if (actorsFirstPass)
								a->getMaterial()->preDraw(frameTime);

							// Pool transparents/translucents for render after opaques
							if (a->getMaterial()->getHasAlphaChannel() || a->getMaterial()->getColor().w < 1.0f)
							{
								float dist = glm::length(this->cameraPosition - a->getTransform().getTranslation());
								this->transparentActors.push_back(std::pair<float, Actor*>(dist, a));
								continue;
							}

							// Draw Opaque

							gpu->useShader(a->getMaterial()->getShader()); // only alters gpu state if necessary
							gpu->useMesh(a->getMesh()); // only alters gpu state if necessary
							gpu->setActiveMaterial(a->getMaterial());
							
							a->getMaterial()->draw(alpha, gpu, a, this->cameraViewMatrix, this->cameraProjectionMatrix);
						}
					}
				}

				actorsFirstPass = false;

				// Draw transparents/translucents
				gpu->enableBlend2();
				//gpu->disableDepthMask();

				// not proud of this, but it gets the job done for the time being, loop through all transparent actors and sort by their distance
				// from the current camera position
				std::sort(transparentActors.begin(), transparentActors.end(), [](auto &left, auto &right) {
					return left.first < right.first;
				});

				// Draw all transparent/translucent actors
				for (std::vector<std::pair<float, Actor*>>::reverse_iterator it = transparentActors.rbegin(); it != transparentActors.rend(); ++it)
				{
					// update gpu states, inefficient to do this for every actor, but transparents should be limited, and other solutions require
					// re-architecting the renderer, adding more complexity, and making this unusable for probably another entire year...so no thank you
					
					auto a = it->second;

					gpu->useShader(a->getMaterial()->getShader()); // only alters gpu state if necessary
					gpu->useMesh(a->getMesh()); // only alters gpu state if necessary
					gpu->setActiveMaterial(a->getMaterial());

					a->getMaterial()->draw(alpha, gpu, a, this->cameraViewMatrix, this->cameraProjectionMatrix);
				}

				//gpu->enableDepthMask();

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