#include <spdlog/spdlog.h>

#include <vel/Runtime.h>
#include <vel/Scene/Scene.h>

namespace vel
{
	Material* Scene::addMaterial(std::unique_ptr<Material> m)
	{
		if (this->materials.contains(m->getName()))
		{
			SPDLOG_DEBUG("Scene::addMaterial(): Existing Material, bypass reload: {}", m->getName());

			return this->materials.at(m->getName()).get();
		}

		SPDLOG_DEBUG("Scene::addMaterial(): Loading new Material: {}", m->getName());

		Material* rawPtr = m.get();
		this->materials.emplace(m->getName(), std::move(m));

		return rawPtr;
	}

	Material* Scene::getMaterial(const std::string& name)
	{
		auto it = this->materials.find(name);

		if (it == this->materials.end())
		{
			SPDLOG_ERROR("Scene::getMaterial(): Attempting to get material that does not exist: {}", name);
			return nullptr;
		}

		return it->second.get();
	}

	void Scene::removeMaterial(Material* pMaterial)
	{
		auto it = this->materials.find(pMaterial->getName());

		if (it == this->materials.end())
			return;

		SPDLOG_DEBUG("Scene::removeMaterial(): Remove Material: {}", pMaterial->getName());

		this->materials.erase(pMaterial->getName());
	}


} // END NAMESPACE