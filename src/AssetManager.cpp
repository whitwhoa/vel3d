#include <thread> 
#include <chrono>
#include <sstream>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <memory>

#include <glad/gl.h>

#include <ozz/base/io/archive.h>
#include <ozz/base/io/stream.h>

#include <spdlog/spdlog.h>

#include <vel/AssetManager.h>
#include <vel/Util/functions.h>


using namespace std::chrono_literals;

namespace vel
{

	AssetManager::AssetManager(const std::string& dataDir, GPU* gpu) :
		dataDir(dataDir),
		gpu(gpu)
	{}
	AssetManager::~AssetManager(){}


	/***********************************************************************************************
	* SKELETONS
	************************************************************************************************/
	ozz::animation::Skeleton* AssetManager::loadSkeleton(const std::string& name, const std::string& path)
	{
		auto it = this->skeletons.find(name);
		if (it != this->skeletons.end()) 
		{
			SPDLOG_DEBUG("Existing Skeleton, bypass reload: {}", name);

			it->second.second++;

			return it->second.first.get();
		}
		
		SPDLOG_DEBUG("Load new Skeleton: {}", name);

		std::unique_ptr<ozz::animation::Skeleton> skel = std::make_unique<ozz::animation::Skeleton>();
		
		ozz::io::File file(path.c_str(), "rb");
		if (!file.opened())
		{
			SPDLOG_DEBUG("AssetManager::loadSkeleton(): failed to load file: {}", path);
			return nullptr;
		}

		ozz::io::IArchive archive(&file);
		if (!archive.TestTag<ozz::animation::Skeleton>())
		{
			SPDLOG_DEBUG("AssetManager::loadSkeleton(): failed to load skeleton instance from file: {}", path);
			return nullptr;
		}

		// Once the tag is validated ^^, reading cannot fail.
		archive >> *skel;
		
		ozz::animation::Skeleton* rawSkelPtr = skel.get();
		this->skeletons[name] = std::pair<std::unique_ptr<ozz::animation::Skeleton>, int>(std::move(skel), 1);

		return rawSkelPtr;
	}

	ozz::animation::Skeleton* AssetManager::getSkeleton(const std::string& name)
	{
		auto it = this->skeletons.find(name);
		if (it != this->skeletons.end())
			return it->second.first.get();
		
		SPDLOG_DEBUG("AssetManager::getSkeleton attempting to get skeleton that does not exist: {}", name);
		return nullptr;
	}

	void AssetManager::removeSkeleton(const std::string& name)
	{
		auto it = this->skeletons.find(name);
		if (it == this->skeletons.end())
			return;

		it->second.second--;

		if (it->second.second == 0)
		{
			SPDLOG_DEBUG("Full remove Skeleton: {}", name);
			this->skeletons.erase(name);
			return;
		}

		SPDLOG_DEBUG("Decrement Skeleton usageCount, retain: {}", name);
	}

	/***********************************************************************************************
	* ANIMATIONS
	************************************************************************************************/
	ozz::animation::Animation* AssetManager::loadAnimation(const std::string& name, const std::string& path)
	{
		auto it = this->animations.find(name);
		if (it != this->animations.end())
		{
			SPDLOG_DEBUG("Existing Animation, bypass reload: {}", name);

			it->second.second++;

			return it->second.first.get();
		}

		SPDLOG_DEBUG("Load new Animation: {}", name);

		std::unique_ptr<ozz::animation::Animation> anim = std::make_unique<ozz::animation::Animation>();

		ozz::io::File file(path.c_str(), "rb");
		if (!file.opened())
		{
			SPDLOG_DEBUG("AssetManager::loadAnimation(): failed to load file: {}", path);
			return nullptr;
		}

		ozz::io::IArchive archive(&file);
		if (!archive.TestTag<ozz::animation::Animation>())
		{
			SPDLOG_DEBUG("AssetManager::loadAnimation(): failed to load animation instance from file: {}", path);
			return nullptr;
		}

		// Once the tag is validated ^^, reading cannot fail.
		archive >> *anim;

		ozz::animation::Animation* rawAnimPtr = anim.get();
		this->animations[name] = std::pair<std::unique_ptr<ozz::animation::Animation>, int>(std::move(anim), 1);

		return rawAnimPtr;
	}

	ozz::animation::Animation* AssetManager::getAnimation(const std::string& name)
	{
		auto it = this->animations.find(name);
		if (it != this->animations.end())
			return it->second.first.get();

		SPDLOG_DEBUG("AssetManager::getAnimation attempting to get animation that does not exist: {}", name);
		return nullptr;
	}

	void AssetManager::removeAnimation(const std::string& name)
	{
		auto it = this->animations.find(name);
		if (it == this->animations.end())
			return;

		it->second.second--;

		if (it->second.second == 0)
		{
			SPDLOG_DEBUG("Full remove Animation: {}", name);
			this->animations.erase(name);
			return;
		}

		SPDLOG_DEBUG("Decrement Animation usageCount, retain: {}", name);
	}

	

	
/* END OF CLASS
******************************************************************/
}