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

	std::unordered_map<unsigned int, std::unordered_map<unsigned int, std::vector<std::unique_ptr<Actor>>>>& Stage::getActors()
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
		for (auto& shaderBucket : this->actors)
		{
			for (auto& vaoBucket : shaderBucket.second)
			{
				for (unsigned int i = 0; i < vaoBucket.second.size(); i++)
				{
					if (vaoBucket.second.at(i)->isDynamic())
					{
						vaoBucket.second.at(i)->updatePreviousTransform();
					}
				}
			}
		}
	}

	Actor* Stage::addActor(const std::string& name, Mesh* mesh, Material* material)
	{
		// ogl uses 0 to indicate error, so we'll never have an index of 0, so we use that for empty
		unsigned int shaderProgramId = material == nullptr ? 0 : material->getShader()->id;
		unsigned int vaoToUse = mesh == nullptr ? 0 : mesh->getGpuMesh()->VAO;

		std::unique_ptr<Actor> a = std::make_unique<Actor>(name);
		a->setMesh(mesh);

		if(material) // actor default to EmptyMaterial if none provided
			a->setMaterial(material);

		Actor* ptrA = a.get(); // save raw pointer for return after move

		this->actors[shaderProgramId][vaoToUse].push_back(std::move(a));

		return ptrA;
	}

	std::optional<std::vector<unsigned int>> Stage::getActorIndex(const std::string& name)
	{
		for (auto& shaderBucket : this->actors)
		{
			for (auto& vaoBucket : shaderBucket.second)
			{
				for (unsigned int i = 0; i < vaoBucket.second.size(); i++)
				{
					if (vaoBucket.second.at(i)->getName() == name)
					{
						return std::vector<unsigned int>{ shaderBucket.first, vaoBucket.first, i };
					}
				}
			}
		}

		return std::nullopt;
	}

	std::optional<std::vector<unsigned int>> Stage::getActorIndex(const Actor* a)
	{
		for (auto& shaderBucket : this->actors)
		{
			for (auto& vaoBucket : shaderBucket.second)
			{
				for (unsigned int i = 0; i < vaoBucket.second.size(); i++)
				{
					if (vaoBucket.second.at(i).get() == a)
					{
						return std::vector<unsigned int>{ shaderBucket.first, vaoBucket.first, i };
					}
				}
			}
		}

		return std::nullopt;
	}

	Actor* Stage::getActor(const std::string& name)
	{
		std::optional<std::vector<unsigned int>> actorIndex = this->getActorIndex(name);

		if (!actorIndex.has_value())
			return nullptr;

		auto ai = actorIndex.value();

		return this->actors[ai[0]][ai[1]].at(ai[2]).get();
	}

	void Stage::_removeActor(std::optional<std::vector<unsigned int>> actorIndex)
	{		
		if (!actorIndex.has_value())
			return;

		auto ai = actorIndex.value();

		this->actors[ai[0]][ai[1]].erase(this->actors[ai[0]][ai[1]].begin() + ai[2]);
	}

	void Stage::removeActor(const Actor* a)
	{
		this->_removeActor(this->getActorIndex(a));
	}

	void Stage::removeActor(const std::string& name)
	{
		this->_removeActor(this->getActorIndex(name));
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

		this->_removeActor(this->getActorIndex(ta->actor));

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

}