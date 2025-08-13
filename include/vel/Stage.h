#pragma once

#include <vector>
#include <string>
#include <optional>
#include <memory>
#include <map>

#include "glm/glm.hpp"

#include "vel/AssetManager.h"
#include "vel/Actor.h"
#include "vel/Camera.h"
#include "vel/TextActor.h"
#include "vel/LineActor.h"
#include "vel/Billboard.h"

namespace vel
{
	class Scene; // ?

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
	
	class Stage
	{
	private:
		const std::string								name;
		AssetManager*									assetManager;

		bool											visible;
		bool											clearDepthBuffer;

		std::vector<Camera*>							cameras; // pointer managed by asset manager since multiple stages can use the same camera

		// FBO:SHADER:VAO:ACTORS - done this way to limit opengl state changes
		std::map<ActCompositeKey, std::vector<std::unique_ptr<Actor>>> actors;

		std::vector<std::unique_ptr<Armature>>			armatures;	// multiple actors can be associated with the same armature (arms, hands, gun1, gun2, etc for example)
																	// so the memory is managed by the stage vs the actor (noting this because it through me for a bit when I
																	// came back to it the last time)
		std::vector<std::unique_ptr<TextActor>>			textActors;
		std::vector<std::unique_ptr<LineActor>>			lineActors;
		std::vector<std::unique_ptr<Billboard>>			billboards;

		std::optional<std::pair<ActCompositeKey, unsigned int>>	getActorLocation(const std::string& name);
		std::optional<std::pair<ActCompositeKey, unsigned int>>	getActorLocation(const Actor* a);
		void											_removeActor(std::optional<std::pair<ActCompositeKey, unsigned int>> actorLocation);

		int												getArmatureIndex(const std::string& name);

		int												getTextActorIndex(const std::string& name);
		int												getTextActorIndex(const TextActor*);
		void											_removeTextActor(int textActorIndex);

		int												getLineActorIndex(const std::string& name);
		int												getLineActorIndex(const LineActor*);
		void											_removeLineActor(int lineActorIndex);


	public:
														Stage(const std::string& name, AssetManager* am);
														~Stage();

		const std::string&								getName() const;

		void											updateFixedArmatureAnimations(float runTime);
		void											updateArmatureAnimations(float runTime);

		void											updateTextActors();
		void											updateLineActors();

		Actor*											addActor(const std::string& name, Mesh* mesh = nullptr, Material* material = nullptr);
		Actor*											addActor(const Actor& actorIn);
		void											removeActor(const std::string& name);
		void											removeActor(const Actor* a);
		Actor*											getActor(const std::string& name);
		std::map<ActCompositeKey, std::vector<std::unique_ptr<Actor>>>& getActors();

		Armature*										addArmature(Armature* a, const std::string& defaultAnimation, const std::vector<std::string>& actors);
		Armature*										getArmature(const std::string& armatureName);

		TextActor*										addTextActor(std::unique_ptr<TextActor> ta);
		TextActor*										getTextActor(const std::string& name);
		void											removeTextActor(TextActor*);
		void											removeTextActor(const std::string& name);

		LineActor*										addLineActor(std::unique_ptr<LineActor> ta);
		LineActor*										getLineActor(const std::string& name);
		void											removeLineActor(LineActor*);
		void											removeLineActor(const std::string& name);

		

		void											addCamera(Camera* c);
		Camera*											getCamera(const std::string& name);
		std::vector<Camera*>&							getCameras();


		void											show();
		void											hide();
		const bool										isVisible();
		void											setClearDepthBuffer(bool b);
		bool											getClearDepthBuffer();
		void											updatePreviousTransforms();

		

		
		
		

		
	};
}