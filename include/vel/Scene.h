#pragma once

#include <memory>
#include <vector>
#include <string>
#include <optional>


#include "vel/GPU.h"
#include "vel/InputState.h"
#include "vel/Camera.h"
#include "vel/Mesh.h"
#include "vel/CollisionWorld.h"
#include "vel/CollisionDebugDrawer.h"
#include "vel/FontBitmap.h"
#include "vel/FontGlyphInfo.h"
#include "vel/AssetManager.h"
#include "vel/HeadlessScene.h"
#include "vel/LineActor.h"
#include "vel/AudioDevice.h"
#include "vel/FinalRenderTarget.h"
#include "vel/Billboard.h"

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
		GPU*								gpu;
		glm::ivec2							windowSize;
		glm::ivec2							resolution;

		std::vector<std::unique_ptr<Camera>>	cameras;
		std::unique_ptr<FinalRenderTarget>		sceneRenderTarget;

		std::vector<std::unique_ptr<TextActor>>	textActors;
		std::vector<std::unique_ptr<LineActor>>	lineActors;
		std::vector<std::unique_ptr<Billboard>>	billboards;

		float								animationTime;
		glm::vec4							screenTint;

		std::vector<Shader*>				shadersInUse;
		std::vector<Texture*> 				texturesInUse;
		std::vector<Material*> 				materialsInUse;
		std::vector<FontBitmap*> 			fontBitmapsInUse;
		std::vector<std::string>			soundsInUse;
		
		double								frameTime;
		double								frameRate;

		void								freeAssets();

		void								setShaderOpts(int opts, std::vector<std::string>& defs, std::string& shaderName);

		int									getTextActorIndex(const std::string& name);
		int									getTextActorIndex(const TextActor*);
		void								_removeTextActor(int textActorIndex);

		int									getLineActorIndex(const std::string& name);
		int									getLineActorIndex(const LineActor*);
		void								_removeLineActor(int lineActorIndex);

		int									getBillboardIndex(const std::string& name);
		int									getBillboardIndex(const Billboard* a);
		void								_removeBillboard(int billboardIndex);
		
	protected:
		const InputState*					inputState;
		AudioDevice*						audioDevice;
		int									audioGroupKey;
		Texture*							loadTexture(const std::string& name, const std::string& path, int options = 0);
		FontBitmap*							loadFontBitmap(const std::string& fontName, int fontSize, const std::string& fontPath);
		FontBitmap*							loadFontBitmapVisualHeight(const std::string& fontName, int desiredVisiblePx, const std::string& fontPath);

		void								loadBGMSound(const std::string& path);
		bool								loadSFXSound(const std::string& path);

		Camera*								addCamera(const std::string& name, CameraType type);

		void								addShaderInUse(Shader* s);
		void								addMaterialInUse(Material* m);

		DiffuseMaterial*					addDiffuseMaterial(const std::string& name, int opts = 0);
		DiffuseLightmapMaterial*			addDiffuseLightmapMaterial(const std::string& name, int opts = 0);
		DiffuseAnimatedMaterial*			addDiffuseAnimatedMaterial(const std::string& name, int opts = 0);
		DiffuseAnimatedLightmapMaterial*	addDiffuseAnimatedLightmapMaterial(const std::string& name, int opts = 0);
		DiffuseSkinnedMaterial*				addDiffuseSkinnedMaterial(const std::string& name, int opts = 0);
		TextMaterial*						addTextMaterial(const std::string& name, int opts = 0);
		DiffuseAmbientCubeMaterial*			addDiffuseAmbientCubeMaterial(const std::string& name, int opts = 0);
		DiffuseAmbientCubeSkinnedMaterial*	addDiffuseAmbientCubeSkinnedMaterial(const std::string& name, int opts = 0);
		RGBAMaterial*						addRGBAMaterial(const std::string& name, int opts = 0);
		RGBALineMaterial*					addRGBALineMaterial(const std::string& name, int opts = 0);
		RGBALightmapMaterial*				addRGBALightmapMaterial(const std::string& name, int opts = 0);
		DiffuseCausticMaterial*				addDiffuseCausticMaterial(const std::string& name, int opts = 0);
		DiffuseCausticLightmapMaterial*		addDiffuseCausticLightmapMaterial(const std::string& name, int opts = 0);


		TextActor*							addTextActor(const std::string& name, const std::string& theText, FontBitmap* fb, glm::vec4 color, TextActorOriginType originType = TextActorOriginType::LEFT_BOTTOM);
		TextActor*							getTextActor(const std::string& name);
		void								removeTextActor(TextActor*);
		void								removeTextActor(const std::string& name);


		LineActor*							addLineActor(Stage* stage, const std::string& name, const std::vector<std::tuple<glm::vec2, glm::vec2, unsigned int>>& points, std::vector<glm::vec4> colors);
		LineActor*							addContinuousLineActor(Stage* stage, const std::string& name, const std::vector<glm::vec2>& points, glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
		LineActor*							getLineActor(const std::string& name);
		void								removeLineActor(LineActor* la);
		void								removeLineActor(const std::string& name);
		



		Billboard*							addBillboard(const std::string& name, Material* material, Camera* parentCamera, float width = 1.0f, float height = 1.0f);
		// version of addBillboard that accepts a pointer to an existing mesh instead of creating one (so that for example
		// if we have 100 enemies that all use the same size billboard, they can all use the same mesh, and we don't have to 
		// make 100 extra calls into the graphics driver to swap vaos)
		Billboard*							addBillboard(const std::string& name, Material* material, Camera* parentCamera, Mesh* mesh);
		Billboard*							getBillboard(const std::string& name);
		void								removeBillboard(Billboard* b);
		void								removeBillboard(const std::string& name);



		



		Shader*								getShader(const std::string& name);
		Texture*							getTexture(const std::string& name);
		FontBitmap*							getFontBitmap(const std::string& name);
		Camera*								getCamera(const std::string& name);
		Material*							getMaterial(const std::string& name);

		
		

	public:
		Scene(const std::string& dataDir, GPU* gpu);
		~Scene();
		virtual bool						load() = 0;
		virtual void						fixedLoop(float deltaTime) = 0;
		virtual void						immediateLoop(float frameTime, float renderLerpInterval) = 0;

		glm::ivec2							getResolution();
		void								setResolution(int x, int y);

		glm::ivec2							getWindowSize();
		void								setWindowSize(int x, int y);

		void								setInputState(const InputState* is);

		void								lerpAnimators(float alpha);
		void								draw(float frameTime, float alpha);
		void								updatePreviousTransforms();

		void								clearAllRenderTargetBuffers(GPU* gpu);

		void								updateTextActors();

		void								setScreenTint(glm::vec4 c);
		void								clearScreenTint();

		std::vector<Camera*>&				getCamerasInUse();

		void								setAudioDevice(AudioDevice* ad);

		int									getAudioDeviceGroupKey();
		
		void								initRenderTarget();
		FinalRenderTarget*					getSceneRenderTarget();

		void								updateBillboards();

		void								setFrameTime(double ft);
		double								getFrameTime() const;
		void								setFrameRate(double fr);
		double								getFrameRate() const;

	};

}