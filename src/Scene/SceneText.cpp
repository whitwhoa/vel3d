#include <fstream>

#include <spdlog/spdlog.h>

#include <stb_headers/stb_truetype.h>

#include <vel/Runtime.h>
#include <vel/Scene/Scene.h>

namespace vel
{

	FontGlyphInfo Scene::getFontGlyphInfo(uint32_t character, float offsetX, float offsetY, FontBitmap* fb)
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

	void Scene::buildTextGeometry(Text& text, Mesh* mesh)
	{
		GeoPoolT<VtxPosNrmlTx>* gp = static_cast<GeoPoolT<VtxPosNrmlTx>*>(mesh->gp);
		gp->vertices.clear();
		gp->indices.clear();


		text.caretPositions.clear();
		text.logicalWidth = 0.0f;

		unsigned int lastIndex = 0;
		unsigned int lineCount = 1;

		float offsetX = 0.0f;
		float offsetY = 0.0f;

		text.caretPositions.push_back({ offsetX, -offsetY });

		for (auto c : text.text)
		{
			if (c == '\n')
			{
				if (offsetX > text.logicalWidth)
					text.logicalWidth = offsetX;

				++lineCount;
				offsetX = 0.0f;
				offsetY += text.fontBitmap->lineHeight;

				text.caretPositions.push_back({ offsetX, -offsetY });

				continue;
			}

			const auto glyphInfo = this->getFontGlyphInfo(c, offsetX, offsetY, text.fontBitmap);
			offsetX = glyphInfo.offsetX;
			offsetY = glyphInfo.offsetY;

			VtxPosNrmlTx v1;
			v1.position = glyphInfo.positions[0];
			v1.normal = glm::vec3(0.0f, 0.0f, 1.0f);
			v1.textureCoords = glyphInfo.uvs[0];
			gp->vertices.push_back(v1);

			VtxPosNrmlTx v2;
			v2.position = glyphInfo.positions[1];
			v2.normal = glm::vec3(0.0f, 0.0f, 1.0f);
			v2.textureCoords = glyphInfo.uvs[1];
			gp->vertices.push_back(v2);

			VtxPosNrmlTx v3;
			v3.position = glyphInfo.positions[2];
			v3.normal = glm::vec3(0.0f, 0.0f, 1.0f);
			v3.textureCoords = glyphInfo.uvs[2];
			gp->vertices.push_back(v3);

			VtxPosNrmlTx v4;
			v4.position = glyphInfo.positions[3];
			v4.normal = glm::vec3(0.0f, 0.0f, 1.0f);
			v4.textureCoords = glyphInfo.uvs[3];
			gp->vertices.push_back(v4);

			// Add indices with correct winding
			gp->indices.push_back(lastIndex + 1); // 1
			gp->indices.push_back(lastIndex); // 0
			gp->indices.push_back(lastIndex + 3); // 3
			gp->indices.push_back(lastIndex + 1); // 1
			gp->indices.push_back(lastIndex + 3); // 3
			gp->indices.push_back(lastIndex + 2); // 2

			lastIndex += 4;

			text.caretPositions.push_back({ offsetX, -offsetY });
		}

		mesh->indexCount = gp->indices.size();

		mesh->sections.clear();
		mesh->sections.emplace_back(mesh->firstIndex, mesh->indexCount, 0);

		mesh->refreshAABB();

		float minX = mesh->aabb.minEdge.x;
		float maxX = mesh->aabb.maxEdge.x;
		float logicalMaxY = text.fontBitmap->ascent;
		float logicalMinY = text.fontBitmap->descent - static_cast<float>(lineCount - 1) * text.fontBitmap->lineHeight;

		float xOffset = 0.0f;
		float yOffset = 0.0f;

		// Horizontal alignment can remain based on actual geometry.
		if (text.originType == PlaneOrigin::RIGHT_BOTTOM ||
			text.originType == PlaneOrigin::RIGHT_CENTER ||
			text.originType == PlaneOrigin::RIGHT_TOP)
		{
			xOffset = maxX;
		}
		else if (text.originType == PlaneOrigin::CENTER_BOTTOM ||
			text.originType == PlaneOrigin::CENTER_CENTER ||
			text.originType == PlaneOrigin::CENTER_TOP)
		{
			xOffset = (minX + maxX) * 0.5f;
		}

		// Vertical alignment uses stable font metrics.
		if (text.originType == PlaneOrigin::LEFT_TOP ||
			text.originType == PlaneOrigin::CENTER_TOP ||
			text.originType == PlaneOrigin::RIGHT_TOP)
		{
			yOffset = logicalMaxY;
		}
		else if (text.originType == PlaneOrigin::LEFT_CENTER ||
			text.originType == PlaneOrigin::CENTER_CENTER ||
			text.originType == PlaneOrigin::RIGHT_CENTER)
		{
			yOffset = (logicalMinY + logicalMaxY) * 0.5f;
		}
		else // BOTTOM
		{
			yOffset = logicalMinY;
		}

		for (auto& v : gp->vertices)
		{
			v.position.x -= xOffset;
			v.position.y -= yOffset;
		}
		mesh->refreshAABB();

		for (auto& p : text.caretPositions)
		{
			p.x -= xOffset;
			p.y -= yOffset;
		}

		text.logicalHeight = logicalMaxY - logicalMinY;
		if (offsetX > text.logicalWidth)
			text.logicalWidth = offsetX;
		//text.logicalWidth = maabb.getSize().x;
	}

	float Scene::measureFontHeight(const std::string& text, FontBitmap* fb)
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

	FontBitmap* Scene::loadFontBitmapRaw(const std::string& fontName, int stbFontSize, const std::string& fontPath)
	{
		if (this->fontBitmaps.contains(fontName))
		{
			SPDLOG_DEBUG("Scene::loadFontBitmapRaw(): Existing FontBitmap, bypass reload: {}", fontName);

			return &this->fontBitmaps.at(fontName);
		}

		SPDLOG_DEBUG("Scene::loadFontBitmapRaw(): Loading new FontBitmap: {}", fontName);


		std::ifstream file(fontPath, std::ios::binary | std::ios::ate);
		if (!file.is_open())
		{
			SPDLOG_DEBUG("Scene::loadFontBitmapRaw(): Failed to open file: {}", fontPath);
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

		FontBitmap fb;
		fb.fontName = fontName;
		fb.fontSize = stbFontSize;
		fb.fontPath = fontPath;
		fb.data = std::make_unique<unsigned char[]>(fb.textureWidth * fb.textureHeight);
		fb.charInfo = std::make_unique<fb_packedchar[]>(fb.charCount);


		//
		// Gather font statistics
		//
		stbtt_fontinfo fontInfo;

		int fontOffset = stbtt_GetFontOffsetForIndex(fontData.data(), 0);

		if (fontOffset < 0 || !stbtt_InitFont(&fontInfo, fontData.data(), fontOffset))
		{
			SPDLOG_DEBUG("Scene::loadFontBitmapRaw(): Failed to initialize font metrics: {}", fontPath);
			return nullptr;
		}

		int ascent;
		int descent;
		int lineGap;

		stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, &lineGap);

		float fontScale = stbtt_ScaleForPixelHeight(&fontInfo, static_cast<float>(stbFontSize));

		fb.ascent = static_cast<float>(ascent) * fontScale;
		fb.descent = static_cast<float>(descent) * fontScale;
		fb.lineGap = static_cast<float>(lineGap) * fontScale;

		fb.fontHeight = fb.ascent - fb.descent;
		fb.lineHeight = fb.fontHeight + fb.lineGap;


		//
		// Pack Atlas
		//
		int maxAtlasSize = 4096;
		bool fontPacked = false;

		while (fb.textureWidth <= maxAtlasSize)
		{
			stbtt_pack_context context;
			bool fontInitialized = stbtt_PackBegin(&context, fb.data.get(), fb.textureWidth, fb.textureHeight, 0, 1, nullptr);

			if (!fontInitialized)
			{
				SPDLOG_DEBUG("Scene::loadFontBitmapRaw(): Failed to initialize font");
				return nullptr;
			}

			stbtt_PackSetOversampling(&context, fb.oversampleX, fb.oversampleY);
			fontPacked = stbtt_PackFontRange(&context, fontData.data(), 0, fb.fontSize, fb.firstChar, fb.charCount, (stbtt_packedchar*)fb.charInfo.get());

			stbtt_PackEnd(&context);

			if (fontPacked)
				break;

			fb.textureWidth *= 2;
			fb.textureHeight *= 2;
			fb.data = std::make_unique<unsigned char[]>(fb.textureWidth * fb.textureHeight);
		}

		if (!fontPacked)
		{
			SPDLOG_DEBUG("Scene::loadFontBitmapRaw(): Failed to pack font");
			return nullptr;
		}

		auto [it, inserted] = this->fontBitmaps.emplace(fontName, std::move(fb));

		return &it->second;
	}

	std::unique_ptr<Mesh> Scene::loadTextMesh(Text& text)
	{
		std::unique_ptr<GeoPoolT<VtxPosNrmlTx>> gp = std::make_unique<GeoPoolT<VtxPosNrmlTx>>();

		std::unique_ptr<Mesh> m = std::make_unique<Mesh>("TextMesh_" + std::to_string(Runtime::_nextId++));
		m->gp = gp.get();
		m->firstIndex = 0;
		m->baseVertex = 0;
		m->flags = MESHFLAG_RENDERABLE;

		this->buildTextGeometry(text, m.get());

		this->renderSoloGeoPools.emplace(m->name, std::move(gp));

		return m;
	}

	FontBitmap* Scene::loadFontBitmap(const std::string& fontName, int fontSize)
	{
		std::string fontPath = Runtime::_config.dataDir + "/fonts/" + fontName + ".ttf";
		std::string fontName2 = fontName + std::to_string(fontSize);
		
		FontBitmap* fb = this->loadFontBitmapRaw(fontName2, fontSize, fontPath);

		Texture t = Runtime::_gpu->generateFontBitmapTexture(fb);
		texture_handle th = this->textures.size();
		this->textures.push_back(t);

		fb->texture = th;

		return fb;
	}

	FontBitmap* Scene::loadFontBitmapVisualHeight(const std::string& fontName, int desiredVisiblePx)
	{
		std::string fontPath = Runtime::_config.dataDir + "/fonts/" + fontName + ".ttf";
		std::string fontName2 = fontName + std::to_string(desiredVisiblePx) + "DVPX";

		if (this->fontBitmaps.contains(fontName2))
		{
			SPDLOG_DEBUG("Scene::loadFontBitmapVisualHeight(): Existing FontBitmap, bypass reload: {}", fontName2);

			return &this->fontBitmaps.at(fontName2);
		}

		SPDLOG_DEBUG("Scene::loadFontBitmapVisualHeight(): Loading new FontBitmap: {}", fontName2);

		const std::string referenceText = "Hg";

		FontBitmap* testFont = this->loadFontBitmapRaw(fontName2 + "_calibration", desiredVisiblePx, fontPath);

		float measuredHeight = this->measureFontHeight(referenceText, testFont);

		if (measuredHeight <= 0.0f)
			return testFont;

		float correction = desiredVisiblePx / measuredHeight;
		int correctedSize = (int)std::round(desiredVisiblePx * correction);

		this->fontBitmaps.erase(testFont->fontName);


		FontBitmap* fb = this->loadFontBitmapRaw(fontName2, correctedSize, fontPath);

		Texture t = Runtime::_gpu->generateFontBitmapTexture(fb);
		texture_handle th = this->textures.size();
		this->textures.push_back(t);

		fb->texture = th;


		return fb;
	}

	text_handle Scene::addText(Stage* stage, const std::string& font, int fontSize, glm::vec4 color, const std::string& theText, PlaneOrigin originType)
	{
		Text t;
		t.text = theText;
		t.fontBitmap = this->loadFontBitmap(font, fontSize);
		t.originType = originType;


		Mesh* mesh = this->addMesh(std::move(this->loadTextMesh(t)));


		material_handle tMaterialHandle = this->addMaterial(MTLFLG_IS_TEXT | MTLFLG_HAS_TEXTURES | MTLFLG_IS_TRANSPARENT);
		Material& tMaterial = this->materials[tMaterialHandle];
		tMaterial.textures.push_back(t.fontBitmap->texture);


		t.actor = this->addActor(stage, mesh, { tMaterialHandle }, ACTFLG_VISIBLE);


		return this->texts.insert(t);
	}

	void Scene::updateTexts()
	{
		for (auto& t : this->texts)
		{
			if (!t.requiresUpdate)
				continue;

			Actor& a = this->actors[t.actor];

			this->buildTextGeometry(t, a.mesh);
			
			Runtime::_gpu->updateGeoPool(a.mesh->gp);

			t.requiresUpdate = false;
		}
	}

} // END NAMESPACE