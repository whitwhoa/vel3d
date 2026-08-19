#include <thread> 
#include <chrono>
#include <sstream>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <memory>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_headers/stb_image.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_headers/stb_truetype.h>

#include <glad/gl.h>

#include <ozz/base/io/archive.h>
#include <ozz/base/io/stream.h>

#include <spdlog/spdlog.h>

#include <vel/AssetManager.h>
#include <vel/Util/functions.h>


using namespace std::chrono_literals;

namespace vel
{

	AssetManager::AssetManager(const std::string& dataDir, GPU* gpu) :
		dataDir(dataDir),
		gpu(gpu)
	{}
	AssetManager::~AssetManager(){}


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

	Texture* AssetManager::loadTexture(const std::string& name, const std::string& path, int options)
	{
		int textureIndex = this->getTextureIndex(name);

		if (textureIndex > -1)
		{
			SPDLOG_DEBUG("Existing Texture, bypass reload: {}", name);

			this->textures.at(textureIndex).second++;

			return this->textures.at(textureIndex).first.get();
		}

		SPDLOG_DEBUG("Load new Texture: {}", name);

		std::unique_ptr<Texture> texture = std::make_unique<Texture>();
		texture->name = name;
		texture->options = options;

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
					SPDLOG_DEBUG("AssetManager::loadTexture(): failed to load all files in directory: {}", path);
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
				SPDLOG_DEBUG("AssetManager::loadTexture(): Unable to load texture at path: {}", path);
				return nullptr;
			}

			texture->frames.push_back(td.value());
		}


		// loop over all frames and if any of them have alpha channel, set HAS_ALPHA of texture to true
		for (auto& f : texture->frames)
		{
			if (f.alphaChannel)
			{
				texture->options |= TXT_OPT_HAS_ALPHA;
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
			SPDLOG_DEBUG("AssetManager::getTexture(): Attempting to get texture that does not exist: {}", name);
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
			SPDLOG_DEBUG("Full remove Texture: {}", pTexture->name);

			this->gpu->clearTexture(t.first.get());

			// texture remained in system ram after gpu load for use within engine, free it now
			if (t.first->options & TXT_OPT_CPU_AND_GPU)
				for (auto& td : t.first->frames)
					stbi_image_free(td.primaryImageData.data);
						
			this->textures.erase(this->textures.begin() + textureIndex);

			return;
		}

		SPDLOG_DEBUG("Decrement Texture usageCount, retain: {}", pTexture->name);
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
		int materialIndex = this->getMaterialIndex(m->getName());

		if (materialIndex > -1)
		{
			SPDLOG_DEBUG("Existing Material, bypass reload: {}", m->getName());

			this->materials.at(materialIndex).second++;

			return this->materials.at(materialIndex).first.get();
		}

		SPDLOG_DEBUG("Loading new Material: {}", m->getName());

		this->materials.push_back(std::pair<std::unique_ptr<Material>, int>(std::move(m), 1));

		return this->materials.back().first.get();
	}

	Material* AssetManager::getMaterial(const std::string& name)
	{
		int materialIndex = this->getMaterialIndex(name);

		if (materialIndex == -1)
		{
			SPDLOG_DEBUG("AssetManager::getMaterial(): Attempting to get material that does not exist: {}", name);
			return nullptr;
		}

		return this->materials.at(materialIndex).first.get();
	}

	int AssetManager::getMaterialUsageCount(const std::string& name)
	{
		int materialIndex = this->getMaterialIndex(name);

		if (materialIndex == -1)
			return 0;

		return this->materials.at(materialIndex).second;
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
			SPDLOG_DEBUG("Full remove Material: {}", pMaterial->getName());

			this->materials.erase(this->materials.begin() + materialIndex);

			return;
		}

		SPDLOG_DEBUG("Decrement Material usageCount, retain: {}", pMaterial->getName());
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

	float AssetManager::measureFontHeight(const std::string& text, FontBitmap* fb)
	{
		if (!fb)
			return 0.f;

		float offsetX = 0.0f;
		float offsetY = 0.0f;

		bool hasVisibleGlyph = false;

		float minY = 0.0f;
		float maxY = 0.0f;

		for (char c : text)
		{
			if (c == '\n')
			{
				offsetX = 0.0f;
				continue;
			}

			uint32_t character = static_cast<uint32_t>(static_cast<unsigned char>(c));

			if (character < fb->firstChar || character >= fb->firstChar + fb->charCount)
				continue;

			stbtt_aligned_quad quad;

			stbtt_GetPackedQuad(
				reinterpret_cast<stbtt_packedchar*>(fb->charInfo.get()),
				fb->textureWidth,
				fb->textureHeight,
				character - fb->firstChar,
				&offsetX,
				&offsetY,
				&quad,
				1
			);

			// Match existing text mesh convention.
			const float ymin = -quad.y1;
			const float ymax = -quad.y0;

			if (!hasVisibleGlyph)
			{
				minY = ymin;
				maxY = ymax;
				hasVisibleGlyph = true;
			}
			else
			{
				minY = std::min(minY, ymin);
				maxY = std::max(maxY, ymax);
			}
		}

		if (!hasVisibleGlyph)
			return 0.0f;

		return maxY - minY;
	}

	FontBitmap* AssetManager::loadFontBitmap(const std::string& fontName, int fontSize, const std::string& fontPath)
	{
		return this->loadFontBitmapRaw(fontName, fontSize, fontPath);
	}

	FontBitmap* AssetManager::loadFontBitmapVisualHeight(const std::string& fontName, int desiredVisiblePx, const std::string& fontPath)
	{
		int fbIndex = this->getFontBitmapIndex(fontName);

		if (fbIndex > -1)
		{
			SPDLOG_DEBUG("Existing FontBitmap, bypass reload: {}", fontName);

			this->fontBitmaps.at(fbIndex).second++;

			return this->fontBitmaps.at(fbIndex).first.get();
		}


		const std::string referenceText = "Hg";

		FontBitmap* testFont = this->loadFontBitmapRaw(fontName + "_calibration", desiredVisiblePx, fontPath);

		float measuredHeight = this->measureFontHeight(referenceText, testFont);

		if (measuredHeight <= 0.0f)
			return testFont;

		float correction = desiredVisiblePx / measuredHeight;
		int correctedSize = (int)std::round(desiredVisiblePx * correction);

		this->removeFontBitmap(testFont);

		return this->loadFontBitmapRaw(fontName, correctedSize, fontPath);
	}

	FontBitmap* AssetManager::loadFontBitmapRaw(const std::string& fontName, int stbFontSize, const std::string& fontPath)
	{
		int fbIndex = this->getFontBitmapIndex(fontName);

		if (fbIndex > -1)
		{
			SPDLOG_DEBUG("Existing FontBitmap, bypass reload: {}", fontName);

			this->fontBitmaps.at(fbIndex).second++;

			return this->fontBitmaps.at(fbIndex).first.get();
		}

		SPDLOG_DEBUG("Load new FontBitmap: {}", fontName);

		std::ifstream file(fontPath, std::ios::binary | std::ios::ate);
		if (!file.is_open())
		{
			SPDLOG_DEBUG("AssetManager::loadFontBitmap(): Failed to open file: {}", fontPath);
			return nullptr;
		}

		const auto size = file.tellg();
		file.seekg(0, std::ios::beg);
		auto bytes = std::vector<uint8_t>(size);
		file.read(reinterpret_cast<char*>(&bytes[0]), size);
		file.close();


		//
		// Build texture data
		//
		auto fontData = bytes;

		std::unique_ptr<FontBitmap> fb = std::make_unique<FontBitmap>();
		fb->fontName = fontName;
		fb->fontSize = stbFontSize;
		fb->fontPath = fontPath;
		fb->data = std::make_unique<unsigned char[]>(fb->textureWidth * fb->textureHeight);
		fb->charInfo = std::make_unique<fb_packedchar[]>(fb->charCount);


		//
		// Gather font statistics
		//
		stbtt_fontinfo fontInfo;

		int fontOffset = stbtt_GetFontOffsetForIndex(fontData.data(), 0);

		if (fontOffset < 0 || !stbtt_InitFont(&fontInfo, fontData.data(), fontOffset))
		{
			SPDLOG_DEBUG("AssetManager::loadFontBitmapRaw(): Failed to initialize font metrics: {}", fontPath);
			return nullptr;
		}

		int ascent;
		int descent;
		int lineGap;

		stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, &lineGap);

		float fontScale = stbtt_ScaleForPixelHeight(&fontInfo, static_cast<float>(stbFontSize));

		fb->ascent = static_cast<float>(ascent) * fontScale;
		fb->descent = static_cast<float>(descent) * fontScale;
		fb->lineGap = static_cast<float>(lineGap) * fontScale;

		fb->fontHeight = fb->ascent - fb->descent;
		fb->lineHeight = fb->fontHeight + fb->lineGap;


		//
		// Pack Atlas
		//
		int maxAtlasSize = 4096;
		bool fontPacked = false;

		while (fb->textureWidth <= maxAtlasSize)
		{
			stbtt_pack_context context;
			bool fontInitialized = stbtt_PackBegin(&context, fb->data.get(), fb->textureWidth, fb->textureHeight, 0, 1, nullptr);

			if (!fontInitialized)
			{
				SPDLOG_DEBUG("AssetManager::loadFontBitmap(): Failed to initialize font");
				return nullptr;
			}

			stbtt_PackSetOversampling(&context, fb->oversampleX, fb->oversampleY);
			fontPacked = stbtt_PackFontRange(&context, fontData.data(), 0, fb->fontSize, fb->firstChar, fb->charCount, (stbtt_packedchar*)fb->charInfo.get());

			stbtt_PackEnd(&context);

			if (fontPacked)
				break;			

			fb->textureWidth *= 2;
			fb->textureHeight *= 2;
			fb->data = std::make_unique<unsigned char[]>(fb->textureWidth * fb->textureHeight);
		}
		
		if (!fontPacked)
		{
			SPDLOG_DEBUG("AssetManager::loadFontBitmap(): Failed to pack font");
			return nullptr;
		}

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
			SPDLOG_DEBUG("AssetManager::loadFontBitmap(): Attempting to get font bitmap that does not exist: {}", name);
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
			SPDLOG_DEBUG("Full remove FontBitmap: {}", pFontBitmap->fontName);

			this->gpu->clearTexture(&fb.first->texture);
			
			this->fontBitmaps.erase(this->fontBitmaps.begin() + fbIndex);

			return;
		}

		SPDLOG_DEBUG("Decrement FontBitmap usageCount, retain: {}", fb.first->fontName);
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

	std::unique_ptr<Mesh> AssetManager::loadTextActorMesh(TextActor* ta)
	{
		std::vector<Vertex> meshVertices = {};
		std::vector<unsigned int> meshIndices = {};
		ta->caretPositions.clear();

		unsigned int lastIndex = 0;
		unsigned int lineCount = 1;

		float offsetX = 0.0f;
		float offsetY = 0.0f;

		ta->caretPositions.push_back({ offsetX, -offsetY });

		for (auto c : ta->text)
		{
			if (c == '\n')
			{
				if (offsetX > ta->logicalWidth)
					ta->logicalWidth = offsetX;

				++lineCount;
				offsetX = 0.0f;
				offsetY += ta->fontBitmap->lineHeight;

				ta->caretPositions.push_back({ offsetX, -offsetY });

				continue;
			}

			const auto glyphInfo = this->getFontGlyphInfo(c, offsetX, offsetY, ta->fontBitmap);
			offsetX = glyphInfo.offsetX;
			offsetY = glyphInfo.offsetY;

			Vertex v1;
			v1.position = glyphInfo.positions[0];
			v1.normal = glm::vec3(0.0f, 0.0f, 1.0f);
			v1.textureCoordinates = glyphInfo.uvs[0];
			v1.materialUBOIndex = 0;
			meshVertices.push_back(v1);

			Vertex v2;
			v2.position = glyphInfo.positions[1];
			v2.normal = glm::vec3(0.0f, 0.0f, 1.0f);
			v2.textureCoordinates = glyphInfo.uvs[1];
			v2.materialUBOIndex = 0;
			meshVertices.push_back(v2);

			Vertex v3;
			v3.position = glyphInfo.positions[2];
			v3.normal = glm::vec3(0.0f, 0.0f, 1.0f);
			v3.textureCoordinates = glyphInfo.uvs[2];
			v3.materialUBOIndex = 0;
			meshVertices.push_back(v3);

			Vertex v4;
			v4.position = glyphInfo.positions[3];
			v4.normal = glm::vec3(0.0f, 0.0f, 1.0f);
			v4.textureCoordinates = glyphInfo.uvs[3];
			v4.materialUBOIndex = 0;
			meshVertices.push_back(v4);

			// Add indices with correct winding
			meshIndices.push_back(lastIndex + 1); // 1
			meshIndices.push_back(lastIndex); // 0
			meshIndices.push_back(lastIndex + 3); // 3
			meshIndices.push_back(lastIndex + 1); // 1
			meshIndices.push_back(lastIndex + 3); // 3
			meshIndices.push_back(lastIndex + 2); // 2

			lastIndex += 4;

			ta->caretPositions.push_back({ offsetX, -offsetY });
		}

		std::unique_ptr<Mesh> m = std::make_unique<Mesh>(ta->name + "_mesh");
		m->setVertices(meshVertices);
		m->setIndices(meshIndices);

		AABB maabb = m->getAABB();

		float minX = maabb.getMinEdge().x;
		float maxX = maabb.getMaxEdge().x;
		float logicalMaxY = ta->fontBitmap->ascent;
		float logicalMinY = ta->fontBitmap->descent - static_cast<float>(lineCount - 1) * ta->fontBitmap->lineHeight;

		float xOffset = 0.0f;
		float yOffset = 0.0f;

		// Horizontal alignment can remain based on actual geometry.
		if (ta->originType == PlaneOrigin::RIGHT_BOTTOM ||
			ta->originType == PlaneOrigin::RIGHT_CENTER ||
			ta->originType == PlaneOrigin::RIGHT_TOP)
		{
			xOffset = maxX;
		}
		else if (ta->originType == PlaneOrigin::CENTER_BOTTOM ||
			ta->originType == PlaneOrigin::CENTER_CENTER ||
			ta->originType == PlaneOrigin::CENTER_TOP)
		{
			xOffset = (minX + maxX) * 0.5f;
		}

		// Vertical alignment uses stable font metrics.
		if (ta->originType == PlaneOrigin::LEFT_TOP ||
			ta->originType == PlaneOrigin::CENTER_TOP ||
			ta->originType == PlaneOrigin::RIGHT_TOP)
		{
			yOffset = logicalMaxY;
		}
		else if (ta->originType == PlaneOrigin::LEFT_CENTER ||
			ta->originType == PlaneOrigin::CENTER_CENTER ||
			ta->originType == PlaneOrigin::RIGHT_CENTER)
		{
			yOffset = (logicalMinY + logicalMaxY) * 0.5f;
		}
		else // BOTTOM
		{
			yOffset = logicalMinY;
		}

		for (auto& v : m->getMutableVertices())
		{
			v.position.x -= xOffset;
			v.position.y -= yOffset;
		}

		for (auto& p : ta->caretPositions)
		{
			p.x -= xOffset;
			p.y -= yOffset;
		}

		ta->logicalHeight = logicalMaxY - logicalMinY;
		if (offsetX > ta->logicalWidth)
			ta->logicalWidth = offsetX;
		//ta->logicalWidth = maabb.getSize().x;

		return m;
	}


	/***********************************************************************************************
	* SKELETONS
	************************************************************************************************/
	ozz::animation::Skeleton* AssetManager::loadSkeleton(const std::string& name, const std::string& path)
	{
		auto it = this->skeletons.find(name);
		if (it != this->skeletons.end()) 
		{
			SPDLOG_DEBUG("Existing Skeleton, bypass reload: {}", name);

			it->second.second++;

			return it->second.first.get();
		}
		
		SPDLOG_DEBUG("Load new Skeleton: {}", name);

		std::unique_ptr<ozz::animation::Skeleton> skel = std::make_unique<ozz::animation::Skeleton>();
		
		ozz::io::File file(path.c_str(), "rb");
		if (!file.opened())
		{
			SPDLOG_DEBUG("AssetManager::loadSkeleton(): failed to load file: {}", path);
			return nullptr;
		}

		ozz::io::IArchive archive(&file);
		if (!archive.TestTag<ozz::animation::Skeleton>())
		{
			SPDLOG_DEBUG("AssetManager::loadSkeleton(): failed to load skeleton instance from file: {}", path);
			return nullptr;
		}

		// Once the tag is validated ^^, reading cannot fail.
		archive >> *skel;
		
		ozz::animation::Skeleton* rawSkelPtr = skel.get();
		this->skeletons[name] = std::pair<std::unique_ptr<ozz::animation::Skeleton>, int>(std::move(skel), 1);

		return rawSkelPtr;
	}

	ozz::animation::Skeleton* AssetManager::getSkeleton(const std::string& name)
	{
		auto it = this->skeletons.find(name);
		if (it != this->skeletons.end())
			return it->second.first.get();
		
		SPDLOG_DEBUG("AssetManager::getSkeleton attempting to get skeleton that does not exist: {}", name);
		return nullptr;
	}

	void AssetManager::removeSkeleton(const std::string& name)
	{
		auto it = this->skeletons.find(name);
		if (it == this->skeletons.end())
			return;

		it->second.second--;

		if (it->second.second == 0)
		{
			SPDLOG_DEBUG("Full remove Skeleton: {}", name);
			this->skeletons.erase(name);
			return;
		}

		SPDLOG_DEBUG("Decrement Skeleton usageCount, retain: {}", name);
	}

	/***********************************************************************************************
	* ANIMATIONS
	************************************************************************************************/
	ozz::animation::Animation* AssetManager::loadAnimation(const std::string& name, const std::string& path)
	{
		auto it = this->animations.find(name);
		if (it != this->animations.end())
		{
			SPDLOG_DEBUG("Existing Animation, bypass reload: {}", name);

			it->second.second++;

			return it->second.first.get();
		}

		SPDLOG_DEBUG("Load new Animation: {}", name);

		std::unique_ptr<ozz::animation::Animation> anim = std::make_unique<ozz::animation::Animation>();

		ozz::io::File file(path.c_str(), "rb");
		if (!file.opened())
		{
			SPDLOG_DEBUG("AssetManager::loadAnimation(): failed to load file: {}", path);
			return nullptr;
		}

		ozz::io::IArchive archive(&file);
		if (!archive.TestTag<ozz::animation::Animation>())
		{
			SPDLOG_DEBUG("AssetManager::loadAnimation(): failed to load animation instance from file: {}", path);
			return nullptr;
		}

		// Once the tag is validated ^^, reading cannot fail.
		archive >> *anim;

		ozz::animation::Animation* rawAnimPtr = anim.get();
		this->animations[name] = std::pair<std::unique_ptr<ozz::animation::Animation>, int>(std::move(anim), 1);

		return rawAnimPtr;
	}

	ozz::animation::Animation* AssetManager::getAnimation(const std::string& name)
	{
		auto it = this->animations.find(name);
		if (it != this->animations.end())
			return it->second.first.get();

		SPDLOG_DEBUG("AssetManager::getAnimation attempting to get animation that does not exist: {}", name);
		return nullptr;
	}

	void AssetManager::removeAnimation(const std::string& name)
	{
		auto it = this->animations.find(name);
		if (it == this->animations.end())
			return;

		it->second.second--;

		if (it->second.second == 0)
		{
			SPDLOG_DEBUG("Full remove Animation: {}", name);
			this->animations.erase(name);
			return;
		}

		SPDLOG_DEBUG("Decrement Animation usageCount, retain: {}", name);
	}

	

	
/* END OF CLASS
******************************************************************/
}