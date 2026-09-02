#pragma once

#include <string>
#include <memory>
#include <cstdint>

#include <ozz/animation/runtime/skeleton.h>
#include <ozz/animation//runtime/animation.h>

#include <vel/Scene/Stage/Stage.h>
#include <vel/Scene/Mesh/MeshLoaderInterface.h>
#include <vel/Scene/Mesh/MeshFlag.h>
#include <vel/Scene/GeoPool/GeoPool.h>
#include <vel/Scene/CollisionWorld/CollisionWorld.h>
#include <vel/Scene/Animation/SkelAnimator.h>


namespace vel
{
	class Stage;

	class HeadlessScene
	{
	protected:
		static unsigned int nextSceneId;

		unsigned int							id;
		std::unique_ptr<MeshLoaderInterface>	meshLoader;

		std::vector<CollisionWorld*> 			collisionWorlds;

		std::unordered_map<std::string, std::unique_ptr<GeoPool>> soloGeoPools;

		std::unordered_map<std::string, std::unique_ptr<Mesh>>						meshes;
		std::unordered_map<std::string, std::unique_ptr<ozz::animation::Skeleton>>	skeletons;
		std::unordered_map<std::string, std::unique_ptr<ozz::animation::Animation>>	animations;

		//
		// TODO: Actors go here...in a slot_map
		//

		std::vector<std::unique_ptr<SkelAnimator>> animators;


		int										getCollisionWorldIndex(const std::string& name);

		virtual std::vector<Mesh*>				loadMesh(const std::string& path, uint32_t meshFlags = MESHFLAG_NONE);
		virtual Mesh*							addMesh(std::unique_ptr<Mesh> m);
		Mesh*									getMesh(const std::string& name);
		virtual void							removeMesh(Mesh* pMesh);

		ozz::animation::Skeleton*				loadSkeleton(const std::string& name, const std::string& path);
		ozz::animation::Skeleton*				getSkeleton(const std::string& name);
		void									removeSkeleton(const std::string& name);

		ozz::animation::Animation*				loadAnimation(const std::string& name, const std::string& path);
		ozz::animation::Animation*				getAnimation(const std::string& name);
		void									removeAnimation(const std::string& name);

		CollisionWorld*							addCollisionWorld(const std::string& name, float gravity = -10.0f);
		CollisionWorld*							getCollisionWorld(const std::string& name);


	public:
		HeadlessScene();
		~HeadlessScene();

		virtual bool							internalLoad();
		virtual bool							load() = 0;
		virtual void							internalFixedLoop(float deltaTime);
		virtual void							fixedLoop(float deltaTime) = 0;

		void									stepPhysics(float delta);

		void									updateAnimators(float delta);

		Stage*									addStage(const std::string& name, int pos = -1);
		Stage*									getStage(const std::string& name);

		unsigned int							getId();
		
	};
}