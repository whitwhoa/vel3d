
#include "vel/Stage.h"
#include "vel/Scene.h"

namespace vel
{
	Stage::Stage(const std::string& name, AssetManager* assetManager, const uint32_t* logicTickPtr) :
		name(name),
		assetManager(assetManager),
		logicTickPtr(logicTickPtr),
		visible(true)
	{}

	Stage::~Stage()
	{}

	void Stage::setVisible(bool v)
	{
		this->visible = v;
	}

	bool Stage::getVisible() const
	{
		return this->visible;
	}

	const std::string& Stage::getName() const
	{
		return this->name;
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

	std::optional<std::pair<ActCompositeKey, unsigned int>>	Stage::_getActorLocation(const std::string& name)
	{
		for (const auto& pair : this->actors)
		{
			unsigned int i = 0;
			for (const auto& actor : pair.second)
			{
				if (actor->getName() == name)
					return std::pair<ActCompositeKey, unsigned int>(pair.first, i);

				i++;
			}
		}

		return std::nullopt;
	}

	std::optional<std::pair<ActCompositeKey, unsigned int>>	Stage::_getActorLocation(const Actor* a)
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

	void Stage::_removeActor(std::optional<std::pair<ActCompositeKey, unsigned int>> actorLocation)
	{
		if (!actorLocation.has_value())
			return;

		auto al = actorLocation.value();

		this->actors[al.first].erase(this->actors[al.first].begin() + al.second);
	}

	void Stage::removeActor(const Actor* a)
	{
		this->_removeActor(this->_getActorLocation(a));
	}

	void Stage::removeActor(const std::string& name)
	{
		this->_removeActor(this->_getActorLocation(name));
	}

	Actor* Stage::addActor(const Actor& actorIn)
	{
		// ogl uses 0 to indicate error, so we'll never have an index of 0, so we use that for empty
		unsigned int fboToUse = actorIn.getMaterial()->getHasAlphaChannel() ? 2 : 1; // always either opaque or has alpha
		unsigned int shaderProgramId = actorIn.getMaterial()->getShader() == nullptr ? 0 : actorIn.getMaterial()->getShader()->id;
		unsigned int vaoToUse = !actorIn.getMesh()->getGpuMesh().has_value() ? 0 : actorIn.getMesh()->getGpuMesh()->VAO;

		std::unique_ptr<Actor> a = std::make_unique<Actor>(actorIn);

		if(!a->getUpdateTick())
			a->setUpdateTick(this->logicTickPtr);

		Actor* ptrA = a.get(); // save raw pointer for return after move

		ActCompositeKey key = { fboToUse, shaderProgramId, vaoToUse };

		this->actors[key].push_back(std::move(a));

		return ptrA;
	}

	Actor* Stage::addActor(const std::string& name, Mesh* mesh, Material* material)
	{
		// ogl uses 0 to indicate error, so we'll never have an index of 0, so we use that for empty
		unsigned int fboToUse = 1; // always either opaque or has alpha
		if (material != nullptr)
			fboToUse = material->getHasAlphaChannel() ? 2 : 1;

		unsigned int shaderProgramId = material == nullptr ? 0 : material->getShader()->id;
		unsigned int vaoToUse = mesh == nullptr ? 0 : mesh->getGpuMesh()->VAO;

		std::unique_ptr<Actor> a = std::make_unique<Actor>(name);
		a->setMesh(mesh);

		if (material) // actor default to EmptyMaterial if none provided
			a->setMaterial(material);

		a->setUpdateTick(this->logicTickPtr);

		Actor* ptrA = a.get(); // save raw pointer for return after move

		ActCompositeKey key = { fboToUse, shaderProgramId, vaoToUse };

		this->actors[key].push_back(std::move(a));

		return ptrA;
	}

	Actor* Stage::getActor(const std::string& name)
	{
		for (const auto& pair : this->actors)
			for (const auto& actor : pair.second)
				if (actor->getName() == name)
					return actor.get();

		return nullptr;
	}

	std::map<ActCompositeKey, std::vector<std::unique_ptr<Actor>>>& Stage::getActors()
	{
		return this->actors;
	}

	void Stage::addSkelAnimator(std::unique_ptr<SkelAnimator> sa)
	{
		this->animators.push_back(std::move(sa));
	}

	void Stage::updateAnimators(float delta)
	{
		for (auto& a : this->animators)
			a->update(delta);
	}

	void Stage::lerpAnimators(float alpha)
	{
		for (auto& a : this->animators)
			a->renderLerp(alpha);
	}


	//
	// LineActors
	//
	int Stage::_getLineActorIndex(const std::string& name)
	{
		for (int i = 0; i < this->lineActors.size(); i++)
			if (this->lineActors.at(i)->name == name)
				return i;

		return -1;
	}

	int Stage::_getLineActorIndex(const LineActor* a)
	{
		for (int i = 0; i < this->lineActors.size(); i++)
			if (this->lineActors.at(i).get() == a)
				return i;

		return -1;
	}

	void Stage::_removeLineActor(int lineActorIndex)
	{
		LineActor* la = this->lineActors.at(lineActorIndex).get();

		this->_removeActor(this->_getActorLocation(la->actor));

		this->lineActors.erase(this->lineActors.begin() + lineActorIndex);
	}

	LineActor* Stage::addLineActor(std::unique_ptr<LineActor> la)
	{
		this->lineActors.push_back(std::move(la));
		return this->lineActors.back().get();
	}

	LineActor* Stage::getLineActor(const std::string& name)
	{
		return this->lineActors.at(this->_getLineActorIndex(name)).get();
	}

	void Stage::removeLineActor(LineActor* la)
	{
		this->_removeLineActor(this->_getLineActorIndex(la));
	}

	void Stage::removeLineActor(const std::string& name)
	{
		this->_removeLineActor(this->_getLineActorIndex(name));
	}



	//
	// TextActors
	//

	int Stage::_getTextActorIndex(const std::string& name)
	{
		for (int i = 0; i < this->textActors.size(); i++)
			if (this->textActors.at(i)->name == name)
				return i;

		return -1;
	}

	int Stage::_getTextActorIndex(const TextActor* a)
	{
		for (int i = 0; i < this->textActors.size(); i++)
			if (this->textActors.at(i).get() == a)
				return i;

		return -1;
	}

	void Stage::_removeTextActor(int textActorIndex)
	{
		TextActor* ta = this->textActors.at(textActorIndex).get();

		this->_removeActor(this->_getActorLocation(ta->actor));

		this->textActors.erase(this->textActors.begin() + textActorIndex);
	}

	TextActor* Stage::addTextActor(std::unique_ptr<TextActor> ta)
	{
		this->textActors.push_back(std::move(ta));
		return this->textActors.back().get();
	}

	TextActor* Stage::getTextActor(const std::string& name)
	{
		return this->textActors.at(this->_getTextActorIndex(name)).get();
	}

	void Stage::removeTextActor(TextActor* ta)
	{
		this->_removeTextActor(this->_getTextActorIndex(ta));
	}

	void Stage::removeTextActor(const std::string& name)
	{
		this->_removeTextActor(this->_getTextActorIndex(name));
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


	//
	// Billboards
	//

	int Stage::_getBillboardIndex(const std::string& name)
	{
		for (int i = 0; i < this->billboards.size(); i++)
			if (this->billboards.at(i)->getActor()->getName() == name)
				return i;

		return -1;
	}

	int Stage::_getBillboardIndex(const Billboard* a)
	{
		for (int i = 0; i < this->billboards.size(); i++)
			if (this->billboards.at(i).get() == a)
				return i;

		return -1;
	}

	void Stage::_removeBillboard(int billboardIndex)
	{
		Billboard* b = this->billboards.at(billboardIndex).get();

		this->_removeActor(this->_getActorLocation(b->getActor()));

		this->billboards.erase(this->billboards.begin() + billboardIndex);
	}

	Billboard* Stage::addBillboard(std::unique_ptr<Billboard> b)
	{
		this->billboards.push_back(std::move(b));
		return this->billboards.back().get();
	}

	Billboard* Stage::getBillboard(const std::string& name)
	{
		return this->billboards.at(this->_getBillboardIndex(name)).get();
	}

	void Stage::removeBillboard(Billboard* b)
	{
		this->_removeBillboard(this->_getBillboardIndex(b));
	}

	void Stage::removeBillboard(const std::string& name)
	{
		this->_removeBillboard(this->_getBillboardIndex(name));
	}

	void Stage::updateBillboards()
	{
		for (auto& b : this->billboards)
			b->update();
	}


}