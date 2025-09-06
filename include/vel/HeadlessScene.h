#pragma once

#include <string>
#include <memory>

#include "vel/AssetManager.h"
#include "vel/Stage.h"
#include "vel/CollisionWorld.h"


namespace vel
{
	class HeadlessScene
	{
	protected:
		std::string								dataDir;
		std::string								name = "";
		AssetManager*							assetManager;
		std::vector<std::unique_ptr<Stage>>		stages;
		std::vector<CollisionWorld*> 			collisionWorlds;
		float									fixedAnimationTime;

		std::vector<Mesh*>						meshesInUse;
		std::vector<Armature*>					armaturesInUse;

		int										getCollisionWorldIndex(const std::string& name);

		bool									loadMesh(const std::string& path);
		Mesh*									getMesh(const std::string& name);

		Armature*								getArmature(const std::string& name);

		Stage*									addStage(const std::string& name);
		Stage*									getStage(const std::string& name);

		CollisionWorld*							addCollisionWorld(const std::string& name, float gravity = -10.0f);
		CollisionWorld*							getCollisionWorld(const std::string& name);

	public:
		HeadlessScene(const std::string& dataDir);
		~HeadlessScene();

		void									setName(const std::string& n);
		const std::string&						getName();

		void									setAssetManager(AssetManager* am);

		void									updateFixedAnimations(float runTime);
		void									stepPhysics(float delta);

		virtual void							load() = 0;
		virtual void							fixedLoop(float deltaTime) = 0;
		virtual void							postPhysics(float deltaTime) {};

	};
}