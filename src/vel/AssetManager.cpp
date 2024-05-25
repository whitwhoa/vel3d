#include <thread> 
#include <chrono>
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
#include "vel/Log.h"
#include "vel/functions.h"

using namespace std::chrono_literals;

namespace vel
{

	AssetManager::AssetManager(std::unique_ptr<MeshLoaderInterface> ml, GPU* gpu) :
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

	Shader* AssetManager::loadShader(const std::string& name, const std::string& vertFile, const std::string& fragFile)
	{
		int shaderIndex = this->getShaderIndex(name);

		if (shaderIndex > -1)
		{
			LOG_TO_CLI_AND_FILE("Existing Shader, bypass reload: " + name);
			
			this->shaders.at(shaderIndex).second++;

			return this->shaders.at(shaderIndex).first.get();
		}	

		LOG_TO_CLI_AND_FILE("Loading new Shader: " + name);

		// 1. retrieve the vertex/fragment source code from filePath
		std::string vertexCode;
		std::string fragmentCode;
		std::ifstream vShaderFile;
		std::ifstream fShaderFile;

		vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

		try
		{
			// open files
			vShaderFile.open(vertFile);
			fShaderFile.open(fragFile);

			std::stringstream vShaderStream, fShaderStream;

			// read file's buffer contents into streams
			vShaderStream << vShaderFile.rdbuf();
			fShaderStream << fShaderFile.rdbuf();

			// close file handlers
			vShaderFile.close();
			fShaderFile.close();

			// convert stream into string
			vertexCode = vShaderStream.str();
			fragmentCode = fShaderStream.str();

		}
		catch (std::ifstream::failure e)
		{
			std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ\n";
			std::cin.get();
			exit(EXIT_FAILURE);
		}
		

		std::unique_ptr<Shader> s = std::make_unique<Shader>();
		s->name = name;
		s->vertCode = vertexCode;
		s->fragCode = fragmentCode;


		this->shaders.push_back(std::pair<std::unique_ptr<Shader>, int>(std::move(s), 1));

		Shader* loadedShader = this->shaders.back().first.get();

		this->gpu->loadShader(loadedShader);

		return loadedShader;
	}

	Shader* AssetManager::getShader(const std::string& name)
	{
		int shaderIndex = this->getShaderIndex(name);

		LOG_CRASH_IF_TRUE(shaderIndex == -1, "AssetManager::getShader(): Attempting to get shader that does not exist: " + name);

		return this->shaders.at(shaderIndex).first.get();
	}

	void AssetManager::removeShader(const Shader* pShader)
	{
		int shaderIndex = this->getShaderIndex(pShader);

		LOG_CRASH_IF_TRUE(shaderIndex == -1, "AssetManager::removeShader(): Attempting to remove shader that does not exist: " + pShader->name);

		auto& s = this->shaders.at(shaderIndex);
		s.second--;

		if (s.second == 0)
		{
			LOG_TO_CLI_AND_FILE("Full remove Shader: " + pShader->name);
			
			this->gpu->clearShader(s.first.get());

			this->shaders.erase(this->shaders.begin() + shaderIndex);

			return;
		}

		LOG_TO_CLI_AND_FILE("Decrement Shader usageCount, retain: " + pShader->name);		
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
	std::pair<std::vector<Mesh*>, Armature*> AssetManager::loadMesh(const std::string& path)
	{
		auto preLoadData = this->meshLoader->preload(path);

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

	// updates gpu state of provided Mesh*
	void AssetManager::updateMesh(Mesh* m)
	{
		if (this->gpu != nullptr)
			this->gpu->updateMesh(m);
	}

	Mesh* AssetManager::getMesh(const std::string& name)
	{
		int meshIndex = this->getMeshIndex(name);

		LOG_CRASH_IF_TRUE(meshIndex == -1, "AssetManager::getMesh(): Attempting to get mesh that does not exist: " + name);

		return this->meshes.at(meshIndex).first.get();
	}
	
	void AssetManager::removeMesh(const Mesh* pMesh)
	{
		int meshIndex = this->getMeshIndex(pMesh);

		LOG_CRASH_IF_TRUE(meshIndex == -1, "AssetManager::removeMesh(): Attempting to remove mesh that does not exist: " + pMesh->getName());

		auto& m = this->meshes.at(meshIndex);
		m.second--;

		if(m.second == 0)
		{
			LOG_TO_CLI_AND_FILE("Full remove Mesh: " + pMesh->getName());

			if (this->gpu != nullptr)
				this->gpu->clearMesh(m.first.get());

			this->meshes.erase(this->meshes.begin() + meshIndex);

			return;
		}

		LOG_TO_CLI_AND_FILE("Decrement Mesh usageCount, retain: " + pMesh->getName());		
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

		LOG_CRASH_IF_TRUE(armIndex == -1, "AssetManager::getArmature(): Attempting to get armature that does not exist: " + name);

		return this->armatures.at(armIndex).first.get();
	}

	void AssetManager::removeArmature(const Armature* pArm)
	{
		int armIndex = this->getArmatureIndex(pArm);

		LOG_CRASH_IF_TRUE(armIndex == -1, "AssetManager::removeArmature(): Attempting to remove armature that does not exist: " + pArm->getName());

		auto& a = this->armatures.at(armIndex);
		a.second--;

		if (a.second == 0)
		{
			LOG_TO_CLI_AND_FILE("Full remove Armature: " + pArm->getName());

			this->armatures.erase(this->armatures.begin() + armIndex);

			return;
		}

		LOG_TO_CLI_AND_FILE("Decrement Armature usageCount, retain: " + pArm->getName());
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

	TextureData AssetManager::generateTextureData(const std::string& path)
	{
		TextureData td;
		td.primaryImageData.data = stbi_load(
			path.c_str(),
			&td.primaryImageData.width,
			&td.primaryImageData.height,
			&td.primaryImageData.nrComponents,
			0
		);

		LOG_CRASH_IF_FALSE(td.primaryImageData.data, "AssetManager::loadTexture(): Unable to load texture at path: " + path);
		LOG_CRASH_IF_TRUE(td.primaryImageData.width != td.primaryImageData.height, "AssetManager::loadTexture(): Texture not square: " + path);
		LOG_CRASH_IF_FALSE(isPowerOfTwo(td.primaryImageData.width), "AssetManager::loadTexture(): Texture not power of two: " + path);

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
			LOG_TO_CLI_AND_FILE("Existing Texture, bypass reload: " + name);

			this->textures.at(textureIndex).second++;

			return this->textures.at(textureIndex).first.get();
		}

		LOG_TO_CLI_AND_FILE("Load new Texture: " + name);

		std::unique_ptr<Texture> texture = std::make_unique<Texture>();
		texture->name = name;
		texture->uvWrapping = uvWrapping;
		texture->freeAfterGPULoad = freeAfterGPULoad;

		// Determine if path is a directory or file, if directory then load each file in the directory as a texture frame
		if (std::filesystem::is_directory(path))
			for (const auto & entry : std::filesystem::directory_iterator(path))
				texture->frames.push_back(this->generateTextureData(entry.path().string()));
		else
			texture->frames.push_back(this->generateTextureData(path));

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

		LOG_CRASH_IF_TRUE(textureIndex == -1, "AssetManager::getTexture(): Attempting to get texture that does not exist: " + name);

		return this->textures.at(textureIndex).first.get();
	}
	
	void AssetManager::removeTexture(const Texture* pTexture)
	{
		int textureIndex = this->getTextureIndex(pTexture);

		LOG_CRASH_IF_TRUE(textureIndex == -1, "AssetManager::removeTexture(): Attempting to remove texture that does not exist: " + pTexture->name);

		auto& t = this->textures.at(textureIndex);
		t.second--;
		
		if (t.second == 0)
		{
			LOG_TO_CLI_AND_FILE("Full remove Texture: " + pTexture->name);

			this->gpu->clearTexture(t.first.get());

			// texture remained in system ram after gpu load for use within engine, free it now
			if (!t.first->freeAfterGPULoad)
				for (auto& td : t.first->frames)
					stbi_image_free(td.primaryImageData.data);
						
			this->textures.erase(this->textures.begin() + textureIndex);

			return;
		}

		LOG_TO_CLI_AND_FILE("Decrement Texture usageCount, retain: " + pTexture->name);
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
			LOG_TO_CLI_AND_FILE("Existing Material, bypass reload: " + m->getName());

			this->materials.at(materialIndex).second++;

			return this->materials.at(materialIndex).first.get();
		}

		LOG_TO_CLI_AND_FILE("Loading new Material: " + m->getName());		

		this->materials.push_back(std::pair<std::unique_ptr<Material>, int>(std::move(m), 1));

		return this->materials.back().first.get();
	}

	Material* AssetManager::getMaterial(const std::string& name)
	{
		int materialIndex = this->getMaterialIndex(name);

		LOG_CRASH_IF_TRUE(materialIndex == -1, "AssetManager::getMaterial(): Attempting to get material that does not exist: " + name);

		return this->materials.at(materialIndex).first.get();
	}

	void AssetManager::removeMaterial(const Material* pMaterial)
	{
		int materialIndex = this->getMaterialIndex(pMaterial);

		LOG_CRASH_IF_TRUE(materialIndex == -1, "AssetManager::removeMaterial(): Attempting to remove material that does not exist: " + pMaterial->getName());

		auto& m = this->materials.at(materialIndex);
		m.second--;

		if (m.second == 0)
		{
			LOG_TO_CLI_AND_FILE("Full remove Material: " + pMaterial->getName());

			this->materials.erase(this->materials.begin() + materialIndex);

			return;
		}

		LOG_TO_CLI_AND_FILE("Decrement Material usageCount, retain: " + pMaterial->getName());
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
			LOG_TO_CLI_AND_FILE("Existing Camera, bypass reload: " + c->getName());

			this->cameras.at(cameraIndex).second++;

			return this->cameras.at(cameraIndex).first.get();
		}

		LOG_TO_CLI_AND_FILE("Loading new Camera: " + c->getName());

		this->cameras.push_back(std::pair<std::unique_ptr<Camera>, int>(std::move(c), 1));

		Camera* cameraPtr = this->cameras.back().first.get();

		cameraPtr->setGpu(this->gpu);
		cameraPtr->setRenderTarget(this->gpu->createRenderTarget(cameraPtr->getResolution().x, cameraPtr->getResolution().y));
		cameraPtr->getRenderTarget()->texture.name = cameraPtr->getName() + "_RT";
		cameraPtr->getRenderTarget()->texture.alphaChannel = false;

		return cameraPtr;
	}

	Camera* AssetManager::getCamera(const std::string& name)
	{
		int cameraIndex = this->getCameraIndex(name);

		LOG_CRASH_IF_TRUE(cameraIndex == -1, "AssetManager::getCamera(): Attempting to get camera that does not exist: " + name);

		return this->cameras.at(cameraIndex).first.get();
	}

	void AssetManager::removeCamera(const Camera* pCamera)
	{
		int cameraIndex = this->getCameraIndex(pCamera);

		LOG_CRASH_IF_TRUE(cameraIndex == -1, "AssetManager::removeCamera(): Attempting to remove camera that does not exist: " + pCamera->getName());

		auto& c = this->cameras.at(cameraIndex);
		c.second--;

		if (c.second == 0)
		{
			LOG_TO_CLI_AND_FILE("Full remove Camera: " + pCamera->getName());

			this->gpu->clearRenderTarget(c.first.get()->getRenderTarget());

			this->cameras.erase(this->cameras.begin() + cameraIndex);

			return;
		}

		LOG_TO_CLI_AND_FILE("Decrement Camera usageCount, retain: " + pCamera->getName());
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
			LOG_TO_CLI_AND_FILE("Existing FontBitmap, bypass reload: " + fontName);

			this->fontBitmaps.at(fbIndex).second++;

			return this->fontBitmaps.at(fbIndex).first.get();
		}

		LOG_TO_CLI_AND_FILE("Load new FontBitmap: " + fontName);


		// Read in bytes from file on disc
		std::ifstream file(fontPath, std::ios::binary | std::ios::ate);
		if (!file.is_open())
			Log::crash("Failed to open file: " + fontPath);

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
		LOG_CRASH_IF_FALSE(fontInitialized, "Failed to initialize font");

		stbtt_PackSetOversampling(&context, fb->oversampleX, fb->oversampleY);
		bool fontPacked = stbtt_PackFontRange(&context, fontData.data(), 0, fb->fontSize, fb->firstChar, fb->charCount, (stbtt_packedchar*)fb->charInfo.get());
		LOG_CRASH_IF_FALSE(fontPacked, "Failed to pack font");

		stbtt_PackEnd(&context);


		this->fontBitmaps.push_back(std::pair<std::unique_ptr<FontBitmap>, int>(std::move(fb), 1));

		FontBitmap* loadedFontBitmap = this->fontBitmaps.back().first.get();

		this->gpu->loadFontBitmapTexture(loadedFontBitmap);

		return loadedFontBitmap;
	}

	FontBitmap* AssetManager::getFontBitmap(const std::string& name)
	{
		int fbIndex = this->getFontBitmapIndex(name);

		LOG_CRASH_IF_TRUE(fbIndex == -1, "AssetManager::getFontBitmap(): Attempting to get font bitmap that does not exist: " + name);

		return this->fontBitmaps.at(fbIndex).first.get();
	}

	void AssetManager::removeFontBitmap(const FontBitmap* pFontBitmap)
	{
		int fbIndex = this->getFontBitmapIndex(pFontBitmap);

		LOG_CRASH_IF_TRUE(fbIndex == -1, "AssetManager::removeFontBitmap(): Attempting to remove font bitmap that does not exist: " + pFontBitmap->fontName);

		auto& fb = this->fontBitmaps.at(fbIndex);
		fb.second--;

		if (fb.second == 0)
		{
			LOG_TO_CLI_AND_FILE("Full remove FontBitmap: " + pFontBitmap->fontName);

			this->gpu->clearTexture(&fb.first->texture);
			
			this->fontBitmaps.erase(this->fontBitmaps.begin() + fbIndex);

			return;
		}

		LOG_TO_CLI_AND_FILE("Decrement FontBitmap usageCount, retain: " + fb.first->fontName);
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

		// recalculate vertex positions for center alignment (origin of mesh at center)
		if (ta->alignment == TextActorAlignment::CENTER_ALIGN)
		{
			AABB maabb = m->getAABB();
			float offsetAmount = maabb.getMaxEdge().x * 0.5f;
			for (auto& v : m->getVertices())
				v.position = glm::vec3((v.position.x - offsetAmount), v.position.y, v.position.z);
		}

		// recalculate vertex positions for right alignment (origin of mesh at right edge)
		if (ta->alignment == TextActorAlignment::RIGHT_ALIGN)
		{
			AABB maabb = m->getAABB();
			float offsetAmount = maabb.getMaxEdge().x;
			for (auto& v : m->getVertices())
				v.position = glm::vec3((v.position.x - offsetAmount), v.position.y, v.position.z);
		}

		// default alignment is left
		return m;
	}


	

	
/* END OF CLASS
******************************************************************/
}