
#include "vel/HeadlessScene.h"
#include "vel/logger.hpp"

namespace vel
{
	HeadlessScene::HeadlessScene(const std::string& dataDir) :
		tick(0),
		dataDir(dataDir),
		name(""),
		assetManager(nullptr),
		fixedAnimationTime(0.0f)
	{}

	HeadlessScene::~HeadlessScene() {}

	void HeadlessScene::setName(const std::string& n)
	{
		this->name = n;
	}

	const std::string& HeadlessScene::getName()
	{
		return this->name;
	}

	void HeadlessScene::setTick(uint32_t t)
	{
		this->tick = t;
	}

	uint32_t HeadlessScene::getTick()
	{
		return this->tick;
	}

	void HeadlessScene::setAssetManager(AssetManager* am)
	{
		this->assetManager = am;
	}

	void HeadlessScene::updateAnimations(float delta)
	{
		this->fixedAnimationTime += delta;
		for (auto& s : this->stages)
			s->updateAnimators(delta);
	}

	void HeadlessScene::stepPhysics(float delta)
	{
		for (auto& cw : this->collisionWorlds)
			if (cw->getIsActive())
				cw->getDynamicsWorld()->stepSimulation(delta, 0);
	}

	bool HeadlessScene::loadMesh(const std::string& path)
	{
		std::vector<Mesh*> loadedMeshes = this->assetManager->loadMesh(path);
		if (loadedMeshes.size() == 0)
		{
			VEL3D_LOG_DEBUG("HeadlessScene::loadMesh: call to assetManager->loadMesh resulted in nullopt");
			return false;
		}

		for (auto& t : loadedMeshes)
			this->meshesInUse.push_back(t);

		return true;
	}

	Mesh* HeadlessScene::getMesh(const std::string& name)
	{
		return this->assetManager->getMesh(name);
	}

	ozz::animation::Skeleton* HeadlessScene::loadSkeleton(const std::string& name, const std::string& path)
	{
		ozz::animation::Skeleton* s = this->assetManager->loadSkeleton(name, path);

		this->skeletonsInUse.push_back(name);

		return s;
	}

	ozz::animation::Skeleton* HeadlessScene::getSkeleton(const std::string& name)
	{
		return this->assetManager->getSkeleton(name);
	}

	ozz::animation::Animation* HeadlessScene::loadAnimation(const std::string& name, const std::string& path)
	{
		ozz::animation::Animation* s = this->assetManager->loadAnimation(name, path);

		this->animationsInUse.push_back(name);

		return s;
	}

	ozz::animation::Animation* HeadlessScene::getAnimation(const std::string& name)
	{
		return this->assetManager->getAnimation(name);
	}

	Stage* HeadlessScene::addStage(const std::string& name)
	{
		std::unique_ptr<Stage> s = std::make_unique<Stage>(name, this->assetManager);

		this->stages.push_back(std::move(s));

		Stage* ptrStage = this->stages.back().get();

		return ptrStage;
	}

	Stage* HeadlessScene::getStage(const std::string& name)
	{
		for (int i = 0; i < this->stages.size(); i++)
			if (this->stages.at(i)->getName() == name)
				return this->stages.at(i).get();

		VEL3D_LOG_DEBUG("HeadlessScene::getStage: Attempting to retrive stage that does not exist: {}" + name);

		return nullptr;
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

}