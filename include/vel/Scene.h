#pragma once

#include <memory>
#include <vector>
#include <string>
#include <optional>

#include "vel/InputState.h"
#include "vel/Camera.h"
#include "vel/Mesh.h"
#include "vel/Stage.h"
#include "vel/Armature.h"
#include "vel/Animation.h"
#include "vel/AssetTrackers.h"
#include "vel/CollisionWorld.h"
#include "vel/CollisionDebugDrawer.h"
#include "vel/FontBitmap.h"
#include "vel/FontGlyphInfo.h"
#include "vel/AssetManager.h"
#include "vel/HeadlessScene.h"
#include "vel/LineActor.h"

#include "vel/Material.h"
#include "vel/DiffuseMaterial.h"
#include "vel/DiffuseLightmapMaterial.h"
#include "vel/DiffuseAnimatedMaterial.h"
#include "vel/DiffuseAnimatedLightmapMaterial.h"
#include "vel/DiffuseSkinnedMaterial.h"
#include "vel/TextMaterial.h"
#include "vel/DiffuseAmbientCubeMaterial.h"
#include "vel/DiffuseAmbientCubeSkinnedMaterial.h"
#include "vel/RGBAMaterial.h"
#include "vel/RGBALineMaterial.h"
#include "vel/RGBALightmapMaterial.h"
#include "vel/DiffuseCausticMaterial.h"
#include "vel/DiffuseCausticLightmapMaterial.h"

namespace vel
{
	class Scene : public HeadlessScene
	{
	private:
		glm::ivec2							windowSize;
		glm::ivec2							resolution;

		float								animationTime;
		glm::vec4							screenColor;

		std::vector<Shader*>				shadersInUse;
		std::vector<Texture*> 				texturesInUse;
		std::vector<Material*> 				materialsInUse;
		std::vector<FontBitmap*> 			fontBitmapsInUse;
		std::vector<Camera*>				camerasInUse;

		glm::vec3							cameraPosition;
		glm::mat4							cameraProjectionMatrix;
		glm::mat4							cameraViewMatrix;

		void								freeAssets();
		
	protected:
		const InputState*					inputState;
		Texture*							loadTexture(const std::string& name, const std::string& path, bool freeAfterGPULoad = true, unsigned int uvWrapping = 1);
		FontBitmap*							loadFontBitmap(const std::string& fontName, int fontSize, const std::string& fontPath);

		Camera*								addCamera(const std::string& name, CameraType type);

		DiffuseMaterial*					addDiffuseMaterial(const std::string& name, bool hasAlpha = false);
		DiffuseLightmapMaterial*			addDiffuseLightmapMaterial(const std::string& name, bool hasAlpha = false);
		DiffuseAnimatedMaterial*			addDiffuseAnimatedMaterial(const std::string& name, bool hasAlpha = false);
		DiffuseAnimatedLightmapMaterial*	addDiffuseAnimatedLightmapMaterial(const std::string& name, bool hasAlpha = false);
		DiffuseSkinnedMaterial*				addDiffuseSkinnedMaterial(const std::string& name, bool hasAlpha = false);
		TextMaterial*						addTextMaterial(const std::string& name, bool hasAlpha = false);
		DiffuseAmbientCubeMaterial*			addDiffuseAmbientCubeMaterial(const std::string& name, bool hasAlpha = false);
		DiffuseAmbientCubeSkinnedMaterial*	addDiffuseAmbientCubeSkinnedMaterial(const std::string& name, bool hasAlpha = false);
		RGBAMaterial*						addRGBAMaterial(const std::string& name, bool hasAlpha = false);
		RGBALineMaterial*					addRGBALineMaterial(const std::string& name, bool hasAlpha = false);
		RGBALightmapMaterial*				addRGBALightmapMaterial(const std::string& name, bool hasAlpha = false);
		DiffuseCausticMaterial*				addDiffuseCausticMaterial(const std::string& name, bool hasAlpha = false);
		DiffuseCausticLightmapMaterial*		addDiffuseCausticLightmapMaterial(const std::string& name, bool hasAlpha = false);


		TextActor*							addTextActor(Stage* stage, const std::string& name, const std::string& theText, FontBitmap* fb,
												TextActorAlignment alignment = TextActorAlignment::LEFT_ALIGN, glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

		LineActor*							addLineActor(Stage* stage, const std::string& name, std::vector<glm::vec2> points, 
												glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

		Shader*								getShader(const std::string& name);
		Texture*							getTexture(const std::string& name);
		FontBitmap*							getFontBitmap(const std::string& name);
		Camera*								getCamera(const std::string& name);
		Material*							getMaterial(const std::string& name);

	public:
		Scene();
		~Scene();
		virtual void						load() = 0;
		virtual void						fixedLoop(float deltaTime) = 0;
		virtual void						immediateLoop(float frameTime, float renderLerpInterval) = 0;
		virtual void						postPhysics(float deltaTime) {};

		glm::ivec2							getResolution();
		void								setResolution(int x, int y);

		glm::ivec2							getWindowSize();
		void								setWindowSize(int x, int y);

		void								setInputState(const InputState* is);

		void								updateAnimations(float frameTime);
		void								draw(GPU* gpu, float frameTime, float alpha);
		void								updatePreviousTransforms();

		void								clearAllRenderTargetBuffers(GPU* gpu);

		void								updateTextActors();

		void								setScreenColor(glm::vec4 c);
		void								clearScreenColor();

		std::vector<Camera*>&				getCamerasInUse();
		

	};

}