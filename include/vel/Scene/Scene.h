#pragma once

#include <memory>
#include <vector>
#include <string>
#include <optional>

#include <vel/InputState.h>
#include <vel/Render/Camera.h>
#include <vel/Render/FinalRenderTarget.h>
#include <vel/Physics/CollisionWorld.h>
#include <vel/Physics/CollisionDebugDrawer.h>
#include <vel/Scene/HeadlessScene.h>
#include <vel/Scene/Stage/Actor/LineActor.h>
#include <vel/Scene/Stage/Actor/Billboard.h>
#include <vel/Scene/Stage/Actor/Mesh/Mesh.h>
#include <vel/Scene/Stage/Actor/Font/FontBitmap.h>
#include <vel/Scene/Stage/Actor/Font/FontGlyphInfo.h>
#include <vel/Scene/Stage/Actor/Material/DiffuseMaterial.h>
#include <vel/Scene/Stage/Actor/Material/DiffuseLightmapMaterial.h>
#include <vel/Scene/Stage/Actor/Material/DiffuseAnimatedMaterial.h>
#include <vel/Scene/Stage/Actor/Material/DiffuseAnimatedLightmapMaterial.h>
#include <vel/Scene/Stage/Actor/Material/DiffuseSkinnedMaterial.h>
#include <vel/Scene/Stage/Actor/Material/AlphaMaskMaterial.h>
#include <vel/Scene/Stage/Actor/Material/TextMaterial.h>
#include <vel/Scene/Stage/Actor/Material/DiffuseAmbientCubeMaterial.h>
#include <vel/Scene/Stage/Actor/Material/DiffuseAmbientCubeSkinnedMaterial.h>
#include <vel/Scene/Stage/Actor/Material/RGBAMaterial.h>
#include <vel/Scene/Stage/Actor/Material/RGBALineMaterial.h>
#include <vel/Scene/Stage/Actor/Material/RGBALightmapMaterial.h>
#include <vel/Scene/Stage/Actor/Material/DiffuseCausticMaterial.h>
#include <vel/Scene/Stage/Actor/Material/DiffuseCausticLightmapMaterial.h>
#include <vel/Scene/Stage/Actor/Material/DiffuseSingleSelectableMaterial.h>
#include <vel/Scene/Stage/Actor/Material/AnimatedBillboardMaterial.h>


namespace vel
{
	class Scene : public HeadlessScene
	{
	private:
		std::vector<std::unique_ptr<Camera>>	cameras; // TODO: why ptr?
		std::unique_ptr<FinalRenderTarget>		sceneRenderTarget; // TODO: why ptr?

		float								animationTime;
		glm::vec4							screenTint;

		std::unordered_map<std::string, std::unique_ptr<Shader>>		shaders;
		std::unordered_map<std::string, std::unique_ptr<Texture>>		textures;
		std::unordered_map<std::string, std::unique_ptr<Material>>		materials;
		std::unordered_map<std::string, std::unique_ptr<FontBitmap>>	fontBitmaps;
		std::vector<std::string>			soundsInUse;
		
		double								frameTime;
		double								frameRate;

		void								setShaderOpts(int opts, std::vector<std::string>& defs, std::string& shaderName);

		std::optional<TextureData>			generateTextureData(const std::string& path);
		FontGlyphInfo						getFontGlyphInfo(uint32_t character, float offsetX, float offsetY, FontBitmap* fb);
		float								measureFontHeight(const std::string& text, FontBitmap* fb);

		
	protected:
		int									audioGroupKey;

		void								loadBGMSound(const std::string& path);
		bool								loadSFXSound(const std::string& path);

		Camera*								addCamera(const std::string& name, CameraType type);

		DiffuseMaterial*					addDiffuseMaterial(const std::string& name, int opts = 0);
		DiffuseLightmapMaterial*			addDiffuseLightmapMaterial(const std::string& name, int opts = 0);
		DiffuseAnimatedMaterial*			addDiffuseAnimatedMaterial(const std::string& name, int opts = 0);
		DiffuseAnimatedLightmapMaterial*	addDiffuseAnimatedLightmapMaterial(const std::string& name, int opts = 0);
		DiffuseSkinnedMaterial*				addDiffuseSkinnedMaterial(const std::string& name, int opts = 0);
		AlphaMaskMaterial*					addAlphaMaskMaterial(const std::string& name);
		TextMaterial*						addTextMaterial(const std::string& name, int opts = 0);
		DiffuseAmbientCubeMaterial*			addDiffuseAmbientCubeMaterial(const std::string& name, int opts = 0);
		DiffuseAmbientCubeSkinnedMaterial*	addDiffuseAmbientCubeSkinnedMaterial(const std::string& name, int opts = 0);
		RGBAMaterial*						addRGBAMaterial(const std::string& name, int opts = 0);
		RGBALineMaterial*					addRGBALineMaterial(const std::string& name, int opts = 0);
		RGBALightmapMaterial*				addRGBALightmapMaterial(const std::string& name, int opts = 0);
		DiffuseCausticMaterial*				addDiffuseCausticMaterial(const std::string& name, int opts = 0);
		DiffuseCausticLightmapMaterial*		addDiffuseCausticLightmapMaterial(const std::string& name, int opts = 0);
		DiffuseSingleSelectableMaterial*	addDiffuseSingleSelectableMaterial(const std::string& name, int opts = 0);
		AnimatedBillboardMaterial*			addAnimatedBillboardMaterial(const std::string& name, int opts = 0);

		Camera*								getCamera(const std::string& name);

		TextActor* addTextActor(Stage* stage, const std::string& name, const std::string& theText, FontBitmap* fb,
			glm::vec4 color, PlaneOrigin originType = PlaneOrigin::LEFT_BOTTOM);

		LineActor* addLineActor(Stage* stage, const std::string& name, const std::vector<std::tuple<glm::vec2, glm::vec2, unsigned int>>& points,
			std::vector<glm::vec4> colors);

		LineActor* addContinuousLineActor(Stage* stage, const std::string& name, const std::vector<glm::vec2>& points,
			glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

		Billboard* addBillboard(Stage* stage, const std::string& name, Material* material, Camera* parentCamera,
			float width = 1.0f, float height = 1.0f);

		// version of addBillboard that accepts a pointer to an existing mesh instead of creating one (so that for example
		// if we have 100 enemies that all use the same size billboard, they can all use the same mesh, and we don't have to 
		// make 100 extra calls into the graphics driver to swap vaos)
		Billboard* addBillboard(Stage* stage, const std::string& name, Material* material, Camera* parentCamera, Mesh* mesh);


		std::optional<std::string>	loadShaderFile(const std::string& shaderPath);
		std::string					getTopShaderLines(const std::string& shaderCode, int numLinesToGet);
		std::string					getBottomShaderLines(const std::string& shaderCode, int numLinesToSkip);
		Shader*						loadShader(const std::string& name, const std::string& vertFile, const std::string& geomFile, const std::string& fragFile, std::vector<std::string> defs = {});
		Shader*						getShader(const std::string& name);
		void						removeShader(Shader* pShader);

		//std::vector<Mesh*>			loadMesh(const std::string& path, bool standaloneGeometry = false) override;
		Mesh*						addMesh(std::unique_ptr<Mesh> m, bool standaloneGeometry = false) override;
		void						updateRenderMesh(Mesh* m);
		void						removeMesh(Mesh* pMesh) override;
		std::unique_ptr<Mesh>		loadTextActorMesh(TextActor* ta);

		Texture*					loadTexture(const std::string& name, const std::string& path, int options = 0);
		Texture*					getTexture(const std::string& name);
		void						removeTexture(Texture* pTexture);

		Material*					addMaterial(std::unique_ptr<Material> m);
		Material*					getMaterial(const std::string& name);
		void						removeMaterial(Material* pMaterial);

		FontBitmap*					loadFontBitmap(const std::string& fontName, int fontSize, const std::string& fontPath); // alias to raw
		FontBitmap*					loadFontBitmapRaw(const std::string& fontName, int stbFontSize, const std::string& fontPath);
		FontBitmap*					loadFontBitmapVisualHeight(const std::string& fontName, int desiredVisiblePx, const std::string& fontPath);
		FontBitmap*					getFontBitmap(const std::string& name);
		void						removeFontBitmap(FontBitmap* pFontBitmap);

		





		

	public:
		Scene();
		~Scene();
		virtual void						internalImmediateLoop(float frameTime, float renderLerpInterval);
		virtual void						immediateLoop(float frameTime, float renderLerpInterval) = 0;
		bool								internalLoad() override;

		void								lerpAnimators(float alpha);
		void								draw(float frameTime, float alpha);

		void								clearAllRenderTargetBuffers();

		void								updateTextActor(TextActor* ta);
		void								updateTextActors();

		void								setScreenTint(glm::vec4 c);
		void								clearScreenTint();

		int									getAudioDeviceGroupKey();
		
		FinalRenderTarget*					getSceneRenderTarget();

		void								updateBillboards();

		void								updateAllCameraResolutions(int x, int y);

	};

}