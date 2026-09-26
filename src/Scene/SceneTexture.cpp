#include <filesystem>
#include <charconv>

#include <spdlog/spdlog.h>

#include <stb_headers/stb_image.h>

#include <vel/Runtime.h>
#include <vel/Scene/Scene.h>
#include <vel/Util/functions.h>
#include <vel/Util/Assert.h>

namespace vel
{
	void Scene::generateTextureData(const std::string& path, Texture& t)
	{
		t.data = stbi_load(path.c_str(), &t.width, &t.height, &t.channels, 0);

		VEL_ASSERT(t.data, ("Scene::generateTextureData(): failed to load file: " + path).c_str());
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

		Runtime::_gpu->loadTexture(texture);

		texture_handle handle = this->textures.size();
		this->textures.push_back(texture);
		this->textureHandleMap.emplace(path, handle);

		return handle;
	}

	std::vector<texture_handle> Scene::loadTextureFrames(const std::string& dir, uint32_t flags)
	{
		VEL_ASSERT(std::filesystem::is_directory(dir), ("Scene::loadTextureFrames(): The following is not a valid directory: " + dir).c_str());

		SPDLOG_DEBUG("Scene::loadTextureFrames(): Loading new texture frames: {}", dir);

		std::map<uint32_t, std::string> orderedFiles;

		for (const auto& entry : std::filesystem::directory_iterator(dir))
		{
			VEL_ASSERT(entry.is_regular_file(), ("Scene::loadTextureFrames(): Directory '" + dir + "' contains a non-file entry named '" + entry.path().filename().string() + "'.").c_str());

			const std::string stem = entry.path().stem().string();

			uint32_t frameIndex = 0;

			const char* begin = stem.data();
			const char* end = begin + stem.size();

			auto [ptr, error] = std::from_chars(begin, end, frameIndex);

			VEL_ASSERT(error == std::errc{} && ptr == end, ("Scene::loadTextureFrames(): Texture frame filename '" + entry.path().filename().string() + "' must have a completely numeric filename stem.").c_str());

			auto [it, inserted] = orderedFiles.try_emplace(frameIndex, entry.path().string());

			VEL_ASSERT(inserted, ("Scene::loadTextureFrames(): Multiple texture frames use frame index " + std::to_string(frameIndex) + ".").c_str());
		}

		std::vector<texture_handle> out;
		out.reserve(orderedFiles.size());

		for (const auto& [frameIndex, path] : orderedFiles)
			out.push_back(this->loadTexture(path, flags));

		return out;
	}

	void Scene::updateTextureHandle(texture_handle textureHandle, uint32_t bufferId, uint64_t dsaHandle)
	{
		Texture& texture = this->textures[textureHandle];

		texture.bufferId = bufferId;
		texture.dsaHandle = dsaHandle;

		auto it = this->materialTextureSlots.find(textureHandle);
		if (it == this->materialTextureSlots.end())
			return;

		for (uint32_t slot : it->second)
		{
			this->materialTexturesGpu[slot] = dsaHandle;

			Runtime::_gpu->uploadBufferSubData(
				this->bufferIds.materialTextureHandlesSsbo,
				slot * sizeof(uint64_t),
				sizeof(uint64_t),
				&this->materialTexturesGpu[slot]
			);
		}
	}

	


} // END NAMESPACE