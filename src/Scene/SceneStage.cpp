#include <spdlog/spdlog.h>

#include <vel/Runtime.h>
#include <vel/Scene/Scene.h>

namespace vel
{

	Stage* Scene::addStage(int pos)
	{
		std::unique_ptr<Stage> s = std::make_unique<Stage>();
		Stage* stage = s.get();

		if (pos == -1 || pos >= static_cast<int>(this->stages.size()))
		{
			this->stages.push_back(std::move(s));
		}
		else if (pos >= 0)
		{
			this->stages.insert(this->stages.begin() + pos, std::move(s));
		}
		else
		{
			SPDLOG_ERROR("Scene::addStage(): invalid stage position {}", pos);
			return nullptr;
		}

		return stage;
	}

} // END NAMESPACE