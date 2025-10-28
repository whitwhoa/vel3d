
#include "vel/HeadlessApp.h"
#include "vel/logger.hpp"

namespace vel
{
	HeadlessApp::HeadlessApp(AssetManager* am) : 
		assetManager(am),
		activeScene(nullptr),
		currentSimTick(0)
	{
		
	};

	HeadlessApp::~HeadlessApp() {};

	void HeadlessApp::addScene(std::unique_ptr<HeadlessScene> scene, bool makeActive)
	{
		std::string className = typeid(*scene).name();// name is "class Test" when we need just "Test", so trim off "class "
		className.erase(0, 6);
		scene->setName(className);

		VEL3D_LOG_DEBUG("HeadlessApp::addScene: Adding HeadlessScene: {}", className);

		scene->setAssetManager(this->assetManager);

		this->scenes.push_back(std::move(scene));

		HeadlessScene* ptrScene = this->scenes.back().get();
		ptrScene->load();

		if (makeActive)
			this->activeScene = ptrScene;
	}

	void HeadlessApp::removeScene(const std::string& name)
	{
		VEL3D_LOG_DEBUG("HeadlessApp::removeScene: Removing Scene: {}", name);

		size_t i = 0;
		for (auto& s : this->scenes)
		{
			if (s->getName() == name)
				break;

			i++;
		}

		this->scenes.erase(this->scenes.begin() + i);
	}

	bool HeadlessApp::sceneExists(const std::string& name)
	{
		for (auto& s : this->scenes)
			if (s->getName() == name)
				return true;

		return false;
	}

	HeadlessScene* HeadlessApp::getScene(const std::string& name)
	{
		for (auto& s : this->scenes)
			if (s->getName() == name)
				return s.get();

		return nullptr;
	}

	void HeadlessApp::swapScene(const std::string& name)
	{
		VEL3D_LOG_DEBUG("HeadlessScene::swapScene: Swapping to Scene: {}", name);

		for (auto& s : this->scenes)
			if (s->getName() == name)
				this->activeScene = s.get();
	}


	void HeadlessApp::stepSimulation(float dt)
	{
		if (this->activeScene == nullptr)
			return;

		this->currentSimTick++;
		this->activeScene->setTick(this->currentSimTick);

		this->activeScene->stepPhysics(dt);
		this->activeScene->fixedLoop(dt);
		this->activeScene->updateFixedAnimations(dt);
		this->activeScene->postPhysics(dt);
	}



}