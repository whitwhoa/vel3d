#include <spdlog/spdlog.h>

#include <vel/Runtime.h>
#include <vel/Scene/Scene.h>

namespace vel
{
	void Scene::loadBGMSound(const std::string& path)
	{
		this->bgmInUse.push_back(Runtime::_audioDevice->loadBGM(path));
	}

	bool Scene::loadSFXSound(const std::string& path)
	{
		std::optional<std::string> sfxOpt = Runtime::_audioDevice->loadSFX(path);
		if (!sfxOpt)
			return false;

		this->sfxInUse.push_back(sfxOpt.value());
		return true;
	}


} // END NAMESPACE