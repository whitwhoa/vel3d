#include <spdlog/spdlog.h>

#include <vel/Runtime.h>
#include <vel/HeadlessApp.h>


namespace vel
{
	HeadlessApp::HeadlessApp() : 
		activeScene(nullptr)
	{
		
	};

	HeadlessApp::~HeadlessApp() {};

	void HeadlessApp::addScene(std::unique_ptr<HeadlessScene> scene, bool makeActive)
	{
		SPDLOG_DEBUG("HeadlessApp::addScene(): Adding new HeadlessScene");

		if (scene->internalLoad())
		{
			HeadlessScene* rawPtr = scene.get();
			this->scenes.push_back(std::move(scene));

			if (makeActive)
				this->activeScene = rawPtr;
		}
	}

	void HeadlessApp::removeScene(unsigned int id)
	{
		SPDLOG_DEBUG("HeadlessApp::removeScene(): Removing Scene: {}", id);

		auto it = std::find_if(this->scenes.begin(), this->scenes.end(),
			[id](const auto& s) { return s->getId() == id; });

		if (it != this->scenes.end())
			this->scenes.erase(it);
		else
			SPDLOG_WARN("HeadlessApp::removeScene(): Scene not found: {}", id);
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

		Runtime::_currentSimTick++;

		this->activeScene->stepPhysics(dt);
		this->activeScene->updateAnimators(dt);
		this->activeScene->internalFixedLoop(dt);
	}



}