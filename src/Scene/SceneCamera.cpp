#include <spdlog/spdlog.h>

#include <vel/Runtime.h>
#include <vel/Scene/Scene.h>

namespace vel
{
	Camera* Scene::addCamera(CameraType type)
	{
		std::unique_ptr<Camera> c = std::make_unique<Camera>(type);

		Camera* cameraPtr = c.get();
		this->cameras.push_back(std::move(c));

		return cameraPtr;
	}

	Camera* Scene::getCamera(unsigned int id)
	{
		for (auto& c : this->cameras)
			if (c->getId() == id)
				return c.get();

		SPDLOG_WARN("Attempting to get camera that does not exist: {}", id);
		return nullptr;
	}

	void Scene::updateAllCameraResolutions(int x, int y)
	{
		for (auto& c : this->cameras)
			if (!c->resolutionFixed)
				c->resolution = { x, y };
	}

	void Scene::clearAllRenderTargetBuffers()
	{
		for (auto& c : this->cameras)
		{
			Runtime::_gpu->setRenderTarget(&c->renderTarget);
			Runtime::_gpu->clearRenderTargetBuffers(0.0f, 0.0f, 0.0f, 0.0f);
		}

		Runtime::_gpu->clearFinalRenderTarget(this->sceneRenderTarget.get(), glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
	}



} // END NAMESPACE