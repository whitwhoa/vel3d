
#include <spdlog/spdlog.h>

#include <ozz/base/io/archive.h>
#include <ozz/base/io/stream.h>

#include <vel/Runtime.h>
#include <vel/Scene/HeadlessScene.h>
#include <vel/Scene/Mesh/AssimpMeshLoader.h>


namespace vel
{
	unsigned int HeadlessScene::nextSceneId = 1;

	HeadlessScene::HeadlessScene() :
		id(HeadlessScene::nextSceneId++),
		meshLoader(std::make_unique<AssimpMeshLoader>())
	{
	
		

	}

	HeadlessScene::~HeadlessScene() 
	{
		for (auto& cw : this->collisionWorlds)
			delete cw;
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

		SPDLOG_DEBUG("HeadlessScene::getStage(): Attempting to retrive stage that does not exist: {}" + name);

		return nullptr;
	}

	/***********************************************************************************************
	* LOAD MESHES
	************************************************************************************************/
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

	// This method assumes that the caller understands no duplication checks are occuring
	Mesh* HeadlessScene::addMesh(std::unique_ptr<Mesh> m)
	{
		Mesh* rawPtr = m.get();
		this->meshes.emplace(m->name, std::move(m));

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

	/***********************************************************************************************
	* LOAD SKELETONS
	************************************************************************************************/
	ozz::animation::Skeleton* HeadlessScene::loadSkeleton(const std::string& name, const std::string& path)
	{
		auto it = this->skeletons.find(name);
		if (it != this->skeletons.end())
		{
			SPDLOG_DEBUG("HeadlessScene::loadSkeleton(): Existing Skeleton, bypass reload: {}", name);

			return it->second.get();
		}

		SPDLOG_DEBUG("HeadlessScene::loadSkeleton(): Load new Skeleton: {}", name);

		std::unique_ptr<ozz::animation::Skeleton> skel = std::make_unique<ozz::animation::Skeleton>();

		ozz::io::File file(path.c_str(), "rb");
		if (!file.opened())
		{
			SPDLOG_DEBUG("HeadlessScene::loadSkeleton(): failed to load file: {}", path);
			return nullptr;
		}

		ozz::io::IArchive archive(&file);
		if (!archive.TestTag<ozz::animation::Skeleton>())
		{
			SPDLOG_DEBUG("HeadlessScene::loadSkeleton(): failed to load skeleton instance from file: {}", path);
			return nullptr;
		}

		// Once the tag is validated ^^, reading cannot fail.
		archive >> *skel;

		ozz::animation::Skeleton* rawSkelPtr = skel.get();
		this->skeletons.emplace(name, std::move(skel));

		return rawSkelPtr;
	}

	ozz::animation::Skeleton* HeadlessScene::getSkeleton(const std::string& name)
	{
		auto it = this->skeletons.find(name);
		if (it != this->skeletons.end())
			return it->second.get();

		SPDLOG_DEBUG("HeadlessScene::getSkeleton(): attempting to get skeleton that does not exist: {}", name);
		return nullptr;
	}

	void HeadlessScene::removeSkeleton(const std::string& name)
	{
		auto it = this->skeletons.find(name);
		if (it == this->skeletons.end())
			return;

		this->skeletons.erase(name);

		SPDLOG_DEBUG("HeadlessScene::removeSkeleton(): removed skeleton {}", name);
	}

	/***********************************************************************************************
	* LOAD ANIMATIONS
	************************************************************************************************/
	ozz::animation::Animation* HeadlessScene::loadAnimation(const std::string& name, const std::string& path)
	{
		auto it = this->animations.find(name);
		if (it != this->animations.end())
		{
			SPDLOG_DEBUG("HeadlessScene::loadAnimation(): Existing Animation, bypass reload: {}", name);

			return it->second.get();
		}

		SPDLOG_DEBUG("HeadlessScene::loadAnimation(): Load new Animation: {}", name);


		std::unique_ptr<ozz::animation::Animation> anim = std::make_unique<ozz::animation::Animation>();

		ozz::io::File file(path.c_str(), "rb");
		if (!file.opened())
		{
			SPDLOG_DEBUG("HeadlessScene::loadAnimation(): failed to load file: {}", path);
			return nullptr;
		}

		ozz::io::IArchive archive(&file);
		if (!archive.TestTag<ozz::animation::Animation>())
		{
			SPDLOG_DEBUG("HeadlessScene::loadAnimation(): failed to load animation instance from file: {}", path);
			return nullptr;
		}

		// Once the tag is validated ^^, reading cannot fail.
		archive >> *anim;

		ozz::animation::Animation* rawAnimPtr = anim.get();
		this->animations.emplace(name, std::move(anim));

		return rawAnimPtr;
	}

	ozz::animation::Animation* HeadlessScene::getAnimation(const std::string& name)
	{
		auto it = this->animations.find(name);
		if (it != this->animations.end())
			return it->second.get();

		SPDLOG_DEBUG("HeadlessScene::getAnimation(): attempting to get animation that does not exist: {}", name);
		return nullptr;
	}

	void HeadlessScene::removeAnimation(const std::string& name)
	{
		auto it = this->animations.find(name);
		if (it == this->animations.end())
			return;

		this->animations.erase(name);

		SPDLOG_DEBUG("HeadlessScene::removeAnimation(): removed animation {}", name);
	}



}