#include <spdlog/spdlog.h>

#include <stb_headers/stb_image.h>

#include <vel/Runtime.h>
#include <vel/Scene/Scene.h>
#include <vel/Util/functions.h>

namespace vel
{
	void Scene::generateTextureData(const std::string& path, Texture& t)
	{
		t.data = stbi_load(path.c_str(), &t.width, &t.height, &t.channels, 0);

		if (!t.data)
			SPDLOG_ERROR("Scene::generateTextureData(): failed to load file: {}", path);
	}

	texture_handle Scene::loadTexture(const std::string& path, uint32_t flags)
	{
		auto it = this->textureHandleMap.find(path);
		if (it != this->textureHandleMap.end())
			return it->second;

		SPDLOG_DEBUG("Scene::loadTexture(): Loading new Texture: {}", path);

		Texture texture;
		texture.flags = flags;
		this->generateTextureData(path, texture);

		Runtime::_gpu->loadTexture(&texture);

		texture_handle handle = this->textures.size();
		this->textures.push_back(texture);
		this->textureHandleMap.emplace(path, handle);

		return handle;
	}

	std::vector<texture_handle> Scene::loadTextureFrames(const std::string& dir, uint32_t flags)
	{
		if (!std::filesystem::is_directory(dir))
		{
			SPDLOG_DEBUG("Scene::loadTextureFrames(): The following is not a valid directory: {}", dir);
			return {};
		}

		SPDLOG_DEBUG("Scene::loadTextureFrames(): Loading new Texture frames: {}", dir);

		std::map<int, std::string> orderedFiles; // sort files by filename
		for (const auto& entry : std::filesystem::directory_iterator(dir))
			orderedFiles[std::stoi(vel::explode_string(entry.path().filename().string(), '.')[0])] = entry.path().string();

		std::vector<texture_handle> out;
		for (auto& of : orderedFiles)
			out.push_back(this->loadTexture(of.second, flags));

		return out;
	}


} // END NAMESPACE