#pragma once

#include "spdlog/spdlog.h"

#include "vel/AssetManager.h"
#include "vel/Camera.h"
#include "vel/TextActor.h"
#include "vel/LineActor.h"
#include "vel/Billboard.h"
#include "vel/SkelAnimator.h"


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

	class Stage
	{
	private:
		const std::string								name;
		AssetManager*									assetManager;
		const uint32_t*									logicTickPtr;
		bool											visible;

		// multiple stages can use the same camera, so lifetime of cameras is managed by Scene
		std::vector<Camera*>							cameras;

		// FBO:SHADER:VAO:ACTORS - limits opengl state changes (not as bad as you might think the first time you see it)
		std::map<ActCompositeKey, std::vector<std::unique_ptr<Actor>>> actors;

		// multiple actors can be associated with the same animator (arms, hands, gun1 for example), so lifetime managed here
		std::vector<std::unique_ptr<SkelAnimator>>		animators;	

		std::vector<std::unique_ptr<TextActor>>			textActors;
		std::vector<std::unique_ptr<LineActor>>			lineActors;
		std::vector<std::unique_ptr<Billboard>>			billboards;


		std::optional<std::pair<ActCompositeKey, unsigned int>>	_getActorLocation(const std::string& name);
		std::optional<std::pair<ActCompositeKey, unsigned int>>	_getActorLocation(const Actor* a);
		void _removeActor(std::optional<std::pair<ActCompositeKey, unsigned int>> actorLocation);

		int									_getTextActorIndex(const std::string& name);
		int									_getTextActorIndex(const TextActor*);
		void								_removeTextActor(int textActorIndex);

		int									_getLineActorIndex(const std::string& name);
		int									_getLineActorIndex(const LineActor*);
		void								_removeLineActor(int lineActorIndex);

		int									_getBillboardIndex(const std::string& name);
		int									_getBillboardIndex(const Billboard* a);
		void								_removeBillboard(int billboardIndex);


	public:
		Stage::Stage(const std::string& name, AssetManager* assetManager, const uint32_t* logicTickPtr);
		~Stage();

		const std::string&	getName() const;

		void			setVisible(bool v);
		bool			getVisible() const;

		void			addCamera(Camera* c);
		Camera*			getCamera(const std::string& name);
		std::vector<Camera*>& getCameras();

		Actor*			addActor(const std::string& name, Mesh* mesh = nullptr, Material* material = nullptr);
		Actor*			addActor(const Actor& actorIn);
		void			removeActor(const std::string& name);
		void			removeActor(const Actor* a);
		Actor*			getActor(const std::string& name);
		std::map<ActCompositeKey, std::vector<std::unique_ptr<Actor>>>& getActors();

		void			addSkelAnimator(std::unique_ptr<SkelAnimator> sa);
		void			updateAnimators(float delta);
		void			lerpAnimators(float alpha);


		TextActor*		addTextActor(std::unique_ptr<TextActor> ta);
		TextActor*		getTextActor(const std::string& name);
		void			removeTextActor(TextActor*);
		void			removeTextActor(const std::string& name);
		void			updateTextActors();


		LineActor*		addLineActor(std::unique_ptr<LineActor> la);
		LineActor*		getLineActor(const std::string& name);
		void			removeLineActor(LineActor* la);
		void			removeLineActor(const std::string& name);
		

		Billboard*		addBillboard(std::unique_ptr<Billboard> b);
		Billboard*		getBillboard(const std::string& name);
		void			removeBillboard(Billboard* b);
		void			removeBillboard(const std::string& name);
		void			updateBillboards();

		void			updateTextActor(TextActor* ta);

	};
}