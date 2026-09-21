#pragma once

#include <memory>
#include <vector>
#include <string>
#include <optional>

#include <ozz/animation/runtime/skeleton.h>
#include <ozz/animation//runtime/animation.h>

#include <vel/InputState.h>
#include <vel/Util/slot_map.h>

#include <vel/Scene/BufferIds.h>
#include <vel/Scene/Actor/Actor.h>
#include <vel/Scene/MeshLoader/MeshLoaderInterface.h>
#include <vel/Scene/GeoPool/GeoPool.h>
#include <vel/Scene/Animation/SkelAnimator.h>
#include <vel/Scene/Camera/Camera.h>
#include <vel/Scene/FinalRenderTarget.h>
#include <vel/Scene/CollisionWorld/CollisionWorld.h>
#include <vel/Scene/CollisionWorld/CollisionDebugDrawer.h>
#include <vel/Scene/Shader.h>
#include <vel/Scene/Texture.h>
#include <vel/Scene/Material.h>
#include <vel/Scene/MaterialGpuData.h>
#include <vel/Scene/Font/FontBitmap.h>
#include <vel/Scene/Font/FontGlyphInfo.h>
#include <vel/Scene/Text.h>
#include <vel/Scene/Mesh/PlaneOrigin.h>
#include <vel/Scene/ActorGpuData.h>



namespace vel
{
	////////////////////////////////////////////////////////////////////////////////////////////////
	// HeadlessScene
	////////////////////////////////////////////////////////////////////////////////////////////////
	class HeadlessScene
	{
	private:
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
		virtual void		internalFixedLoop(float deltaTime);
		virtual bool		load() = 0;
		virtual void		fixedLoop(float deltaTime) = 0;


	////////////////////////////////////////////////////////////////////////////////////////////////
	// SceneActor
	////////////////////////////////////////////////////////////////////////////////////////////////
	protected:
		actor_handle		addActor(Mesh* mesh);
		void				setActorParent(actor_handle child, actor_handle parent);
		void				clearActorParent(actor_handle child);
		void				setActorParentBone(actor_handle child, actor_handle parent, int32_t parentBoneId);
		void				clearActorParentBone(actor_handle child);
		glm::mat4			getActorWorldMatrix(actor_handle h);


	////////////////////////////////////////////////////////////////////////////////////////////////
	// SceneMesh
	////////////////////////////////////////////////////////////////////////////////////////////////
	protected:
		virtual std::vector<Mesh*>				loadMesh(const std::string& path, uint32_t meshFlags = MESHFLAG_NONE);
		virtual Mesh*							addMesh(std::unique_ptr<Mesh> m); // This method assumes that the caller understands no duplication checks are occuring
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
	private:
		int																audioGroupKey;
		BufferIds														bufferIds;
	protected:
		FinalRenderTarget												sceneRenderTarget;
		std::vector<std::unique_ptr<Stage>> 							stages;
		std::vector<std::unique_ptr<Camera>>							cameras;

		std::unordered_map<std::string, texture_handle>					textureHandleMap;
		std::vector<Texture>											textures;

		std::unordered_map<unsigned int, shader_handle>					shaderHandleMap;
		std::vector<Shader>												shaders;

		std::vector<Material>											materials; // contiguous array of every material, added during load(), NOT modified at runtime
		std::vector<MaterialGpuData>									materialsGpu; // contiguous array of every material's gpu representation, generated once during load
		std::vector<uint64_t>											materialTexturesGpu; // contiguous array of every texture in every material, generated once during load
		
		std::vector<ActorGpuData> 										actorsGpu;
		std::vector<glm::vec4> 											actorAmbientCube;
		std::vector<glm::mat4> 											actorBoneMatrices;
		
		std::unordered_map<std::string, FontBitmap>						fontBitmaps;
		slot_map<Text>													texts;

		std::unordered_map<VtxLayout, std::unique_ptr<GeoPool>>			renderGeoPools;
		std::unordered_map<std::string, std::unique_ptr<GeoPool>>		renderSoloGeoPools;

		std::vector<std::string>										soundsInUse;
	
	private:
		void			initMaterialData();
	public:
		Scene();
		~Scene();
		virtual void	immediateLoop(float frameTime, float renderLerpInterval) = 0;
		virtual void	internalImmediateLoop(float frameTime, float renderLerpInterval);
		bool			internalLoad() override;
		void			draw(float frameTime, float alpha);
		
	
	////////////////////////////////////////////////////////////////////////////////////////////////
	// SceneStage
	////////////////////////////////////////////////////////////////////////////////////////////////
	protected:
		Stage*						addStage(int pos = -1);

	////////////////////////////////////////////////////////////////////////////////////////////////
	// SceneActor
	////////////////////////////////////////////////////////////////////////////////////////////////
	private:
		DrawBucketLocation			findOrCreateDrawBucket(Stage* stage, RenderPass pass, uint32_t shader, uint32_t vao);
	protected:
		actor_handle				addActor(Stage* stage, Mesh* mesh, std::vector<material_handle> materials, uint32_t flags);
		actor_handle				addActor(Stage* stage, Mesh* mesh, SkelAnimator* animator, std::vector<material_handle> materials, uint32_t flags);
		void						hideActor(actor_handle h);
		void						showActor(actor_handle h);
		glm::mat4					getActorWorldRenderMatrix(actor_handle h, float alpha);

	////////////////////////////////////////////////////////////////////////////////////////////////
	// SceneText
	////////////////////////////////////////////////////////////////////////////////////////////////
	private:
		FontGlyphInfo				getFontGlyphInfo(uint32_t character, float offsetX, float offsetY, FontBitmap* fb);
		void						buildTextGeometry(Text& ta, Mesh* mesh);
		float						measureFontHeight(const std::string& text, FontBitmap* fb);
		FontBitmap*					loadFontBitmapRaw(const std::string& fontName, int stbFontSize, const std::string& fontPath);
		std::unique_ptr<Mesh>		loadTextMesh(Text& ta);
		void						removeFontBitmap(FontBitmap* pFontBitmap);
	protected:
		FontBitmap*					loadFontBitmap(const std::string& fontName, int fontSize);
		FontBitmap*					loadFontBitmapVisualHeight(const std::string& fontName, int desiredVisiblePx); // for ui elements where you would expect that font size is in pixels
		text_handle					addText(Stage* stage, const std::string& font, int fontSize, glm::vec4 color, const std::string& theText, PlaneOrigin originType = PlaneOrigin::LEFT_BOTTOM);
	public:
		void						updateTexts();
	
	////////////////////////////////////////////////////////////////////////////////////////////////
	// SceneTexture
	////////////////////////////////////////////////////////////////////////////////////////////////
	private:
		void						generateTextureData(const std::string& path, Texture& t);
	protected:
		texture_handle				loadTexture(const std::string& path, uint32_t flags = 0);
		std::vector<texture_handle>	loadTextureFrames(const std::string& dir, uint32_t flags = 0);
	
	////////////////////////////////////////////////////////////////////////////////////////////////
	// SceneShader
	////////////////////////////////////////////////////////////////////////////////////////////////
	private:
		shader_handle				generateLineShader(uint32_t flags);
		void						generateVertexShader(uint32_t flags, std::string& code);
		void						generateFragmentShader(uint32_t flags, std::string& code);
		shader_handle				generateShader(uint32_t flags);
	protected:
		unsigned int				getShaderProgramId(uint32_t flags);

	////////////////////////////////////////////////////////////////////////////////////////////////
	// SceneMaterial
	////////////////////////////////////////////////////////////////////////////////////////////////
	protected:
		material_handle				addMaterial(uint32_t flags);
		
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
		Actor*						addLine(const std::vector<std::tuple<glm::vec2, glm::vec2, unsigned int>>& points, std::vector<glm::vec4> colors, float thickness = 1.f);
		Actor*						addContinuousLine(const std::vector<glm::vec2>& points, glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), float thickness = 1.f);
		std::unique_ptr<Mesh>		linePointsToMesh(const std::vector<glm::vec2>& points);
		std::unique_ptr<Mesh>		lineSegmentsToMesh(const std::vector<std::tuple<glm::vec2, glm::vec2, unsigned int>>& points);
		
	////////////////////////////////////////////////////////////////////////////////////////////////
	// SceneMesh
	////////////////////////////////////////////////////////////////////////////////////////////////
	protected:
		std::vector<Mesh*>			loadMesh(const std::string& path, uint32_t meshFlags = MESHFLAG_POOLED | MESHFLAG_RENDERABLE) override;
		Mesh*						loadBillboardMesh(const std::string& name, float width, float height);
		Mesh*						addMesh(std::unique_ptr<Mesh> m) override; // This method assumes that the caller understands no duplication checks are occuring
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
		Camera*						addCamera(CameraType type);
		Camera*						getCamera(unsigned int id);
	public:
		void						updateAllCameraResolutions(int x, int y);
		void						clearAllRenderTargetBuffers();
		texture_handle				createCameraTexture(Camera* c);

	}; // END SCENE CLASS

} // END NAMESPACE