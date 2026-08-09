
#include <spdlog/spdlog.h>

#include <vel/Runtime.h>
#include <vel/Scene/HeadlessScene.h>


namespace vel
{
	HeadlessScene::HeadlessScene() :
		name("")
	{}

	HeadlessScene::~HeadlessScene() {}

	bool HeadlessScene::internalLoad()
	{
		return this->load();
	}

	void HeadlessScene::internalFixedLoop(float deltaTime)
	{
		this->fixedLoop(deltaTime);
	}

	void HeadlessScene::setName(const std::string& n)
	{
		this->name = n;
	}

	const std::string& HeadlessScene::getName()
	{
		return this->name;
	}

	void HeadlessScene::stepPhysics(float delta)
	{
		for (auto& cw : this->collisionWorlds)
			if (cw->getIsActive())
				cw->getDynamicsWorld()->stepSimulation(delta, 0);
	}

	bool HeadlessScene::loadMesh(const std::string& path)
	{
		std::vector<Mesh*> loadedMeshes = Runtime::_assetManager->loadMesh(path);
		if (loadedMeshes.size() == 0)
		{
			SPDLOG_DEBUG("HeadlessScene::loadMesh: call to assetManager->loadMesh resulted in nullopt");
			return false;
		}

		for (auto& t : loadedMeshes)
			this->meshesInUse.push_back(t);

		return true;
	}

	Mesh* HeadlessScene::getMesh(const std::string& name)
	{
		return Runtime::_assetManager->getMesh(name);
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

}