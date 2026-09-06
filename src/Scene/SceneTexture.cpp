#include <spdlog/spdlog.h>

#include <stb_headers/stb_image.h>

#include <vel/Runtime.h>
#include <vel/Scene/Scene.h>
#include <vel/Util/functions.h>

namespace vel
{
	TextureData Scene::generateTextureData(const std::string& path)
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
		{
			SPDLOG_ERROR("Scene::generateTextureData(): failed to load file: {}", path);
		}

		return td;
	}

	unsigned int Scene::loadTexture(const std::string& path, unsigned int flags)
	{
		SPDLOG_DEBUG("Scene::loadTexture(): Loading new Texture: {}", path);

		Texture texture;
		texture.flags = flags;

		// Determine if path is a directory or file, if directory then load each file in the directory as a texture frame
		if (std::filesystem::is_directory(path))
		{
			std::map<int, std::string> orderedFiles;

			for (const auto& entry : std::filesystem::directory_iterator(path))
				orderedFiles[std::stoi(vel::explode_string(entry.path().filename().string(), '.')[0])] = entry.path().string();

			for (auto& of : orderedFiles)
				texture.frames.push_back(this->generateTextureData(of.second));
		}
		else
		{
			texture.frames.push_back(this->generateTextureData(path));
		}

		Runtime::_gpu->loadTexture(&texture);

		unsigned int textureIndex = this->textures.size();
		this->textures.push_back(texture);

		return textureIndex;
	}


} // END NAMESPACE