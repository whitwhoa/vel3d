#include <spdlog/spdlog.h>

#include <ozz/base/io/archive.h>
#include <ozz/base/io/stream.h>

#include <vel/Runtime.h>
#include <vel/Scene/Scene.h>

namespace vel
{
	ozz::animation::Skeleton* HeadlessScene::loadSkeleton(const std::string& name, const std::string& path)
	{
		auto it = this->skeletons.find(name);
		if (it != this->skeletons.end())
		{
			SPDLOG_DEBUG("HeadlessScene::loadSkeleton(): Existing Skeleton, bypass reload: {}", name);

			return it->second.get();
		}

		SPDLOG_DEBUG("HeadlessScene::loadSkeleton(): Load new Skeleton: {}", name);

		std::unique_ptr<ozz::animation::Skeleton> skel = std::make_unique<ozz::animation::Skeleton>();

		ozz::io::File file(path.c_str(), "rb");
		if (!file.opened())
		{
			SPDLOG_DEBUG("HeadlessScene::loadSkeleton(): failed to load file: {}", path);
			return nullptr;
		}

		ozz::io::IArchive archive(&file);
		if (!archive.TestTag<ozz::animation::Skeleton>())
		{
			SPDLOG_DEBUG("HeadlessScene::loadSkeleton(): failed to load skeleton instance from file: {}", path);
			return nullptr;
		}

		// Once the tag is validated ^^, reading cannot fail.
		archive >> *skel;

		ozz::animation::Skeleton* rawSkelPtr = skel.get();
		this->skeletons.emplace(name, std::move(skel));

		return rawSkelPtr;
	}

	ozz::animation::Skeleton* HeadlessScene::getSkeleton(const std::string& name)
	{
		auto it = this->skeletons.find(name);
		if (it != this->skeletons.end())
			return it->second.get();

		SPDLOG_DEBUG("HeadlessScene::getSkeleton(): attempting to get skeleton that does not exist: {}", name);
		return nullptr;
	}

	void HeadlessScene::removeSkeleton(const std::string& name)
	{
		auto it = this->skeletons.find(name);
		if (it == this->skeletons.end())
			return;

		this->skeletons.erase(name);

		SPDLOG_DEBUG("HeadlessScene::removeSkeleton(): removed skeleton {}", name);
	}

	ozz::animation::Animation* HeadlessScene::loadAnimation(const std::string& name, const std::string& path)
	{
		auto it = this->animations.find(name);
		if (it != this->animations.end())
		{
			SPDLOG_DEBUG("HeadlessScene::loadAnimation(): Existing Animation, bypass reload: {}", name);

			return it->second.get();
		}

		SPDLOG_DEBUG("HeadlessScene::loadAnimation(): Load new Animation: {}", name);


		std::unique_ptr<ozz::animation::Animation> anim = std::make_unique<ozz::animation::Animation>();

		ozz::io::File file(path.c_str(), "rb");
		if (!file.opened())
		{
			SPDLOG_DEBUG("HeadlessScene::loadAnimation(): failed to load file: {}", path);
			return nullptr;
		}

		ozz::io::IArchive archive(&file);
		if (!archive.TestTag<ozz::animation::Animation>())
		{
			SPDLOG_DEBUG("HeadlessScene::loadAnimation(): failed to load animation instance from file: {}", path);
			return nullptr;
		}

		// Once the tag is validated ^^, reading cannot fail.
		archive >> *anim;

		ozz::animation::Animation* rawAnimPtr = anim.get();
		this->animations.emplace(name, std::move(anim));

		return rawAnimPtr;
	}

	ozz::animation::Animation* HeadlessScene::getAnimation(const std::string& name)
	{
		auto it = this->animations.find(name);
		if (it != this->animations.end())
			return it->second.get();

		SPDLOG_DEBUG("HeadlessScene::getAnimation(): attempting to get animation that does not exist: {}", name);
		return nullptr;
	}

	void HeadlessScene::removeAnimation(const std::string& name)
	{
		auto it = this->animations.find(name);
		if (it == this->animations.end())
			return;

		this->animations.erase(name);

		SPDLOG_DEBUG("HeadlessScene::removeAnimation(): removed animation {}", name);
	}

	void HeadlessScene::updateAnimators(float delta)
	{
		for (auto& s : this->stages)
			s->updateAnimators(delta);
	}

	void Scene::lerpAnimators(float alpha)
	{
		for (auto& s : this->stages)
			s->lerpAnimators(alpha);
	}

} // END NAMESPACE