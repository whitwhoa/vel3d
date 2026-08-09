#pragma once

#include <string>
#include <memory>
#include <cstdint>

#include <ozz/animation/runtime/skeleton.h>
#include <ozz/animation//runtime/animation.h>

#include <vel/Scene/Stage/Stage.h>
#include <vel/Physics/CollisionWorld.h>


namespace vel
{
	class Stage;

	class HeadlessScene
	{
	protected:
		std::string								name = "";
		std::vector<std::unique_ptr<Stage>>		stages;
		std::vector<CollisionWorld*> 			collisionWorlds;
		std::vector<Mesh*>						meshesInUse;
		std::vector<std::string>				skeletonsInUse;
		std::vector<std::string>				animationsInUse;


		int										getCollisionWorldIndex(const std::string& name);

		bool									loadMesh(const std::string& path);
		Mesh*									getMesh(const std::string& name);

		ozz::animation::Skeleton*				loadSkeleton(const std::string& name, const std::string& path);
		ozz::animation::Skeleton*				getSkeleton(const std::string& name);

		ozz::animation::Animation*				loadAnimation(const std::string& name, const std::string& path);
		ozz::animation::Animation*				getAnimation(const std::string& name);

		CollisionWorld*							addCollisionWorld(const std::string& name, float gravity = -10.0f);
		CollisionWorld*							getCollisionWorld(const std::string& name);


	public:
		HeadlessScene();
		~HeadlessScene();

		virtual bool							internalLoad();
		virtual bool							load() = 0;
		virtual void							internalFixedLoop(float deltaTime);
		virtual void							fixedLoop(float deltaTime) = 0;

		void									setName(const std::string& n);
		const std::string&						getName();

		void									stepPhysics(float delta);

		void									updateAnimators(float delta);

		Stage*									addStage(const std::string& name, int pos = -1);
		Stage*									getStage(const std::string& name);

		
	};
}