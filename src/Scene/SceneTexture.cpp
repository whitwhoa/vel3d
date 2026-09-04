#include <spdlog/spdlog.h>

#include <stb_headers/stb_image.h>

#include <vel/Runtime.h>
#include <vel/Scene/Scene.h>
#include <vel/Util/functions.h>

namespace vel
{
	std::optional<TextureData> Scene::generateTextureData(const std::string& path)
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

		return td;
	}

	Texture* Scene::loadTexture(const std::string& name, const std::string& path, int options)
	{
		if (this->textures.contains(name))
		{
			SPDLOG_DEBUG("Scene::loadTexture(): Existing Texture, bypass reload: {}", name);

			return this->textures.at(name).get();
		}

		SPDLOG_DEBUG("Scene::loadTexture(): Loading new Texture: {}", name);


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
					SPDLOG_DEBUG("Scene::loadTexture(): failed to load all files in directory: {}", path);
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
				SPDLOG_DEBUG("Scene::loadTexture(): Unable to load texture at path: {}", path);
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

		Texture* rawPtr = texture.get();

		this->textures.emplace(name, std::move(texture));

		Runtime::_gpu->loadTexture(rawPtr);

		return rawPtr;
	}

	Texture* Scene::getTexture(const std::string& name)
	{
		auto it = this->textures.find(name);

		if (it == this->textures.end())
		{
			SPDLOG_ERROR("Scene::getTexture(): Attempting to get texture that does not exist: {}", name);
			return nullptr;
		}

		return it->second.get();
	}

	void Scene::removeTexture(Texture* pTexture)
	{
		auto it = this->textures.find(pTexture->name);

		if (it == this->textures.end())
			return;

		SPDLOG_DEBUG("Scene::removeTexture(): Remove Texture: {}", pTexture->name);

		Runtime::_gpu->clearTexture(pTexture);

		// texture remained in system ram after gpu load for use within engine, free it now
		if (pTexture->options & TXT_OPT_CPU_AND_GPU)
			for (auto& td : pTexture->frames)
				stbi_image_free(td.primaryImageData.data);

		this->textures.erase(pTexture->name);
	}


} // END NAMESPACE