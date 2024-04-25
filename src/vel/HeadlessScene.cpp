
#include "vel/HeadlessScene.h"
#include "vel/Log.h"

namespace vel
{
	HeadlessScene::HeadlessScene() :
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

	void HeadlessScene::setAssetManager(AssetManager* am)
	{
		this->assetManager = am;
	}

	void HeadlessScene::updateFixedAnimations(float delta)
	{
		this->fixedAnimationTime += delta;
		for (auto& s : this->stages)
			s->updateFixedArmatureAnimations(this->fixedAnimationTime);
	}

	void HeadlessScene::stepPhysics(float delta)
	{
		for (auto& cw : this->collisionWorlds)
			if (cw->getIsActive())
				cw->getDynamicsWorld()->stepSimulation(delta, 0);
	}

	void HeadlessScene::loadMesh(const std::string& path)
	{
		auto tts = this->assetManager->loadMesh(path);

		for (auto& t : tts.first)
			this->meshesInUse.push_back(t);

		if (tts.second)
			this->armaturesInUse.push_back(tts.second);
	}

	Mesh* HeadlessScene::getMesh(const std::string& name)
	{
		return this->assetManager->getMesh(name);
	}

	Armature* HeadlessScene::getArmature(const std::string& name)
	{
		return this->assetManager->getArmature(name);
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

		LOG_CRASH("Attempting to retrive stage that does not exist: " + name);

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