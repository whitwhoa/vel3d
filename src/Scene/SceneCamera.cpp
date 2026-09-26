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
			Runtime::_gpu->clearRenderTargetBuffers(c->renderTarget, 0.0f, 0.0f, 0.0f, 0.0f);

		Runtime::_gpu->clearFinalRenderTarget(this->sceneRenderTarget, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
	}

	texture_handle Scene::createCameraTexture(Camera* c)
	{
		std::string name = "camera_" + std::to_string(c->getId());
		auto it = this->textureHandleMap.find(name);
		if (it != this->textureHandleMap.end())
			return it->second;

		SPDLOG_DEBUG("Scene::createCameraTexture(): Creating new camera texture: {}", name);

		Texture texture;
		texture.flags = TXTRFLG_RT_WRAPPER;
		texture.bufferId = c->renderTarget.opaqueBufferId;
		texture.dsaHandle = c->renderTarget.opaqueDsaHandle;

		texture_handle handle = this->textures.size();
		this->textures.push_back(texture);
		this->textureHandleMap.emplace(name, handle);

		c->renderTargetTextureHandle = handle;

		return handle;
	}

	void Scene::refreshCameraTexture(Camera& camera)
	{
		if (!camera.renderTargetTextureHandle)
			return;

		texture_handle handle = camera.renderTargetTextureHandle.value();

		const Texture& texture = this->textures[handle];

		if (texture.bufferId == camera.renderTarget.opaqueBufferId && texture.dsaHandle == camera.renderTarget.opaqueDsaHandle)
			return;

		this->updateTextureHandle(handle, camera.renderTarget.opaqueBufferId, camera.renderTarget.opaqueDsaHandle);
	}

} // END NAMESPACE