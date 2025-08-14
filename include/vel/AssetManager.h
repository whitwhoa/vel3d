#pragma once

#include <string>
#include <unordered_map>

#include "sac.h"

#include "vel/Shader.h"
#include "vel/Mesh.h"
#include "vel/Texture.h"
#include "vel/Material.h"
#include "vel/MaterialAnimator.h"
#include "vel/Animation.h"
#include "vel/Armature.h"
#include "vel/FontBitmap.h"
#include "vel/FontGlyphInfo.h"
#include "vel/GPU.h"
#include "vel/TextActor.h"
#include "vel/MeshLoaderInterface.h"

#include "vel/AssetTrackers.h"

namespace vel
{
	//class GPU;

	class AssetManager
	{
	private:
		std::string													dataDir;
		std::unique_ptr<MeshLoaderInterface>						meshLoader;
		GPU*														gpu;
		


		std::vector<std::pair<std::unique_ptr<Shader>, int>>		shaders;
		std::vector<std::pair<std::unique_ptr<Mesh>, int>>			meshes;
		std::vector<std::pair<std::unique_ptr<Armature>, int>>		armatures;
		std::vector<std::pair<std::unique_ptr<Texture>, int>>		textures;
		std::vector<std::pair<std::unique_ptr<Material>, int>>		materials;
		std::vector<std::pair<std::unique_ptr<FontBitmap>, int>>	fontBitmaps;
		std::vector<std::pair<std::unique_ptr<Camera>, int>>		cameras;


		int													getShaderIndex(const std::string& name);
		int													getShaderIndex(const Shader* s);

		int													getMeshIndex(const std::string& name);
		int													getMeshIndex(const Mesh* m);

		int													getArmatureIndex(const std::string& name);
		int													getArmatureIndex(const Armature* a);

		int													getTextureIndex(const std::string& name);
		int													getTextureIndex(const Texture* t);
		TextureData											generateTextureData(const std::string& path);

		int													getMaterialIndex(const std::string& name);
		int													getMaterialIndex(const Material* m);

		FontGlyphInfo										getFontGlyphInfo(uint32_t character, float offsetX, float offsetY, FontBitmap* fb);
		int													getFontBitmapIndex(const std::string& name);
		int													getFontBitmapIndex(const FontBitmap* m);

		int													getCameraIndex(const std::string& name);
		int													getCameraIndex(const Camera* m);
		

	public:
		AssetManager(const std::string& dataDir, std::unique_ptr<MeshLoaderInterface> ml, GPU* gpu);
		~AssetManager();

		std::string					loadShaderFile(const std::string& shaderPath);
		std::string					getTopShaderLines(const std::string& shaderCode, int numLinesToGet);
		std::string					getBottomShaderLines(const std::string& shaderCode, int numLinesToSkip);
		Shader*						loadShader(const std::string& name, const std::string& vertFile, const std::string& geomFile, const std::string& fragFile, std::vector<std::string> defs = {});
		Shader*						getShader(const std::string& name);
		void						removeShader(const Shader* pShader);

		std::pair<std::vector<Mesh*>, Armature*> loadMesh(const std::string& path);
		Mesh*						addMesh(std::unique_ptr<Mesh> m);
		Mesh*						getMesh(const std::string& name);
		void						updateMesh(Mesh* m);
		void						removeMesh(const Mesh* pMesh);
		void						incrementMeshUsage(const Mesh* pMesh);

		Armature*					getArmature(const std::string& name);
		void						removeArmature(const Armature* pArm);

		Texture*					loadTexture(const std::string& name, const std::string& path, bool freeAfterGPULoad = true, unsigned int uvWrapping = 1);
		Texture*					getTexture(const std::string& name);
		void						removeTexture(const Texture* pTexture);

		Material*					addMaterial(std::unique_ptr<Material> m);
		Material*					getMaterial(const std::string& name);
		void						removeMaterial(const Material* pMaterial);

		FontBitmap*					loadFontBitmap(const std::string& fontName, int fontSize, const std::string& fontPath);
		FontBitmap*					getFontBitmap(const std::string& name);
		void						removeFontBitmap(const FontBitmap* pFontBitmap);

		Camera*						addCamera(std::unique_ptr<Camera> c);
		Camera*						getCamera(const std::string& name);
		void						removeCamera(const Camera* pCamera);
		

		std::unique_ptr<Mesh>		loadTextActorMesh(const TextActor* ta);
		

	};

}