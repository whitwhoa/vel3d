
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

	void HeadlessApp::stepSimulation(float dt)
	{
		if (this->activeScene == nullptr)
			return;

		this->currentSimTick++;

		this->activeScene->stepPhysics(dt);
		this->activeScene->fixedLoop(dt);
		this->activeScene->updateFixedAnimations(dt);
		this->activeScene->postPhysics(dt);
	}



}