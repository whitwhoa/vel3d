#include <spdlog/spdlog.h>

#include <vel/Runtime.h>
#include <vel/Scene/Scene.h>

namespace vel
{
	std::vector<Mesh*> HeadlessScene::loadMesh(const std::string& path, uint32_t meshFlags)
	{
		this->meshLoader->setHeadlessMode(true);

		const std::vector<std::pair<std::string, VtxLayout>>& preLoadData = this->meshLoader->preload(path);
		if (preLoadData.size() == 0)
		{
			SPDLOG_DEBUG("HeadlessScene::loadMesh(): preload failed");
			return {};
		}

		std::vector<Mesh*> out;

		// check for duplicates
		std::vector<std::pair<std::string, GeoPool*>> requiredData;
		for (auto& pld : preLoadData)
		{
			auto it = this->meshes.find(pld.first);

			if (it == this->meshes.end())
			{
				SPDLOG_DEBUG("HeadlessScene::loadMesh(): new mesh load: {}", pld);

				std::unique_ptr<GeoPool> soloGeoPool = std::make_unique<GeoPoolT<VtxPos>>();

				GeoPool* soloGeoPoolRawPtr = soloGeoPool.get();

				this->soloGeoPools.push_back(std::move(soloGeoPool));

				requiredData.push_back({ pld.first, soloGeoPoolRawPtr });
			}
			else
			{
				SPDLOG_DEBUG("HeadlessScene::loadMesh(): existing mesh (load bypassed): {}", pld);
				out.push_back(it->second.get());
			}
		}

		std::vector<std::unique_ptr<Mesh>> loadedAssets = this->meshLoader->load(&requiredData);

		for (auto& m : loadedAssets)
		{
			Mesh* rawPtr = m.get();
			this->meshes.emplace(m->name, std::move(m));

			out.push_back(rawPtr);
		}

		this->meshLoader->reset();

		return out;
	}

	std::vector<Mesh*> Scene::loadMesh(const std::string& path, uint32_t meshFlags)
	{
		if (!(meshFlags & MESHFLAG_RENDERABLE))
			return HeadlessScene::loadMesh(path, meshFlags);

		const std::vector<std::pair<std::string, VtxLayout>>& preLoadData = this->meshLoader->preload(path);
		if (preLoadData.size() == 0)
		{
			SPDLOG_DEBUG("Scene::loadMesh(): preload failed");
			return {};
		}

		std::vector<Mesh*> out;

		// check for duplicates
		std::vector<std::pair<std::string, GeoPool*>> requiredData;
		for (auto& pld : preLoadData)
		{
			auto it = this->meshes.find(pld.first);

			if (it == this->meshes.end())
			{
				SPDLOG_DEBUG("Scene::loadMesh(): new mesh load: {}", pld);

				if (meshFlags & MESHFLAG_POOLED)
				{
					requiredData.push_back({ pld.first, this->renderGeoPools[pld.second].get() });
				}
				else
				{
					std::unique_ptr<GeoPool> renderSoloGeoPool = nullptr;
					if (pld.second == VtxLayout::VTX_POS_NRML)
						renderSoloGeoPool = std::make_unique<GeoPoolT<VtxPosNrml>>(true);
					else if (pld.second == VtxLayout::VTX_POS_NRML_TX)
						renderSoloGeoPool = std::make_unique<GeoPoolT<VtxPosNrmlTx>>(true);
					else if (pld.second == VtxLayout::VTX_POS_NRML_TX_LM)
						renderSoloGeoPool = std::make_unique<GeoPoolT<VtxPosNrmlTxLm>>(true);
					else if (pld.second == VtxLayout::VTX_POS_NRML_TX_SKN)
						renderSoloGeoPool = std::make_unique<GeoPoolT<VtxPosNrmlTxSkn>>(true);

					GeoPool* standAlonePoolRawPtr = renderSoloGeoPool.get();

					this->renderSoloGeoPools.emplace(pld.first, std::move(renderSoloGeoPool));

					requiredData.push_back({ pld.first, standAlonePoolRawPtr });
				}
			}
			else
			{
				SPDLOG_DEBUG("Scene::loadMesh(): existing mesh (load bypassed): {}", pld);
				out.push_back(it->second.get());
			}
		}

		std::vector<std::unique_ptr<Mesh>> loadedAssets = this->meshLoader->load(&requiredData);

		for (auto& m : loadedAssets)
		{
			Mesh* rawPtr = m.get();
			this->meshes.emplace(m->name, std::move(m));

			out.push_back(rawPtr);
		}

		this->meshLoader->reset();

		return out;
	}

	Mesh* HeadlessScene::addMesh(std::unique_ptr<Mesh> m)
	{
		Mesh* rawPtr = m.get();
		this->meshes.emplace(m->name, std::move(m));

		return rawPtr;
	}

	Mesh* Scene::addMesh(std::unique_ptr<Mesh> m)
	{
		Mesh* rawPtr = HeadlessScene::addMesh(std::move(m));
		Runtime::_gpu->loadGeoPool(rawPtr->gp);

		return rawPtr;
	}

	Mesh* HeadlessScene::getMesh(const std::string& name)
	{
		auto it = this->meshes.find(name);

		if (it == this->meshes.end())
		{
			SPDLOG_ERROR("HeadlessScene::getMesh(): Attempting to get mesh that does not exist: {}", name);
			return nullptr;
		}

		return it->second.get();
	}

	void HeadlessScene::removeMesh(Mesh* m)
	{
		auto it = this->meshes.find(m->name);

		if (it == this->meshes.end())
			return;

		this->meshes.erase(it);
	}

	void Scene::removeMesh(Mesh* m)
	{
		auto it = this->meshes.find(m->name);

		if (it == this->meshes.end())
			return;

		if ((m->flags & MESHFLAG_RENDERABLE) && !(m->flags & MESHFLAG_POOLED))
			Runtime::_gpu->clearGeoPool(m->gp->gpuGeoPool.value());

		this->meshes.erase(it);
	}

} // END NAMESPACE