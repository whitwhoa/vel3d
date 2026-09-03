#pragma once

#include <memory>
#include <vector>
#include <string>
#include <optional>

#include <ozz/animation/runtime/skeleton.h>
#include <ozz/animation//runtime/animation.h>

#include <vel/Scene/Actor/Actor.h>
#include <vel/Scene/Mesh/MeshLoaderInterface.h>
#include <vel/Scene/Mesh/MeshFlag.h>
#include <vel/Scene/GeoPool/GeoPool.h>
#include <vel/Scene/Animation/SkelAnimator.h>
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
#include <vel/Scene/Text.h>
#include <vel/Scene/Line.h>
#include <vel/Scene/Billboard.h>
#include <vel/Scene/Mesh/PlaneOrigin.h>
#include <vel/Util/slot_map.h>


namespace vel
{
	////////////////////////////////////////////////////////////////////////////////////////////////
	// HeadlessScene
	////////////////////////////////////////////////////////////////////////////////////////////////
	class HeadlessScene
	{
	private:
		static unsigned int 														nextSceneId;
		unsigned int																id;
	protected:
		std::unique_ptr<MeshLoaderInterface>										meshLoader;
		std::vector<CollisionWorld*> 												collisionWorlds;
		std::unordered_map<std::string, std::unique_ptr<GeoPool>>					soloGeoPools;
		std::unordered_map<std::string, std::unique_ptr<Mesh>>						meshes;
		std::unordered_map<std::string, std::unique_ptr<ozz::animation::Skeleton>>	skeletons;
		std::unordered_map<std::string, std::unique_ptr<ozz::animation::Animation>>	animations;
		std::vector<std::unique_ptr<SkelAnimator>>									animators;
		slot_map<Actor>																actors;
	public:
		HeadlessScene();
		~HeadlessScene();
		unsigned int		getId() const;
		virtual bool		internalLoad();
		virtual bool		load() = 0;
		virtual void		internalFixedLoop(float deltaTime);
		virtual void		fixedLoop(float deltaTime) = 0;

	////////////////////////////////////////////////////////////////////////////////////////////////
	// SceneMeshes
	////////////////////////////////////////////////////////////////////////////////////////////////
	protected:
		virtual std::vector<Mesh*>				loadMesh(const std::string& path, uint32_t meshFlags = MESHFLAG_NONE);
		virtual Mesh*							addMesh(std::unique_ptr<Mesh> m);
		Mesh*									getMesh(const std::string& name);
		virtual void							removeMesh(Mesh* pMesh);

	////////////////////////////////////////////////////////////////////////////////////////////////
	// SceneSkeletalAnimation
	////////////////////////////////////////////////////////////////////////////////////////////////
	protected:
		ozz::animation::Skeleton*				loadSkeleton(const std::string& name, const std::string& path);
		ozz::animation::Skeleton*				getSkeleton(const std::string& name);
		void									removeSkeleton(const std::string& name);
		ozz::animation::Animation*				loadAnimation(const std::string& name, const std::string& path);
		ozz::animation::Animation*				getAnimation(const std::string& name);
		void									removeAnimation(const std::string& name);
	public:
		void									updateAnimators(float delta);
		
	////////////////////////////////////////////////////////////////////////////////////////////////
	// ScenePhysics
	////////////////////////////////////////////////////////////////////////////////////////////////
	protected:
		CollisionWorld*							addCollisionWorld(const std::string& name, float gravity = -10.0f);
		CollisionWorld*							getCollisionWorld(const std::string& name);
		int										getCollisionWorldIndex(const std::string& name);
	public:
		void									stepPhysics(float delta);
	};

	////////////////////////////////////////////////////////////////////////////////////////////////
	// Scene
	////////////////////////////////////////////////////////////////////////////////////////////////
	class Scene : public HeadlessScene
	{
	protected:
		std::unique_ptr<FinalRenderTarget>								sceneRenderTarget;
		std::vector<unique_ptr<Stage>> 									stages;
		std::vector<std::unique_ptr<Camera>>							cameras;
		std::unordered_map<std::string, std::unique_ptr<Shader>>		shaders;
		std::unordered_map<std::string, std::unique_ptr<Texture>>		textures;
		std::unordered_map<std::string, std::unique_ptr<Material>>		materials;
		std::unordered_map<std::string, std::unique_ptr<FontBitmap>>	fontBitmaps;
		std::vector<std::string>										soundsInUse;
		std::unordered_map<VtxLayout, std::unique_ptr<GeoPool>>			renderGeoPools;
		std::unordered_map<std::string, std::unique_ptr<GeoPool>>		renderSoloGeoPools;
	public:
		int																audioGroupKey;
		Scene();
		~Scene();
		virtual void	internalImmediateLoop(float frameTime, float renderLerpInterval);
		virtual void	immediateLoop(float frameTime, float renderLerpInterval) = 0;
		bool			internalLoad() override;
		void			draw(float frameTime, float alpha);
	
	////////////////////////////////////////////////////////////////////////////////////////////////
	// SceneStage
	////////////////////////////////////////////////////////////////////////////////////////////////
	protected:
		Stage*						addStage(int pos = -1);
	
	////////////////////////////////////////////////////////////////////////////////////////////////
	// SceneText
	////////////////////////////////////////////////////////////////////////////////////////////////
	private:
		FontGlyphInfo				getFontGlyphInfo(uint32_t character, float offsetX, float offsetY, FontBitmap* fb);
		void						buildTextGeometry(Text* ta, Mesh* mesh);
		float						measureFontHeight(const std::string& text, FontBitmap* fb);
		FontBitmap*					loadFontBitmapRaw(const std::string& fontName, int stbFontSize, const std::string& fontPath);
		std::unique_ptr<Mesh>		loadTextMesh(Text* ta);
	protected:
		FontBitmap*					loadFontBitmap(const std::string& fontName, int fontSize, const std::string& fontPath);
		FontBitmap*					loadFontBitmapVisualHeight(const std::string& fontName, int desiredVisiblePx, const std::string& fontPath);
		FontBitmap*					getFontBitmap(const std::string& name);
		void						removeFontBitmap(FontBitmap* pFontBitmap);
		Text*						addText(Stage* stage, const std::string& name, const std::string& theText, FontBitmap* fb, glm::vec4 color, PlaneOrigin originType = PlaneOrigin::LEFT_BOTTOM);
	public:
		void						updateText(Text* t);
		void						updateTexts();
	
	////////////////////////////////////////////////////////////////////////////////////////////////
	// SceneTexture
	////////////////////////////////////////////////////////////////////////////////////////////////
	private:
		std::optional<TextureData>	generateTextureData(const std::string& path);
	protected:
		Texture*					loadTexture(const std::string& name, const std::string& path, int options = 0);
		Texture*					getTexture(const std::string& name);
		void						removeTexture(Texture* pTexture);
		
	////////////////////////////////////////////////////////////////////////////////////////////////
	// SceneMaterial
	////////////////////////////////////////////////////////////////////////////////////////////////
	protected:
		Material*					addMaterial(std::unique_ptr<Material> m);
		Material*					getMaterial(const std::string& name);
		void						removeMaterial(Material* pMaterial);
		
	////////////////////////////////////////////////////////////////////////////////////////////////
	// SceneSound
	////////////////////////////////////////////////////////////////////////////////////////////////
	protected:
		void						loadBGMSound(const std::string& path);
		bool						loadSFXSound(const std::string& path);
	public:
		int							getAudioGroupKey() const;
		
	////////////////////////////////////////////////////////////////////////////////////////////////
	// SceneLine
	////////////////////////////////////////////////////////////////////////////////////////////////
	protected:
		Line*						addLine(Stage* stage, const std::string& name, const std::vector<std::tuple<glm::vec2, glm::vec2, unsigned int>>& points, std::vector<glm::vec4> colors);
		Line*						addContinuousLine(Stage* stage, const std::string& name, const std::vector<glm::vec2>& points, glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
		
	////////////////////////////////////////////////////////////////////////////////////////////////
	// SceneBillboard
	////////////////////////////////////////////////////////////////////////////////////////////////
	private:
		std::unique_ptr<Mesh>		loadBillboardMesh(std::string name, float width, float height);
	protected:
		Billboard*					addBillboard(Stage* stage, const std::string& name, Material* material, Camera* parentCamera, float width = 1.0f, float height = 1.0f);
		Billboard*					addBillboard(Stage* stage, const std::string& name, Material* material, Camera* parentCamera, Mesh* mesh);
	public:
		void						updateBillboards();
		
	////////////////////////////////////////////////////////////////////////////////////////////////
	// SceneMesh
	////////////////////////////////////////////////////////////////////////////////////////////////
	protected:
		std::vector<Mesh*>			loadMesh(const std::string& path, uint32_t meshFlags = MESHFLAG_POOLED | MESHFLAG_RENDERABLE) override;
		Mesh*						addMesh(std::unique_ptr<Mesh> m) override;
		void						removeMesh(Mesh* pMesh) override;
	
	////////////////////////////////////////////////////////////////////////////////////////////////
	// SceneSkeletalAnimation
	////////////////////////////////////////////////////////////////////////////////////////////////
	public:
		void						lerpAnimators(float alpha);
	
	////////////////////////////////////////////////////////////////////////////////////////////////
	// SceneCamera
	////////////////////////////////////////////////////////////////////////////////////////////////
	protected:
		Camera*						addCamera(const std::string& name, CameraType type);
		Camera*						getCamera(const std::string& name);
	public:
		void						updateAllCameraResolutions(int x, int y);
		void						clearAllRenderTargetBuffers();

	
		
		
		
	// UNSORTED BELOW THIS LINE
		
		
		void						setShaderOpts(int opts, std::vector<std::string>& defs, std::string& shaderName);
		std::optional<std::string>	loadShaderFile(const std::string& shaderPath);
		std::string					getTopShaderLines(const std::string& shaderCode, int numLinesToGet);
		std::string					getBottomShaderLines(const std::string& shaderCode, int numLinesToSkip);
		Shader*						loadShader(const std::string& name, const std::string& vertFile, const std::string& geomFile, const std::string& fragFile, std::vector<std::string> defs = {});
		Shader*						getShader(const std::string& name);
		void						removeShader(Shader* pShader);		
		

	};

}