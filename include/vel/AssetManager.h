#pragma once

#include <string>
#include <unordered_map>
#include <optional>

#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/skeleton.h>

#include <vel/Render/GPU.h>
//#include <vel/Render/Shader.h>
#include <vel/Scene/Stage/Actor/TextActor.h>
#include <vel/Scene/Stage/Actor/Mesh/Mesh.h>
//#include <vel/Scene/Stage/Actor/Mesh/MeshLoaderInterface.h>
#include <vel/Scene/Stage/Actor/Material/Material.h>
#include <vel/Scene/Stage/Actor/Material/MaterialAnimator.h>
#include <vel/Scene/Stage/Actor/Font/FontBitmap.h>
#include <vel/Scene/Stage/Actor/Font/FontGlyphInfo.h>


namespace vel
{
	//class GPU;

	class AssetManager
	{
	private:
		std::string													dataDir;
		//std::unique_ptr<MeshLoaderInterface>						meshLoader;
		GPU*														gpu;
		
		//std::vector<std::pair<std::unique_ptr<Mesh>, int>>			meshes;
		std::vector<std::pair<std::unique_ptr<Texture>, int>>		textures;
		std::vector<std::pair<std::unique_ptr<Material>, int>>		materials;
		std::vector<std::pair<std::unique_ptr<FontBitmap>, int>>	fontBitmaps;
		
		std::unordered_map<std::string, std::pair<std::unique_ptr<ozz::animation::Skeleton>, int>> skeletons;
		std::unordered_map<std::string, std::pair<std::unique_ptr<ozz::animation::Animation>, int>> animations;


		//int													getMeshIndex(const std::string& name);
		//int													getMeshIndex(const Mesh* m);

		int													getTextureIndex(const std::string& name);
		int													getTextureIndex(const Texture* t);
		std::optional<TextureData>							generateTextureData(const std::string& path);

		int													getMaterialIndex(const std::string& name);
		int													getMaterialIndex(const Material* m);

		FontGlyphInfo										getFontGlyphInfo(uint32_t character, float offsetX, float offsetY, FontBitmap* fb);
		int													getFontBitmapIndex(const std::string& name);
		int													getFontBitmapIndex(const FontBitmap* m);
		

	public:
		AssetManager(const std::string& dataDir, GPU* gpu = nullptr);
		~AssetManager();

		//std::vector<Mesh*>			loadMesh(const std::string& path);
		//Mesh*						addMesh(std::unique_ptr<Mesh> m);
		//Mesh*						getMesh(const std::string& name);
		//void						updateMesh(Mesh* m);
		//void						removeMesh(const Mesh* pMesh);
		//void						incrementMeshUsage(const Mesh* pMesh);

		Texture*					loadTexture(const std::string& name, const std::string& path, int options = 0);
		Texture*					getTexture(const std::string& name);
		void						removeTexture(const Texture* pTexture);

		Material*					addMaterial(std::unique_ptr<Material> m);
		Material*					getMaterial(const std::string& name);
		void						removeMaterial(const Material* pMaterial);
		int							getMaterialUsageCount(const std::string& name);

		float						measureFontHeight(const std::string& text, FontBitmap* fb);
		FontBitmap*					loadFontBitmap(const std::string& fontName, int fontSize, const std::string& fontPath); // alias to raw
		FontBitmap*					loadFontBitmapRaw(const std::string& fontName, int stbFontSize, const std::string& fontPath);
		FontBitmap*					loadFontBitmapVisualHeight(const std::string& fontName, int desiredVisiblePx, const std::string& fontPath);
		FontBitmap*					getFontBitmap(const std::string& name);
		void						removeFontBitmap(const FontBitmap* pFontBitmap);

		std::unique_ptr<Mesh>		loadTextActorMesh(TextActor* ta);

		ozz::animation::Skeleton*	loadSkeleton(const std::string& name, const std::string& path);
		ozz::animation::Skeleton*	getSkeleton(const std::string& name);
		void						removeSkeleton(const std::string& name);
		
		ozz::animation::Animation*	loadAnimation(const std::string& name, const std::string& path);
		ozz::animation::Animation*	getAnimation(const std::string& name);
		void						removeAnimation(const std::string& name);

	};

}