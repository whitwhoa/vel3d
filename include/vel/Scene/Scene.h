#pragma once

#include <memory>
#include <vector>
#include <string>
#include <optional>

#include <vel/InputState.h>
#include <vel/Scene/Camera/Camera.h>
#include <vel/Scene/FinalRenderTarget.h>
#include <vel/Scene/CollisionWorld/CollisionWorld.h>
#include <vel/Scene/CollisionWorld/CollisionDebugDrawer.h>
#include <vel/Scene/HeadlessScene.h>
#include <vel/Scene/Shader.h>
#include <vel/Scene/Texture/Texture.h>
#include <vel/Scene/Material.h>
#include <vel/Scene/Font/FontBitmap.h>
#include <vel/Scene/Font/FontGlyphInfo.h>
#include <vel/Scene/TextActor.h>
#include <vel/Scene/LineActor.h>
#include <vel/Scene/Billboard.h>
#include <vel/Scene/Mesh/PlaneOrigin.h>


namespace vel
{
	class Scene : public HeadlessScene
	{
	private:
		std::vector<std::unique_ptr<Camera>>	cameras; // TODO: why ptr?
		std::unique_ptr<FinalRenderTarget>		sceneRenderTarget; // TODO: why ptr?
		glm::vec4								screenTint; // TODO: rename?		

		std::unordered_map<std::string, std::unique_ptr<Shader>>		shaders;
		std::unordered_map<std::string, std::unique_ptr<Texture>>		textures;
		std::unordered_map<std::string, std::unique_ptr<Material>>		materials;
		std::unordered_map<std::string, std::unique_ptr<FontBitmap>>	fontBitmaps;
		std::vector<std::string> soundsInUse;

		std::unordered_map<VtxLayout, std::unique_ptr<GeoPool>>			renderGeoPools;
		std::unordered_map<std::string, std::unique_ptr<GeoPool>>		renderSoloGeoPools;
		
		double								frameTime; // TODO: unused?
		double								frameRate; // TODO: unused?

		void								setShaderOpts(int opts, std::vector<std::string>& defs, std::string& shaderName);

		std::optional<TextureData>			generateTextureData(const std::string& path);
		FontGlyphInfo						getFontGlyphInfo(uint32_t character, float offsetX, float offsetY, FontBitmap* fb);
		float								measureFontHeight(const std::string& text, FontBitmap* fb);
		void								buildTextActorGeometry(TextActor* ta, Mesh* mesh);

		
	protected:
		int									audioGroupKey;

		void								loadBGMSound(const std::string& path);
		bool								loadSFXSound(const std::string& path);

		Camera*								addCamera(const std::string& name, CameraType type);
		Camera*								getCamera(const std::string& name);

		TextActor* addTextActor(Stage* stage, const std::string& name, const std::string& theText, FontBitmap* fb, glm::vec4 color, PlaneOrigin originType = PlaneOrigin::LEFT_BOTTOM);
		LineActor* addLineActor(Stage* stage, const std::string& name, const std::vector<std::tuple<glm::vec2, glm::vec2, unsigned int>>& points, std::vector<glm::vec4> colors);
		LineActor* addContinuousLineActor(Stage* stage, const std::string& name, const std::vector<glm::vec2>& points, glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
		Billboard* addBillboard(Stage* stage, const std::string& name, Material* material, Camera* parentCamera, float width = 1.0f, float height = 1.0f);
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

		std::vector<Mesh*>			loadMesh(const std::string& path, uint32_t meshFlags = MESHFLAG_POOLED | MESHFLAG_RENDERABLE) override;
		Mesh*						addMesh(std::unique_ptr<Mesh> m) override;
		void						removeMesh(Mesh* pMesh) override;
		std::unique_ptr<Mesh>		loadTextActorMesh(TextActor* ta);
		std::unique_ptr<Mesh>		loadBillboardMesh(std::string name, float width, float height);

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