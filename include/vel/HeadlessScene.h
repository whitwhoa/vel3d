#pragma once

#include <string>
#include <memory>
#include <cstdint>

#include "vel/AssetManager.h"
#include "vel/Stage.h"
#include "vel/CollisionWorld.h"


namespace vel
{
	class HeadlessScene
	{
	protected:
		uint32_t								tick;
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

		void									setTick(uint32_t t);
		uint32_t								getTick();

		void									setAssetManager(AssetManager* am);

		void									updateFixedAnimations(float runTime);
		void									stepPhysics(float delta);

		virtual void							load() = 0;
		virtual void							fixedLoop(float deltaTime) = 0;
		virtual void							postPhysics(float deltaTime) {};

		// Intended for override in derived classes where it is a requirement that a derived App
		// pass data from itself into a derived Scene. It was added when we were working on networking and needed
		// a way to pass received snapshots from a "ClientApp" into a "ClientScene", and to on the server side to
		// pass input commands from a "ServerApp" to a "ServerScene"
		virtual void							passData(void* p) {};

		// Intended for override in derived classes where it is a requirement that a derived App
		// invoke a method at it's own fixed rate, independent of the existing loop structure. It was added when we
		// were working on networking an needed a way to have the scene (which would be holding the game state)
		// send snapshots at a fixed rate, which was determined by logic in the owning derived App
		virtual void							appLoop() {};

	};
}