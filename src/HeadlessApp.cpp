#include <spdlog/spdlog.h>

#include <vel/HeadlessApp.h>


namespace vel
{
	HeadlessApp::HeadlessApp() : 
		activeScene(nullptr),
		currentSimTick(0)
	{
		
	};

	HeadlessApp::~HeadlessApp() {};

	void HeadlessApp::addScene(std::unique_ptr<HeadlessScene> scene, bool makeActive)
	{
		SPDLOG_DEBUG("HeadlessApp::addScene(): Adding new HeadlessScene");

		this->scenes.push_back(std::move(scene));

		HeadlessScene* ptrScene = this->scenes.back().get();
		ptrScene->internalLoad();

		if (makeActive)
			this->activeScene = ptrScene;
	}

	void HeadlessApp::removeScene(unsigned int id)
	{
		SPDLOG_DEBUG("HeadlessApp::removeScene(): Removing Scene: {}", id);

		size_t i = 0;
		for (auto& s : this->scenes)
		{
			if (s->getId() == id)
				break;

			i++;
		}

		this->scenes.erase(this->scenes.begin() + i);
	}

	void HeadlessApp::swapScene(unsigned int id)
	{
		SPDLOG_DEBUG("HeadlessScene::swapScene(): Swapping to Scene: {}", id);

		for (auto& s : this->scenes)
			if (s->getId() == id)
				this->activeScene = s.get();
	}


	void HeadlessApp::stepSimulation(float dt)
	{
		if (this->activeScene == nullptr)
			return;

		this->currentSimTick++;

		this->activeScene->stepPhysics(dt);
		this->activeScene->updateAnimators(dt);
		this->activeScene->internalFixedLoop(dt);
	}



}