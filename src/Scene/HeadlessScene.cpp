
#include <spdlog/spdlog.h>

#include <vel/Runtime.h>
#include <vel/Scene/HeadlessScene.h>
#include <vel/Scene/Stage/Actor/Mesh/AssimpMeshLoader.h>

namespace vel
{
	unsigned int HeadlessScene::nextSceneId = 0;

	HeadlessScene::HeadlessScene() :
		id(HeadlessScene::nextSceneId++),
		meshLoader(std::make_unique<AssimpMeshLoader>())
	{}

	HeadlessScene::~HeadlessScene() {}

	void HeadlessScene::freeAssets()
	{
		for (auto& meshKV : this->meshes)
			this->removeMesh(meshKV.second.get());
	}

	unsigned int HeadlessScene::getId()
	{
		return this->id;
	}

	bool HeadlessScene::internalLoad()
	{
		return this->load();
	}

	void HeadlessScene::internalFixedLoop(float deltaTime)
	{
		this->fixedLoop(deltaTime);
	}

	void HeadlessScene::stepPhysics(float delta)
	{
		for (auto& cw : this->collisionWorlds)
			if (cw->getIsActive())
				cw->getDynamicsWorld()->stepSimulation(delta, 0);
	}

	ozz::animation::Skeleton* HeadlessScene::loadSkeleton(const std::string& name, const std::string& path)
	{
		ozz::animation::Skeleton* s = Runtime::_assetManager->loadSkeleton(name, path);

		this->skeletonsInUse.push_back(name);

		return s;
	}

	ozz::animation::Skeleton* HeadlessScene::getSkeleton(const std::string& name)
	{
		return Runtime::_assetManager->getSkeleton(name);
	}

	ozz::animation::Animation* HeadlessScene::loadAnimation(const std::string& name, const std::string& path)
	{
		ozz::animation::Animation* s = Runtime::_assetManager->loadAnimation(name, path);

		this->animationsInUse.push_back(name);

		return s;
	}

	ozz::animation::Animation* HeadlessScene::getAnimation(const std::string& name)
	{
		return Runtime::_assetManager->getAnimation(name);
	}

	CollisionWorld* HeadlessScene::addCollisionWorld(const std::string& name, float gravity)
	{
		// for some reason CollisionWorld has to be a pointer or bullet has read access violation issues
		// delete in destructor
		CollisionWorld* cw = new CollisionWorld(name, gravity);
		this->collisionWorlds.push_back(cw);

		return cw;
	}

	int HeadlessScene::getCollisionWorldIndex(const std::string& name)
	{
		for (int i = 0; i < this->collisionWorlds.size(); i++)
			if (this->collisionWorlds.at(i)->getName() == name)
				return i;

		return -1;
	}

	CollisionWorld* HeadlessScene::getCollisionWorld(const std::string& name)
	{
		return this->collisionWorlds.at(this->getCollisionWorldIndex(name));
	}

	void HeadlessScene::updateAnimators(float delta)
	{
		for (auto& s : this->stages)
			s->updateAnimators(delta);
	}

	//Stage* HeadlessScene::addStage(const std::string& name, int pos)
	//{
	//	std::unique_ptr<Stage> s = std::make_unique<Stage>(name, this->assetManager, this->getTickPointer());
	//	this->stages.push_back(std::move(s));

	//	return this->stages.back().get();
	//}

	Stage* HeadlessScene::addStage(const std::string& name, int pos)
	{
		std::unique_ptr<Stage> s = std::make_unique<Stage>(name);
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
			SPDLOG_ERROR("HeadlessScene::addStage(): invalid stage position {}", pos);
			return nullptr;
		}

		return stage;
	}

	Stage* HeadlessScene::getStage(const std::string& name)
	{
		for (int i = 0; i < this->stages.size(); i++)
			if (this->stages.at(i)->getName() == name)
				return this->stages.at(i).get();

		SPDLOG_DEBUG("HeadlessScene::getStage: Attempting to retrive stage that does not exist: {}" + name);

		return nullptr;
	}

	/***********************************************************************************************
	* LOAD MESHES
	************************************************************************************************/
	std::vector<Mesh*> HeadlessScene::loadMesh(const std::string& path)
	{
		const std::vector<std::string>& preLoadData = this->meshLoader->preload(path);
		if (preLoadData.size() == 0)
		{
			SPDLOG_DEBUG("AssetManager::loadMesh: failed to preload required data for loading of mesh");
			return {};
		}

		std::vector<Mesh*> out;

		// check for duplicates
		std::vector<std::string> requiredData;
		for (auto& pld : preLoadData)
		{
			auto it = this->meshes.find(pld);

			if (it == this->meshes.end())
				requiredData.push_back(pld);
			else
				out.push_back(it->second.get());
		}

		std::vector<std::unique_ptr<Mesh>> loadedAssets = this->meshLoader->load(&requiredData);

		for (auto& m : loadedAssets)
		{
			Mesh* rawPtr = m.get();
			this->meshes.emplace(m->getName(), std::move(m));

			out.push_back(rawPtr);
		}

		this->meshLoader->reset();

		return out;
	}

	// This method assumes that the caller understands no duplication checks are occuring
	Mesh* HeadlessScene::addMesh(std::unique_ptr<Mesh> m)
	{
		Mesh* rawPtr = m.get();
		this->meshes.emplace(m->getName(), std::move(m));

		return rawPtr;
	}

	Mesh* HeadlessScene::getMesh(const std::string& name)
	{
		auto it = this->meshes.find(name);

		if (it == this->meshes.end())
		{
			SPDLOG_ERROR("AssetManager::getMesh(): Attempting to get mesh that does not exist: {}", name);
			return nullptr;
		}
		else
		{
			return it->second.get();
		}
	}

	void HeadlessScene::removeMesh(Mesh* m)
	{
		auto it = this->meshes.find(m->getName());

		if (it == this->meshes.end())
			return;

		this->meshes.erase(m->getName());
	}

}