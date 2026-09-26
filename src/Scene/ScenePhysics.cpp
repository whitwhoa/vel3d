#include <spdlog/spdlog.h>

#include <vel/Runtime.h>
#include <vel/Scene/Scene.h>
#include <vel/Util/Assert.h>

namespace vel
{
	CollisionWorld* HeadlessScene::addCollisionWorld(const std::string& name, float gravity)
	{
		// for some reason CollisionWorld has to be a pointer or bullet has read access violation issues
		// TODO: did we ever try unique_ptr here?
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
		int index = this->getCollisionWorldIndex(name);

		VEL_ASSERT(index >= 0, ("HeadlessScene::getCollisionWorld(): No collision world named '" + name + "' exists.").c_str());

		return this->collisionWorlds[index];
	}

	void HeadlessScene::stepPhysics(float delta)
	{
		for (auto& cw : this->collisionWorlds)
			if (cw->getIsActive())
				cw->getDynamicsWorld()->stepSimulation(delta, 0);
	}

} // END NAMESPACE