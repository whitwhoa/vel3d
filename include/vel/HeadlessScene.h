#pragma once

#include <string>
#include <memory>
#include <cstdint>

#include "ozz/animation/runtime/skeleton.h"
#include "ozz/animation//runtime/animation.h"

#include "vel/AssetManager.h"
#include "vel/CollisionWorld.h"


namespace vel
{
	struct ActCompositeKey
	{
		unsigned int fbo;
		unsigned int shader;
		unsigned int vao;

		bool operator<(const ActCompositeKey& other) const
		{
			return std::tie(fbo, shader, vao) < std::tie(other.fbo, other.shader, other.vao);
		}
	};

	class HeadlessScene
	{
	protected:
		uint32_t								tick;
		std::string								dataDir;
		std::string								name = "";
		AssetManager*							assetManager;
		std::vector<CollisionWorld*> 			collisionWorlds;

		// FBO:SHADER:VAO:ACTORS - done this way to limit opengl state changes (not as bad as you might think the first time you see it)
		std::map<ActCompositeKey, std::vector<std::unique_ptr<Actor>>> actors;

		// ultiple actors can be associated with the same animator (arms, hands, gun1 for example) so the memory is managed at Scene level
		std::vector<std::unique_ptr<SkelAnimator>> animators;

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

		std::optional<std::pair<ActCompositeKey, unsigned int>>	getActorLocation(const std::string& name);
		std::optional<std::pair<ActCompositeKey, unsigned int>>	getActorLocation(const Actor* a);
		void _removeActor(std::optional<std::pair<ActCompositeKey, unsigned int>> actorLocation);

		Actor*									addActor(const std::string& name, Mesh* mesh = nullptr, Material* material = nullptr);
		Actor*									addActor(const Actor& actorIn);
		void									removeActor(const std::string& name);
		void									removeActor(const Actor* a);
		Actor*									getActor(const std::string& name);

	public:
		HeadlessScene(const std::string& dataDir);
		~HeadlessScene();

		virtual bool							load() = 0;
		virtual void							fixedLoop(float deltaTime) = 0;


		void									setName(const std::string& n);
		const std::string&						getName();

		void									setTick(uint32_t t);
		uint32_t								getTick();

		void									setAssetManager(AssetManager* am);

		std::map<ActCompositeKey, std::vector<std::unique_ptr<Actor>>>& getActors();

		void									addSkelAnimator(std::unique_ptr<SkelAnimator> sa);
		void									updateAnimators(float delta);

		void									stepPhysics(float delta);

		
	};
}