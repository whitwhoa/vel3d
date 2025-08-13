#include <iostream>


#include "vel/App.h"
#include "vel/Stage.h"



namespace vel
{

	Stage::Stage(const std::string& name, AssetManager* am) :
		name(name),
		assetManager(am),
		visible(true),
		clearDepthBuffer(false)
	{}

	Stage::~Stage()
	{
		
	}

	const std::string& Stage::getName() const
	{
		return this->name;
	}

	int Stage::getArmatureIndex(const std::string& name)
	{
		for (int i = 0; i < this->armatures.size(); i++)
			if (this->armatures.at(i)->getName() == name)
				return i;

		return -1;
	}

	Armature* Stage::getArmature(const std::string& armatureName)
	{
		return this->armatures.at(this->getArmatureIndex(armatureName)).get();
	}

	Armature* Stage::addArmature(Armature* a, const std::string& defaultAnimation, const std::vector<std::string>& actorsIn)
	{
		this->armatures.push_back(std::make_unique<Armature>(*a));

		Armature* sa = this->armatures.back().get();

		sa->playAnimation(defaultAnimation);

		for (auto& actorName : actorsIn)
		{
			auto act = this->getActor(actorName);
			act->setArmature(sa);

			std::vector<std::pair<size_t, unsigned int>> activeBones;
			unsigned int index = 0;
			for (auto& meshBone : act->getMesh()->getBones())
			{
				// associate the index of the armature bone with the index of the mesh bone used for transformation
				activeBones.push_back(std::pair<size_t, unsigned int>(act->getArmature()->getBoneIndex(meshBone.name), index));
				index++;
			}

			act->setActiveBones(activeBones);
		}

		return sa;
	}

	bool Stage::getClearDepthBuffer()
	{
		return this->clearDepthBuffer;
	}

	void Stage::setClearDepthBuffer(bool b)
	{
		this->clearDepthBuffer = b;
	}

	std::map<ActCompositeKey, std::vector<std::unique_ptr<Actor>>>& Stage::getActors()
	{
		return this->actors;
	}

	const bool Stage::isVisible()
	{
		return this->visible;
	}

	void Stage::hide()
	{
		this->visible = false;
	}

	void Stage::show()
	{
		this->visible = true;
	}

	void Stage::addCamera(Camera* c)
	{
		this->cameras.push_back(c);
	}

	Camera* Stage::getCamera(const std::string& name)
	{
		for (auto c : this->cameras)
			if (c->getName() == name)
				return c;

		return nullptr;
	}

	std::vector<Camera*>& Stage::getCameras()
	{
		return this->cameras;
	}

	void Stage::updateFixedArmatureAnimations(float runTime)
	{
		for (auto& a : this->armatures)
			if(a->getShouldInterpolate())
				a->updateAnimation(runTime);
	}

	void Stage::updateArmatureAnimations(float runTime)
	{
		for (auto& a : this->armatures)
			if (!a->getShouldInterpolate())
				a->updateAnimation(runTime);
	}

	void Stage::updatePreviousTransforms()
	{
		// OMG we loop through all actors twice 0_0 once here then once in the render loop...wow,
		// well...I guess if it ain't broke...
		//
		// !TODO: There has to be a better way to do this...
		for (auto& pair : this->actors) 
			for (auto& actor : pair.second) 
				actor->updatePreviousTransform();
	}

	Actor* Stage::addActor(const Actor& actorIn)
	{
		// ogl uses 0 to indicate error, so we'll never have an index of 0, so we use that for empty
		unsigned int fboToUse = actorIn.getMaterial()->getHasAlphaChannel() ? 2 : 1; // always either opaque or has alpha
		unsigned int shaderProgramId = actorIn.getMaterial()->getShader() == nullptr ? 0 : actorIn.getMaterial()->getShader()->id;
		unsigned int vaoToUse = !actorIn.getMesh()->getGpuMesh().has_value() ? 0 : actorIn.getMesh()->getGpuMesh()->VAO;

		std::unique_ptr<Actor> a = std::make_unique<Actor>(actorIn);

		Actor* ptrA = a.get(); // save raw pointer for return after move

		ActCompositeKey key = {fboToUse, shaderProgramId, vaoToUse};

		this->actors[key].push_back(std::move(a));

		return ptrA;
	}

	Actor* Stage::addActor(const std::string& name, Mesh* mesh, Material* material)
	{
		// ogl uses 0 to indicate error, so we'll never have an index of 0, so we use that for empty
		unsigned int fboToUse = 1; // always either opaque or has alpha
		if(material != nullptr)
			fboToUse = material->getHasAlphaChannel() ? 2 : 1;

		unsigned int shaderProgramId = material == nullptr ? 0 : material->getShader()->id;
		unsigned int vaoToUse = mesh == nullptr ? 0 : mesh->getGpuMesh()->VAO;

		std::unique_ptr<Actor> a = std::make_unique<Actor>(name);
		a->setMesh(mesh);

		if(material) // actor default to EmptyMaterial if none provided
			a->setMaterial(material);

		Actor* ptrA = a.get(); // save raw pointer for return after move

		ActCompositeKey key = { fboToUse, shaderProgramId, vaoToUse };

		this->actors[key].push_back(std::move(a));

		return ptrA;
	}

	std::optional<std::pair<ActCompositeKey, unsigned int>>	Stage::getActorLocation(const std::string& name)
	{
		for (const auto& pair : this->actors)
		{
			unsigned int i = 0;
			for (const auto& actor : pair.second)
			{
				if (actor->getName() == name)
				{
					return std::pair<ActCompositeKey, unsigned int>(pair.first, i);
				}

				i++;
			}
		}

		return std::nullopt;
	}

	std::optional<std::pair<ActCompositeKey, unsigned int>>	Stage::getActorLocation(const Actor* a)
	{
		for (const auto& pair : this->actors)
		{
			unsigned int i = 0;
			for (const auto& actor : pair.second)
			{
				if (actor.get() == a)
				{
					return std::pair<ActCompositeKey, unsigned int>(pair.first, i);
				}

				i++;
			}
		}

		return std::nullopt;
	}

	Actor* Stage::getActor(const std::string& name)
	{
		for (const auto& pair : this->actors) 
			for (const auto& actor : pair.second) 
				if (actor->getName() == name)
					return actor.get();

		return nullptr;
	}

	void Stage::_removeActor(std::optional<std::pair<ActCompositeKey, unsigned int>> actorLocation)
	{		
		if (!actorLocation.has_value())
			return;

		auto al = actorLocation.value();

		this->actors[al.first].erase(this->actors[al.first].begin() + al.second);
	}

	void Stage::removeActor(const Actor* a)
	{
		this->_removeActor(this->getActorLocation(a));
	}

	void Stage::removeActor(const std::string& name)
	{
		this->_removeActor(this->getActorLocation(name));
	}

	int Stage::getTextActorIndex(const std::string& name)
	{
		for (int i = 0; i < this->textActors.size(); i++)
			if (this->textActors.at(i)->name == name)
				return i;

		return -1;
	}

	int Stage::getTextActorIndex(const TextActor* a)
	{
		for (int i = 0; i < this->textActors.size(); i++)
			if (this->textActors.at(i).get() == a)
				return i;

		return -1;
	}

	TextActor* Stage::addTextActor(std::unique_ptr<TextActor> ta)
	{
		this->textActors.push_back(std::move(ta));
		return this->textActors.back().get();
	}

	TextActor* Stage::getTextActor(const std::string& name)
	{
		return this->textActors.at(this->getTextActorIndex(name)).get();
	}

	void Stage::_removeTextActor(int textActorIndex)
	{
		TextActor* ta = this->textActors.at(textActorIndex).get();

		this->_removeActor(this->getActorLocation(ta->actor));

		this->textActors.erase(this->textActors.begin() + textActorIndex);
	}

	void Stage::removeTextActor(TextActor* ta)
	{
		this->_removeTextActor(this->getTextActorIndex(ta));
	}

	void Stage::removeTextActor(const std::string& name)
	{
		this->_removeTextActor(this->getTextActorIndex(name));
	}

	void Stage::updateTextActors()
	{
		for (auto& ta : this->textActors)
		{
			if (ta->requiresUpdate)
			{
				// update the mesh data associated with text actor
				std::unique_ptr<Mesh> updatedMesh = std::move(this->assetManager->loadTextActorMesh(ta.get()));
				ta->actor->getMesh()->setVertices(updatedMesh->getVertices());
				ta->actor->getMesh()->setIndices(updatedMesh->getIndices());

				this->assetManager->updateMesh(ta->actor->getMesh());
				ta->requiresUpdate = false;
			}
		}
	}

	///////////

	int Stage::getLineActorIndex(const std::string& name)
	{
		for (int i = 0; i < this->lineActors.size(); i++)
			if (this->lineActors.at(i)->name == name)
				return i;

		return -1;
	}

	int Stage::getLineActorIndex(const LineActor* a)
	{
		for (int i = 0; i < this->lineActors.size(); i++)
			if (this->lineActors.at(i).get() == a)
				return i;

		return -1;
	}

	LineActor* Stage::addLineActor(std::unique_ptr<LineActor> la)
	{
		this->lineActors.push_back(std::move(la));
		return this->lineActors.back().get();
	}

	LineActor* Stage::getLineActor(const std::string& name)
	{
		return this->lineActors.at(this->getLineActorIndex(name)).get();
	}

	void Stage::_removeLineActor(int lineActorIndex)
	{
		LineActor* la = this->lineActors.at(lineActorIndex).get();

		this->_removeActor(this->getActorLocation(la->actor));

		this->lineActors.erase(this->lineActors.begin() + lineActorIndex);
	}

	void Stage::removeLineActor(LineActor* la)
	{
		this->_removeLineActor(this->getLineActorIndex(la));
	}

	void Stage::removeLineActor(const std::string& name)
	{
		this->_removeLineActor(this->getLineActorIndex(name));
	}

	void Stage::updateLineActors()
	{
		// TODO
	}

}