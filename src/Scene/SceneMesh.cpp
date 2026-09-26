#include <spdlog/spdlog.h>

#include <vel/Runtime.h>
#include <vel/Scene/Scene.h>
#include <vel/Util/Assert.h>

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
				SPDLOG_DEBUG("HeadlessScene::loadMesh(): new mesh load: {}", pld.first);

				std::unique_ptr<GeoPool> soloGeoPool = std::make_unique<GeoPoolT<VtxPos>>();

				GeoPool* soloGeoPoolRawPtr = soloGeoPool.get();

				this->soloGeoPools.emplace(pld.first, std::move(soloGeoPool));

				requiredData.push_back({ pld.first, soloGeoPoolRawPtr });
			}
			else
			{
				SPDLOG_DEBUG("HeadlessScene::loadMesh(): existing mesh (load bypassed): {}", pld.first);
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

		std::unordered_set<std::string> preloadedNames;
		for (const auto& pld : preLoadData)
		{
			if (!preloadedNames.emplace(pld.first).second)
			{
				SPDLOG_ERROR("Scene::loadMesh(): File '{}' contains multiple mesh nodes named '{}'.", path, pld.first);

				this->meshLoader->reset();
				return {};
			}
		}


		std::vector<Mesh*> out;

		// check for duplicates
		std::vector<std::pair<std::string, GeoPool*>> requiredData;
		for (auto& pld : preLoadData)
		{
			auto it = this->meshes.find(pld.first);

			if (it == this->meshes.end())
			{
				SPDLOG_DEBUG("Scene::loadMesh(): new mesh load: {}", pld.first);

				if (meshFlags & MESHFLAG_POOLED)
				{
					requiredData.push_back({ pld.first, this->renderGeoPools[pld.second].get() });
				}
				else
				{
					std::unique_ptr<GeoPool> renderSoloGeoPool;

					if (pld.second == VtxLayout::VTX_POS_NRML)
						renderSoloGeoPool = std::make_unique<GeoPoolT<VtxPosNrml>>();
					else if (pld.second == VtxLayout::VTX_POS_NRML_TX)
						renderSoloGeoPool = std::make_unique<GeoPoolT<VtxPosNrmlTx>>();
					else if (pld.second == VtxLayout::VTX_POS_NRML_TX_LM)
						renderSoloGeoPool = std::make_unique<GeoPoolT<VtxPosNrmlTxLm>>();
					else if (pld.second == VtxLayout::VTX_POS_NRML_TX_SKN)
						renderSoloGeoPool = std::make_unique<GeoPoolT<VtxPosNrmlTxSkn>>();

					VEL_ASSERT(renderSoloGeoPool, ("Scene::loadMesh(): Unsupported vertex layout for mesh: " + pld.first).c_str());

					auto [poolIt, inserted] = this->renderSoloGeoPools.try_emplace(pld.first, std::move(renderSoloGeoPool));

					VEL_ASSERT(inserted, ("Scene::loadMesh(): A standalone geometry pool named already exists without a matching mesh: " + pld.first).c_str());

					requiredData.push_back({ pld.first, poolIt->second.get() });
				}
			}
			else
			{
				SPDLOG_DEBUG("Scene::loadMesh(): existing mesh (load bypassed): {}", pld.first);
				out.push_back(it->second.get());
			}
		}

		std::vector<std::unique_ptr<Mesh>> loadedAssets = this->meshLoader->load(&requiredData);

		for (auto& mesh : loadedAssets)
		{
			mesh->flags = meshFlags;
			mesh->refreshAABB();

			std::string name = mesh->name;

			auto [it, inserted] = this->meshes.try_emplace(name, std::move(mesh));

			VEL_ASSERT(inserted, ("Scene::loadMesh(): The following mesh was unexpectedly produced more than once: " + name).c_str());

			out.push_back(it->second.get());
		}

		this->meshLoader->reset();

		return out;
	}

	Mesh* Scene::loadBillboardMesh(const std::string& name, float width, float height)
	{
		auto it = this->meshes.find(name);
		if (it != this->meshes.end())
			return it->second.get();

		GeoPoolT<VtxPosNrmlTx>* gp = static_cast<GeoPoolT<VtxPosNrmlTx>*>(this->renderGeoPools[VTX_POS_NRML_TX].get());

		std::unique_ptr<Mesh> mesh = std::make_unique<Mesh>(name);
		mesh->gp = gp;
		mesh->firstIndex = static_cast<uint32_t>(gp->indices.size());
		mesh->baseVertex = gp->vertexCount();
		mesh->flags = MESHFLAG_RENDERABLE | MESHFLAG_POOLED;

		const float halfWidth = width * 0.5f;
		const float halfHeight = height * 0.5f;

		const glm::vec3 n(0.0f, 0.0f, 1.0f); // face toward camera

		VtxPosNrmlTx v0;
		v0.position = glm::vec3(-halfWidth, halfHeight, 0.0f);
		v0.normal = n;
		v0.textureCoords = glm::vec2(0.0f, 0.0f);
		gp->vertices.push_back(v0);

		VtxPosNrmlTx v1;
		v1.position = glm::vec3(-halfWidth, -halfHeight, 0.0f);
		v1.normal = n;
		v1.textureCoords = glm::vec2(0.0f, 1.0f);
		gp->vertices.push_back(v1);

		VtxPosNrmlTx v2;
		v2.position = glm::vec3(halfWidth, -halfHeight, 0.0f);
		v2.normal = n;
		v2.textureCoords = glm::vec2(1.0f, 1.0f);
		gp->vertices.push_back(v2);

		VtxPosNrmlTx v3;
		v3.position = glm::vec3(halfWidth, halfHeight, 0.0f);
		v3.normal = n;
		v3.textureCoords = glm::vec2(1.0f, 0.0f);
		gp->vertices.push_back(v3);

		gp->indices.push_back(0);
		gp->indices.push_back(1);
		gp->indices.push_back(2);
		gp->indices.push_back(0);
		gp->indices.push_back(2);
		gp->indices.push_back(3);

		mesh->indexCount = static_cast<uint32_t>(gp->indices.size()) - mesh->firstIndex;

		// Billboard meshes contain one section using actor material slot 0.
		mesh->sections.emplace_back(mesh->firstIndex, mesh->indexCount, 0);

		mesh->refreshAABB();

		Mesh* rawPtr = mesh.get();
		this->meshes.emplace(mesh->name, std::move(mesh));

		return rawPtr;
	}

	Mesh* HeadlessScene::addMesh(std::unique_ptr<Mesh> mesh)
	{
		if (!mesh)
			return nullptr;

		std::string name = mesh->name;

		auto [it, inserted] = this->meshes.try_emplace(name, std::move(mesh));

		if (!inserted)
			SPDLOG_WARN("HeadlessScene::addMesh(): A mesh named '{}' already exists.", name);

		return it->second.get();
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