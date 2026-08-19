#pragma once

#include <string>
#include <unordered_map>
#include <optional>

#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/skeleton.h>

#include <vel/Render/GPU.h>
#include <vel/Scene/Stage/Actor/TextActor.h>
#include <vel/Scene/Stage/Actor/Material/Material.h>
#include <vel/Scene/Stage/Actor/Material/MaterialAnimator.h>
#include <vel/Scene/Stage/Actor/Font/FontBitmap.h>
#include <vel/Scene/Stage/Actor/Font/FontGlyphInfo.h>


namespace vel
{
	class AssetManager
	{
	private:
		std::string													dataDir;
		GPU*														gpu;
		
		std::vector<std::pair<std::unique_ptr<FontBitmap>, int>>	fontBitmaps;
		
		std::unordered_map<std::string, std::pair<std::unique_ptr<ozz::animation::Skeleton>, int>> skeletons;
		std::unordered_map<std::string, std::pair<std::unique_ptr<ozz::animation::Animation>, int>> animations;

		
		

	public:
		AssetManager(const std::string& dataDir, GPU* gpu = nullptr);
		~AssetManager();


		ozz::animation::Skeleton*	loadSkeleton(const std::string& name, const std::string& path);
		ozz::animation::Skeleton*	getSkeleton(const std::string& name);
		void						removeSkeleton(const std::string& name);
		
		ozz::animation::Animation*	loadAnimation(const std::string& name, const std::string& path);
		ozz::animation::Animation*	getAnimation(const std::string& name);
		void						removeAnimation(const std::string& name);

	};

}