#include <thread> 
#include <chrono>
#include <sstream>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <memory>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_headers/stb_image.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_headers/stb_truetype.h"

#include "glad/gl.h"

#include "vel/AssetManager.h"
#include "vel/AssimpMeshLoader.h"
#include "vel/functions.h"
#include "vel/logger.hpp"

using namespace std::chrono_literals;

namespace vel
{

	AssetManager::AssetManager(const std::string& dataDir, std::unique_ptr<MeshLoaderInterface> ml, GPU* gpu) :
		dataDir(dataDir),
		meshLoader(std::move(ml)),
		gpu(gpu)
	{}
	AssetManager::~AssetManager(){}

	

	/***********************************************************************************************
	* SHADERS
	************************************************************************************************/
	int AssetManager::getShaderIndex(const std::string& name)
	{
		for (int i = 0; i < this->shaders.size(); i++)
			if (this->shaders.at(i).first->name == name)
				return i;

		return -1;
	}

	int AssetManager::getShaderIndex(const Shader* s)
	{
		for (int i = 0; i < this->shaders.size(); i++)
			if (this->shaders.at(i).first.get() == s)
				return i;

		return -1;
	}

	std::optional<std::string> AssetManager::loadShaderFile(const std::string& shaderPath)
	{
		std::ifstream shaderFile(shaderPath);

		if (!shaderFile.is_open()) 
		{
			VEL3D_LOG_DEBUG("AssetManager::loadShaderFile(): could not open shader file: {}", shaderPath);
			return std::nullopt;
		}

		std::stringstream shaderStream;
		shaderStream << shaderFile.rdbuf();
		shaderFile.close();

		if (shaderStream.str().empty()) 
		{
			VEL3D_LOG_DEBUG("AssetManager::loadShaderFile(): Shader file is empty: {}", shaderPath);
			return std::nullopt;
		}

		return shaderStream.str();
	}

	std::string AssetManager::getTopShaderLines(const std::string& shaderCode, int numLinesToGet)
	{
		std::istringstream shaderStream(shaderCode);
		std::string line;
		std::string firstLines;

		for (int i = 0; i < numLinesToGet; ++i) 
		{
			std::getline(shaderStream, line);
			firstLines += line + "\n";
		}

		return firstLines;
	}

	std::string AssetManager::getBottomShaderLines(const std::string& shaderCode, int numLinesToSkip)
	{
		std::istringstream shaderStream(shaderCode);
		std::string line;

		// Skip the specified number of lines
		for (int i = 0; i < numLinesToSkip; ++i) 
			std::getline(shaderStream, line);

		// Return the remaining shader code
		std::stringstream remainingShaderCode;
		remainingShaderCode << shaderStream.rdbuf();

		return remainingShaderCode.str();
	}

	Shader* AssetManager::loadShader(const std::string& name, const std::string& vertFile, const std::string& geomFile, 
		const std::string& fragFile, std::vector<std::string> defs)
	{
		int shaderIndex = this->getShaderIndex(name);

		if (shaderIndex > -1)
		{
			VEL3D_LOG_DEBUG("Existing Shader, bypass reload: {}", name);
			
			this->shaders.at(shaderIndex).second++;

			return this->shaders.at(shaderIndex).first.get();
		}

		VEL3D_LOG_DEBUG("Loading new Shader: {}", name);


		// Process vertex shader script
		std::optional<std::string> vcOpt = this->loadShaderFile(this->dataDir + "/shaders/" + vertFile);
		if (!vcOpt)
			return nullptr;

		std::string vertexCode = vcOpt.value();
		std::string topVertexLines = this->getTopShaderLines(vertexCode, 10);
		std::string bottomVertexLines = this->getBottomShaderLines(vertexCode, 10);
		std::stringstream preprocessedVertexCode;
		preprocessedVertexCode << topVertexLines;
			
		for (const auto& def : defs) // preload defs into scripts
			preprocessedVertexCode << "#define " << def << "\n";

		preprocessedVertexCode << bottomVertexLines;

		vertexCode = preprocessedVertexCode.str();


		// Process Geometry shader script
		std::string geomCode = "";
		if (geomFile != "")
		{
			std::optional<std::string> gcOpt = this->loadShaderFile(this->dataDir + "/shaders/" + geomFile);
			if (!gcOpt)
				return nullptr;

			geomCode = gcOpt.value();
			std::string topGeomLines = this->getTopShaderLines(geomCode, 10);
			std::string bottomGeomLines = this->getBottomShaderLines(geomCode, 10);
			std::stringstream preprocessedGeomCode;
			preprocessedGeomCode << topGeomLines;

			for (const auto& def : defs) // preload defs into scripts
				preprocessedGeomCode << "#define " << def << "\n";

			preprocessedGeomCode << bottomGeomLines;

			geomCode = preprocessedGeomCode.str();
		}
			

		// Process fragment shader script
		std::optional<std::string> fcOpt = this->loadShaderFile(this->dataDir + "/shaders/" + fragFile);
		if (!fcOpt)
			return nullptr;

		std::string fragmentCode = fcOpt.value();
		std::string topFragmentLines = this->getTopShaderLines(fragmentCode, 10);
		std::string bottomFragmentLines = this->getBottomShaderLines(fragmentCode, 10);
		std::stringstream preprocessedFragmentCode;
		preprocessedFragmentCode << topFragmentLines;

		for (const auto& def : defs) // preload defs into scripts
			preprocessedFragmentCode << "#define " << def << "\n";

		preprocessedFragmentCode << bottomFragmentLines;			
		fragmentCode = preprocessedFragmentCode.str();

		
		// Build the shader
		std::unique_ptr<Shader> s = std::make_unique<Shader>();
		s->name = name;
		s->vertCode = vertexCode;
		s->geomCode = geomCode;
		s->fragCode = fragmentCode;

		this->shaders.push_back(std::pair<std::unique_ptr<Shader>, int>(std::move(s), 1));

		Shader* loadedShader = this->shaders.back().first.get();

		this->gpu->loadShader(loadedShader);

		return loadedShader;
	}

	Shader* AssetManager::getShader(const std::string& name)
	{
		int shaderIndex = this->getShaderIndex(name);

		if (shaderIndex == -1)
		{
			VEL3D_LOG_DEBUG("AssetManager::getShader(): Attempting to get shader that does not exist: {}", name);
			return nullptr;
		}

		return this->shaders.at(shaderIndex).first.get();
	}

	void AssetManager::removeShader(const Shader* pShader)
	{
		int shaderIndex = this->getShaderIndex(pShader);

		if (shaderIndex == -1)
			return;

		auto& s = this->shaders.at(shaderIndex);
		s.second--;

		if (s.second == 0)
		{
			VEL3D_LOG_DEBUG("Full remove Shader: {}", pShader->name);
			
			this->gpu->clearShader(s.first.get());

			this->shaders.erase(this->shaders.begin() + shaderIndex);

			return;
		}

		VEL3D_LOG_DEBUG("Decrement Shader usageCount, retain: {}", pShader->name);
	}


	/***********************************************************************************************
	* MESHES
	************************************************************************************************/
	int AssetManager::getMeshIndex(const std::string& name)
	{
		for (int i = 0; i < this->meshes.size(); i++)
			if (this->meshes.at(i).first->getName() == name)
				return i;

		return -1;
	}

	int AssetManager::getMeshIndex(const Mesh* m)
	{
		for (int i = 0; i < this->meshes.size(); i++)
			if (this->meshes.at(i).first.get() == m)
				return i;

		return -1;
	}

	// returns a tuple of a vector of mesh names that were loaded along with the name of an armature
	// if the loaded file happened to contain an armature as well
	std::optional<std::pair<std::vector<Mesh*>, Armature*>> AssetManager::loadMesh(const std::string& path)
	{
		std::optional<std::pair<std::vector<std::string>, std::string>> plOpt = this->meshLoader->preload(path);
		if (!plOpt)
		{
			VEL3D_LOG_DEBUG("AssetManager::loadMesh: failed to preload required data for loading of mesh");
			return std::nullopt;
		}

		auto preLoadData = plOpt.value();

		std::pair<std::vector<Mesh*>, Armature*> out({}, nullptr);

		// check for duplicates
		std::pair<std::vector<std::string>, std::string> requiredData;
		for (auto& pld : preLoadData.first)
		{
			int meshIndex = this->getMeshIndex(pld);

			if (meshIndex == -1)
			{
				requiredData.first.push_back(pld);
			}		
			else
			{
				this->meshes.at(meshIndex).second++;
				out.first.push_back(this->meshes.at(meshIndex).first.get());
			}
		}

		if (preLoadData.second != "")
		{
			int armatureIndex = this->getArmatureIndex(preLoadData.second);

			if (armatureIndex == -1)
			{
				requiredData.second = preLoadData.second;
			}
			else
			{
				this->armatures.at(armatureIndex).second++;
				out.second = this->armatures.at(armatureIndex).first.get();
			}
		}

		auto loadedAssets = this->meshLoader->load(requiredData);

		for (auto& lam : loadedAssets.first)
		{
			this->meshes.push_back(std::pair<std::unique_ptr<Mesh>, int>(std::move(lam), 1));
			out.first.push_back(this->meshes.back().first.get());

			if (this->gpu != nullptr)
				this->gpu->loadMesh(this->meshes.back().first.get());
		}

		if (loadedAssets.second)
		{
			this->armatures.push_back(std::pair<std::unique_ptr<Armature>, int>(std::move(loadedAssets.second), 1));
			out.second = this->armatures.back().first.get();
		}

		this->meshLoader->reset();
			
		return out;
	}

	// This method assumes that the caller understands no duplication checks are occuring
	Mesh* AssetManager::addMesh(std::unique_ptr<Mesh> m)
	{		
		this->meshes.push_back(std::pair<std::unique_ptr<Mesh>, int>(std::move(m), 1));

		if(this->gpu != nullptr)
			this->gpu->loadMesh(this->meshes.back().first.get());

		return this->meshes.back().first.get();
	}

	void AssetManager::incrementMeshUsage(const Mesh* pMesh)
	{
		for (auto& meshUsagePair : this->meshes)
		{
			if (meshUsagePair.first.get() == pMesh)
			{
				meshUsagePair.second++;
				return;
			}
		}

		VEL3D_LOG_DEBUG("{} is not an existing mesh, and cannot be incremented", pMesh->getName());
	}

	// updates gpu state of provided Mesh*
	void AssetManager::updateMesh(Mesh* m)
	{
		if (this->gpu != nullptr)
			this->gpu->updateMesh(m);
	}

	Mesh* AssetManager::getMesh(const std::string& name)
	{
		int meshIndex = this->getMeshIndex(name);

		if (meshIndex == -1)
		{
			VEL3D_LOG_DEBUG("AssetManager::getMesh(): Attempting to get mesh that does not exist: {}", name);
			return nullptr;
		}

		return this->meshes.at(meshIndex).first.get();
	}
	
	void AssetManager::removeMesh(const Mesh* pMesh)
	{
		int meshIndex = this->getMeshIndex(pMesh);

		if (meshIndex == -1)
			return;

		auto& m = this->meshes.at(meshIndex);
		m.second--;

		if(m.second == 0)
		{
			VEL3D_LOG_DEBUG("Full remove Mesh: {}", pMesh->getName());

			if (this->gpu != nullptr)
				this->gpu->clearMesh(m.first.get());

			this->meshes.erase(this->meshes.begin() + meshIndex);

			return;
		}

		VEL3D_LOG_DEBUG("Decrement Mesh usageCount, retain: {}", pMesh->getName());
	}


	/***********************************************************************************************
	* ARMATURES
	************************************************************************************************/
	int AssetManager::getArmatureIndex(const std::string& name)
	{
		for (int i = 0; i < this->armatures.size(); i++)
			if (this->armatures.at(i).first->getName() == name)
				return i;

		return -1;
	}

	int AssetManager::getArmatureIndex(const Armature* a)
	{
		for (int i = 0; i < this->armatures.size(); i++)
			if (this->armatures.at(i).first.get() == a)
				return i;

		return -1;
	}

	Armature* AssetManager::getArmature(const std::string& name)
	{
		int armIndex = this->getArmatureIndex(name);

		if (armIndex == -1)
		{
			VEL3D_LOG_DEBUG("AssetManager::getArmature(): Attempting to get armature that does not exis: {}", name);
			return nullptr;
		}

		return this->armatures.at(armIndex).first.get();
	}

	void AssetManager::removeArmature(const Armature* pArm)
	{
		int armIndex = this->getArmatureIndex(pArm);

		if (armIndex == -1)
			return;

		auto& a = this->armatures.at(armIndex);
		a.second--;

		if (a.second == 0)
		{
			VEL3D_LOG_DEBUG("Full remove Armature: {}", pArm->getName());

			this->armatures.erase(this->armatures.begin() + armIndex);

			return;
		}

		VEL3D_LOG_DEBUG("Decrement Armature usageCount, retain: {}", pArm->getName());
	}


	/***********************************************************************************************
	* TEXTURES
	************************************************************************************************/
	int AssetManager::getTextureIndex(const std::string& name)
	{
		for (int i = 0; i < this->textures.size(); i++)
			if (this->textures.at(i).first->name == name)
				return i;

		return -1;
	}

	int AssetManager::getTextureIndex(const Texture* t)
	{
		for (int i = 0; i < this->textures.size(); i++)
			if (this->textures.at(i).first.get() == t)
				return i;

		return -1;
	}

	std::optional<TextureData> AssetManager::generateTextureData(const std::string& path)
	{
		TextureData td;
		td.primaryImageData.data = stbi_load(
			path.c_str(),
			&td.primaryImageData.width,
			&td.primaryImageData.height,
			&td.primaryImageData.nrComponents,
			0
		);

		if (!td.primaryImageData.data)
			return std::nullopt;

		if (td.primaryImageData.nrComponents == 1)
		{
			td.alphaChannel = false;
			td.primaryImageData.sizedFormat = GL_R8; // 8 bits per channel x1 channel
			td.primaryImageData.format = GL_RED;
		}
		else if (td.primaryImageData.nrComponents == 3)
		{
			td.alphaChannel = false;
			td.primaryImageData.sizedFormat = GL_RGB8; // 8 bits per channel x3 channels
			td.primaryImageData.format = GL_RGB;
		}
		else if (td.primaryImageData.nrComponents == 4)
		{
			td.alphaChannel = true;
			td.primaryImageData.sizedFormat = GL_RGBA8; // 8 bits per channel x4 channels
			td.primaryImageData.format = GL_RGBA;
		}

		return td;
	}

	Texture* AssetManager::loadTexture(const std::string& name, const std::string& path, bool freeAfterGPULoad, unsigned int uvWrapping)
	{	
		int textureIndex = this->getTextureIndex(name);

		if(textureIndex > -1)
		{
			VEL3D_LOG_DEBUG("Existing Texture, bypass reload: {}", name);

			this->textures.at(textureIndex).second++;

			return this->textures.at(textureIndex).first.get();
		}

		VEL3D_LOG_DEBUG("Load new Texture: {}", name);

		std::unique_ptr<Texture> texture = std::make_unique<Texture>();
		texture->name = name;
		texture->uvWrapping = uvWrapping;
		texture->freeAfterGPULoad = freeAfterGPULoad;

		// Determine if path is a directory or file, if directory then load each file in the directory as a texture frame
		if (std::filesystem::is_directory(path))
		{
			std::map<int, std::string> orderedFiles;

			for (const auto& entry : std::filesystem::directory_iterator(path))
				orderedFiles[std::stoi(vel::explode_string(entry.path().filename().string(), '.')[0])] = entry.path().string();

			for (auto& of : orderedFiles)
			{
				std::optional<TextureData> td = this->generateTextureData(of.second);

				if (!td)
				{
					VEL3D_LOG_DEBUG("AssetManager::loadTexture(): failed to load all files in directory: {}", path);
					return nullptr;
				}

				texture->frames.push_back(td.value());
			}
				
		}
		else
		{
			std::optional<TextureData> td = this->generateTextureData(path);

			if (!td)
			{
				VEL3D_LOG_DEBUG("AssetManager::loadTexture(): Unable to load texture at path: {}", path);
				return nullptr;
			}

			texture->frames.push_back(td.value());
		}
			

		// loop over all frames and if any of them have alpha channel, set alpha channel member of texture to true
		texture->alphaChannel = false;
		for (auto& f : texture->frames)
		{
			if (f.alphaChannel)
			{
				texture->alphaChannel = true;
				break;
			}
		}

		this->textures.push_back(std::pair<std::unique_ptr<Texture>, int>(std::move(texture), 1));

		this->gpu->loadTexture(this->textures.back().first.get());

		return this->textures.back().first.get();
	}

	Texture* AssetManager::getTexture(const std::string& name)
	{
		int textureIndex = this->getTextureIndex(name);

		if (textureIndex == -1)
		{
			VEL3D_LOG_DEBUG("AssetManager::getTexture(): Attempting to get texture that does not exist: {}", name);
			return nullptr;
		}

		return this->textures.at(textureIndex).first.get();
	}
	
	void AssetManager::removeTexture(const Texture* pTexture)
	{
		int textureIndex = this->getTextureIndex(pTexture);

		if (textureIndex == -1)
			return;

		auto& t = this->textures.at(textureIndex);
		t.second--;
		
		if (t.second == 0)
		{
			VEL3D_LOG_DEBUG("Full remove Texture: {}", pTexture->name);

			this->gpu->clearTexture(t.first.get());

			// texture remained in system ram after gpu load for use within engine, free it now
			if (!t.first->freeAfterGPULoad)
				for (auto& td : t.first->frames)
					stbi_image_free(td.primaryImageData.data);
						
			this->textures.erase(this->textures.begin() + textureIndex);

			return;
		}

		VEL3D_LOG_DEBUG("Decrement Texture usageCount, retain: {}", pTexture->name);
	}


	/***********************************************************************************************
	* MATERIALS
	************************************************************************************************/

	int AssetManager::getMaterialIndex(const std::string& name)
	{
		for (int i = 0; i < this->materials.size(); i++)
			if (this->materials.at(i).first->getName() == name)
				return i;

		return -1;
	}

	int AssetManager::getMaterialIndex(const Material* m)
	{
		for (int i = 0; i < this->materials.size(); i++)
			if (this->materials.at(i).first.get() == m)
				return i;

		return -1;
	}

	Material* AssetManager::addMaterial(std::unique_ptr<Material> m)
	{
		int materialIndex = this->getMaterialIndex(m.get());

		if (materialIndex > -1)
		{
			VEL3D_LOG_DEBUG("Existing Material, bypass reload: {}", m->getName());

			this->materials.at(materialIndex).second++;

			return this->materials.at(materialIndex).first.get();
		}

		VEL3D_LOG_DEBUG("Loading new Material: {}", m->getName());

		this->materials.push_back(std::pair<std::unique_ptr<Material>, int>(std::move(m), 1));

		return this->materials.back().first.get();
	}

	Material* AssetManager::getMaterial(const std::string& name)
	{
		int materialIndex = this->getMaterialIndex(name);

		if (materialIndex == -1)
		{
			VEL3D_LOG_DEBUG("AssetManager::getMaterial(): Attempting to get material that does not exist: {}", name);
			return nullptr;
		}

		return this->materials.at(materialIndex).first.get();
	}

	void AssetManager::removeMaterial(const Material* pMaterial)
	{
		int materialIndex = this->getMaterialIndex(pMaterial);

		if (materialIndex == -1)
			return;

		auto& m = this->materials.at(materialIndex);
		m.second--;

		if (m.second == 0)
		{
			VEL3D_LOG_DEBUG("Full remove Material: {}", pMaterial->getName());

			this->materials.erase(this->materials.begin() + materialIndex);

			return;
		}

		VEL3D_LOG_DEBUG("Decrement Material usageCount, retain: {}", pMaterial->getName());
	}


	/***********************************************************************************************
	* CAMERAS
	************************************************************************************************/
	int AssetManager::getCameraIndex(const std::string& name)
	{
		for (int i = 0; i < this->cameras.size(); i++)
			if (this->cameras.at(i).first->getName() == name)
				return i;

		return -1;
	}

	int AssetManager::getCameraIndex(const Camera* s)
	{
		for (int i = 0; i < this->cameras.size(); i++)
			if (this->cameras.at(i).first.get() == s)
				return i;

		return -1;
	}

	Camera* AssetManager::addCamera(std::unique_ptr<Camera> c)
	{
		int cameraIndex = this->getCameraIndex(c.get());

		if (cameraIndex > -1)
		{
			VEL3D_LOG_DEBUG("Existing Camera, bypass reload: {}", c->getName());

			this->cameras.at(cameraIndex).second++;

			return this->cameras.at(cameraIndex).first.get();
		}

		VEL3D_LOG_DEBUG("Loading new Camera: {}", c->getName());

		this->cameras.push_back(std::pair<std::unique_ptr<Camera>, int>(std::move(c), 1));

		Camera* cameraPtr = this->cameras.back().first.get();

		cameraPtr->setGpu(this->gpu);
		cameraPtr->setRenderTarget(this->gpu->createRenderTarget(
			(cameraPtr->getName() + "_RT"), 
			cameraPtr->getResolution().x, 
			cameraPtr->getResolution().y
		));

		return cameraPtr;
	}

	Camera* AssetManager::getCamera(const std::string& name)
	{
		int cameraIndex = this->getCameraIndex(name);

		if (cameraIndex == -1)
		{
			VEL3D_LOG_DEBUG("AssetManager::getCamera(): Attempting to get camera that does not exist: {}", name);
			return nullptr;
		}

		return this->cameras.at(cameraIndex).first.get();
	}

	void AssetManager::removeCamera(const Camera* pCamera)
	{
		int cameraIndex = this->getCameraIndex(pCamera);

		if (cameraIndex == -1)
			return;

		auto& c = this->cameras.at(cameraIndex);
		c.second--;

		if (c.second == 0)
		{
			VEL3D_LOG_DEBUG("Full remove Camera: {}", pCamera->getName());

			this->gpu->clearRenderTarget(c.first.get()->getRenderTarget());

			this->cameras.erase(this->cameras.begin() + cameraIndex);

			return;
		}

		VEL3D_LOG_DEBUG("Decrement Camera usageCount, retain: {}", pCamera->getName());
	}


	/***********************************************************************************************
	* FONTBITMAPS
	************************************************************************************************/
	int AssetManager::getFontBitmapIndex(const std::string& name)
	{
		for (int i = 0; i < this->fontBitmaps.size(); i++)
			if (this->fontBitmaps.at(i).first->fontName == name)
				return i;

		return -1;
	}

	int AssetManager::getFontBitmapIndex(const FontBitmap* f)
	{
		for (int i = 0; i < this->fontBitmaps.size(); i++)
			if (this->fontBitmaps.at(i).first.get() == f)
				return i;

		return -1;
	}

	FontBitmap* AssetManager::loadFontBitmap(const std::string& fontName, int fontSize, const std::string& fontPath)
	{
		int fbIndex = this->getShaderIndex(fontName);

		if (fbIndex > -1)
		{
			VEL3D_LOG_DEBUG("Existing FontBitmap, bypass reload: {}", fontName);

			this->fontBitmaps.at(fbIndex).second++;

			return this->fontBitmaps.at(fbIndex).first.get();
		}

		VEL3D_LOG_DEBUG("Load new FontBitmap: {}", fontName);

		std::ifstream file(fontPath, std::ios::binary | std::ios::ate);
		if (!file.is_open())
		{
			VEL3D_LOG_DEBUG("AssetManager::loadFontBitmap(): Failed to open file: {}", fontPath);
			return nullptr;
		}

		const auto size = file.tellg();
		file.seekg(0, std::ios::beg);
		auto bytes = std::vector<uint8_t>(size);
		file.read(reinterpret_cast<char *>(&bytes[0]), size);
		file.close();

		// Build texture data using read in bytes
		auto fontData = bytes;


		std::unique_ptr<FontBitmap> fb = std::make_unique<FontBitmap>();
		fb->fontName = fontName;
		fb->fontSize = fontSize;
		fb->fontPath = fontPath;
		fb->data = std::make_unique<unsigned char[]>(fb->textureWidth * fb->textureHeight);
		fb->charInfo = std::make_unique<fb_packedchar[]>(fb->charCount);


		stbtt_pack_context context;
		bool fontInitialized = stbtt_PackBegin(&context, fb->data.get(), fb->textureWidth, fb->textureHeight, 0, 1, nullptr);

		if (!fontInitialized)
		{
			VEL3D_LOG_DEBUG("AssetManager::loadFontBitmap(): Failed to initialize font");
			return nullptr;
		}


		stbtt_PackSetOversampling(&context, fb->oversampleX, fb->oversampleY);
		bool fontPacked = stbtt_PackFontRange(&context, fontData.data(), 0, fb->fontSize, fb->firstChar, fb->charCount, (stbtt_packedchar*)fb->charInfo.get());

		if (!fontPacked)
		{
			VEL3D_LOG_DEBUG("AssetManager::loadFontBitmap(): Failed to pack font");
			return nullptr;
		}


		stbtt_PackEnd(&context);


		this->fontBitmaps.push_back(std::pair<std::unique_ptr<FontBitmap>, int>(std::move(fb), 1));

		FontBitmap* loadedFontBitmap = this->fontBitmaps.back().first.get();

		this->gpu->loadFontBitmapTexture(loadedFontBitmap);

		return loadedFontBitmap;
	}

	FontBitmap* AssetManager::getFontBitmap(const std::string& name)
	{
		int fbIndex = this->getFontBitmapIndex(name);

		if (fbIndex == -1)
		{
			VEL3D_LOG_DEBUG("AssetManager::loadFontBitmap(): Attempting to get font bitmap that does not exist: {}", name);
			return nullptr;
		}

		return this->fontBitmaps.at(fbIndex).first.get();
	}

	void AssetManager::removeFontBitmap(const FontBitmap* pFontBitmap)
	{
		int fbIndex = this->getFontBitmapIndex(pFontBitmap);

		if (fbIndex == -1)
			return;

		auto& fb = this->fontBitmaps.at(fbIndex);
		fb.second--;

		if (fb.second == 0)
		{
			VEL3D_LOG_DEBUG("Full remove FontBitmap: {}", pFontBitmap->fontName);

			this->gpu->clearTexture(&fb.first->texture);
			
			this->fontBitmaps.erase(this->fontBitmaps.begin() + fbIndex);

			return;
		}

		VEL3D_LOG_DEBUG("Decrement FontBitmap usageCount, retain: {}", fb.first->fontName);
	}

	FontGlyphInfo AssetManager::getFontGlyphInfo(uint32_t character, float offsetX, float offsetY, FontBitmap* fb)
	{
		stbtt_aligned_quad quad;

		stbtt_GetPackedQuad((stbtt_packedchar*)fb->charInfo.get(), fb->textureWidth, fb->textureHeight,
			character - fb->firstChar, &offsetX, &offsetY, &quad, 1);
		const auto xmin = quad.x0;
		const auto xmax = quad.x1;
		const auto ymin = -quad.y1;
		const auto ymax = -quad.y0;

		FontGlyphInfo info{};
		info.offsetX = offsetX;
		info.offsetY = offsetY;
		info.positions[0] = { xmin, ymin, 0 };
		info.positions[1] = { xmin, ymax, 0 };
		info.positions[2] = { xmax, ymax, 0 };
		info.positions[3] = { xmax, ymin, 0 };
		info.uvs[0] = { quad.s0, quad.t1 };
		info.uvs[1] = { quad.s0, quad.t0 };
		info.uvs[2] = { quad.s1, quad.t0 };
		info.uvs[3] = { quad.s1, quad.t1 };

		return info;
	}

	std::unique_ptr<Mesh> AssetManager::loadTextActorMesh(const TextActor* ta)
	{
		std::vector<Vertex> meshVertices = {};
		std::vector<unsigned int> meshIndices = {};

		unsigned int lastIndex = 0;

		float lineMinY = 0.0f;
		float lineMaxY = 0.0f;

		float offsetX = 0.0f;
		float offsetY = 0.0f;

		for (auto c : ta->text)
		{
			if (c == '\n')
			{
				offsetX = 0.0f;
				offsetY += fabsf(lineMinY - lineMaxY);
				lineMinY = 0.0f;
				lineMaxY = 0.0f;

				continue;
			}

			const auto glyphInfo = this->getFontGlyphInfo(c, offsetX, offsetY, ta->fontBitmap);
			offsetX = glyphInfo.offsetX;

			Vertex v1;
			v1.position = glyphInfo.positions[0];
			v1.normal = glm::vec3(0.0f, 0.0f, 1.0f);
			v1.textureCoordinates = glyphInfo.uvs[0];
			v1.materialUBOIndex = 0;
			meshVertices.push_back(v1);
			if (v1.position.y < lineMinY)
				lineMinY = v1.position.y;
			if (v1.position.y > lineMaxY)
				lineMaxY = v1.position.y;


			Vertex v2;
			v2.position = glyphInfo.positions[1];
			v2.normal = glm::vec3(0.0f, 0.0f, 1.0f);
			v2.textureCoordinates = glyphInfo.uvs[1];
			v2.materialUBOIndex = 0;
			meshVertices.push_back(v2);
			if (v2.position.y < lineMinY)
				lineMinY = v2.position.y;
			if (v2.position.y > lineMaxY)
				lineMaxY = v2.position.y;


			Vertex v3;
			v3.position = glyphInfo.positions[2];
			v3.normal = glm::vec3(0.0f, 0.0f, 1.0f);
			v3.textureCoordinates = glyphInfo.uvs[2];
			v3.materialUBOIndex = 0;
			meshVertices.push_back(v3);
			if (v3.position.y < lineMinY)
				lineMinY = v3.position.y;
			if (v3.position.y > lineMaxY)
				lineMaxY = v3.position.y;


			Vertex v4;
			v4.position = glyphInfo.positions[3];
			v4.normal = glm::vec3(0.0f, 0.0f, 1.0f);
			v4.textureCoordinates = glyphInfo.uvs[3];
			v4.materialUBOIndex = 0;
			meshVertices.push_back(v4);
			if (v4.position.y < lineMinY)
				lineMinY = v4.position.y;
			if (v4.position.y > lineMaxY)
				lineMaxY = v4.position.y;


			// Add indices with correct winding
			meshIndices.push_back(lastIndex + 1); // 1
			meshIndices.push_back(lastIndex); // 0
			meshIndices.push_back(lastIndex + 3); // 3
			meshIndices.push_back(lastIndex + 1); // 1
			meshIndices.push_back(lastIndex + 3); // 3
			meshIndices.push_back(lastIndex + 2); // 2

			lastIndex += 4;
		}

		std::unique_ptr<Mesh> m = std::make_unique<Mesh>(ta->name + "_mesh");
		m->setVertices(meshVertices);
		m->setIndices(meshIndices);

		AABB maabb = m->getAABB();

		// recalculate vertex positions for right alignment (origin of mesh at right edge)
		if (ta->alignment == TextActorAlignment::RIGHT_ALIGN)
		{
			float offsetAmount = maabb.getMaxEdge().x;
			for (auto& v : m->getVertices())
				v.position = glm::vec3((v.position.x - offsetAmount), v.position.y, v.position.z);
		}
		// recalculate vertex positions for center alignment (origin of mesh at center)
		else if (ta->alignment == TextActorAlignment::CENTER_ALIGN)
		{
			float offsetAmount = maabb.getMaxEdge().x * 0.5f;
			for (auto& v : m->getVertices())
				v.position = glm::vec3((v.position.x - offsetAmount), v.position.y, v.position.z);
		}
		// default alignment is left

		if (ta->vAlignment == TextActorVerticalAlignment::TOP_ALIGN)
		{
			float offsetAmount = maabb.getMaxEdge().y;
			for (auto& v : m->getVertices())
				v.position = glm::vec3(v.position.x, (v.position.y - offsetAmount), v.position.z);
		}
		else if (ta->vAlignment == TextActorVerticalAlignment::CENTER_ALIGN)
		{
			float offsetAmount = maabb.getMaxEdge().y * 0.5f;
			for (auto& v : m->getVertices())
				v.position = glm::vec3(v.position.x, (v.position.y - offsetAmount), v.position.z);
		}
		// default alignment is bottom


		return m;
	}


	

	
/* END OF CLASS
******************************************************************/
}