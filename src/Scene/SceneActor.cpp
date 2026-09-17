#include <spdlog/spdlog.h>

#include <vel/Runtime.h>
#include <vel/Scene/Scene.h>

namespace vel
{
	void Scene::hideActor(actor_handle h)
	{
		Actor& a = this->actors[h];
		a.flags &= ~ACTFLG_VISIBLE;

		for (auto& h2 : a.childActors)
			this->hideActor(h2);
	}

	void Scene::showActor(actor_handle h)
	{
		Actor& a = this->actors[h];
		a.flags |= ACTFLG_VISIBLE;

		for (auto& h2 : a.childActors)
			this->showActor(h2);
	}

} // END NAMESPACE