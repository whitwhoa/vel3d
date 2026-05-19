
#include "spdlog/spdlog.h"

#include "vel/HeadlessScene.h"


namespace vel
{
	HeadlessScene::HeadlessScene(const std::string& dataDir) :
		tick(0),
		dataDir(dataDir),
		name(""),
		assetManager(nullptr)
	{}

	HeadlessScene::~HeadlessScene() {}

	std::optional<std::pair<ActCompositeKey, unsigned int>>	HeadlessScene::getActorLocation(const std::string& name)
	{
		for (const auto& pair : this->actors)
		{
			unsigned int i = 0;
			for (const auto& actor : pair.second)
			{
				if (actor->getName() == name)
				{
					return std::pair<ActCompositeKey, unsigned int>(pair.first, i);
				}

				i++;
			}
		}

		return std::nullopt;
	}

	std::optional<std::pair<ActCompositeKey, unsigned int>>	HeadlessScene::getActorLocation(const Actor* a)
	{
		for (const auto& pair : this->actors)
		{
			unsigned int i = 0;
			for (const auto& actor : pair.second)
			{
				if (actor.get() == a)
				{
					return std::pair<ActCompositeKey, unsigned int>(pair.first, i);
				}

				i++;
			}
		}

		return std::nullopt;
	}

	void HeadlessScene::_removeActor(std::optional<std::pair<ActCompositeKey, unsigned int>> actorLocation)
	{
		if (!actorLocation.has_value())
			return;

		auto al = actorLocation.value();

		this->actors[al.first].erase(this->actors[al.first].begin() + al.second);
	}

	void HeadlessScene::removeActor(const Actor* a)
	{
		this->_removeActor(this->getActorLocation(a));
	}

	void HeadlessScene::removeActor(const std::string& name)
	{
		this->_removeActor(this->getActorLocation(name));
	}

	Actor* HeadlessScene::addActor(const Actor& actorIn)
	{
		// ogl uses 0 to indicate error, so we'll never have an index of 0, so we use that for empty
		unsigned int fboToUse = actorIn.getMaterial()->getHasAlphaChannel() ? 2 : 1; // always either opaque or has alpha
		unsigned int shaderProgramId = actorIn.getMaterial()->getShader() == nullptr ? 0 : actorIn.getMaterial()->getShader()->id;
		unsigned int vaoToUse = !actorIn.getMesh()->getGpuMesh().has_value() ? 0 : actorIn.getMesh()->getGpuMesh()->VAO;

		std::unique_ptr<Actor> a = std::make_unique<Actor>(actorIn);

		Actor* ptrA = a.get(); // save raw pointer for return after move

		ActCompositeKey key = { fboToUse, shaderProgramId, vaoToUse };

		this->actors[key].push_back(std::move(a));

		return ptrA;
	}

	Actor* HeadlessScene::addActor(const std::string& name, Mesh* mesh, Material* material)
	{
		// ogl uses 0 to indicate error, so we'll never have an index of 0, so we use that for empty
		unsigned int fboToUse = 1; // always either opaque or has alpha
		if (material != nullptr)
			fboToUse = material->getHasAlphaChannel() ? 2 : 1;

		unsigned int shaderProgramId = material == nullptr ? 0 : material->getShader()->id;
		unsigned int vaoToUse = mesh == nullptr ? 0 : mesh->getGpuMesh()->VAO;

		std::unique_ptr<Actor> a = std::make_unique<Actor>(name);
		a->setMesh(mesh);

		if (material) // actor default to EmptyMaterial if none provided
			a->setMaterial(material);

		Actor* ptrA = a.get(); // save raw pointer for return after move

		ActCompositeKey key = { fboToUse, shaderProgramId, vaoToUse };

		this->actors[key].push_back(std::move(a));

		return ptrA;
	}

	Actor* HeadlessScene::getActor(const std::string& name)
	{
		for (const auto& pair : this->actors)
			for (const auto& actor : pair.second)
				if (actor->getName() == name)
					return actor.get();

		return nullptr;
	}



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

	std::map<ActCompositeKey, std::vector<std::unique_ptr<Actor>>>& HeadlessScene::getActors()
	{
		return this->actors;
	}

	void HeadlessScene::addSkelAnimator(std::unique_ptr<SkelAnimator> sa)
	{
		this->animators.push_back(std::move(sa));
	}

	void HeadlessScene::updateAnimators(float delta)
	{
		for (auto& a : this->animators)
			a->update(delta);
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
			SPDLOG_DEBUG("HeadlessScene::loadMesh: call to assetManager->loadMesh resulted in nullopt");
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