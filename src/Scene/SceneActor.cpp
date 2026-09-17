#include <spdlog/spdlog.h>

#include <vel/Runtime.h>
#include <vel/Util/functions.h>
#include <vel/Scene/Scene.h>


namespace vel
{
	void HeadlessScene::setActorParent(actor_handle child, actor_handle parent)
	{
		this->actors[child].parentActor = parent;
		this->actors[parent].childActors.push_back(child);
	}

	void HeadlessScene::clearActorParent(actor_handle child)
	{
		this->actors[child].parentActor = actor_handle{};

		auto& childActors = this->actors[this->actors[child].parentActor].childActors;
		auto it = std::find(childActors.begin(), childActors.end(), child);
		if (it != childActors.end())
			childActors.erase(it);
	}

	void HeadlessScene::setActorParentBone(actor_handle child, actor_handle parent, int32_t parentBoneId)
	{
		Actor& c = this->actors[child];
		c.parentActor = parent;
		c.parentActorBone = parentBoneId;

		this->actors[parent].childActors.push_back(child);
	}

	void HeadlessScene::clearActorParentBone(actor_handle child)
	{
		Actor& c = this->actors[child];
		c.parentActor = actor_handle{};
		c.parentActorBone = -1;

		auto& childActors = this->actors[this->actors[child].parentActor].childActors;
		auto it = std::find(childActors.begin(), childActors.end(), child);
		if (it != childActors.end())
			childActors.erase(it);
	}

	glm::mat4 HeadlessScene::getActorWorldMatrix(actor_handle h)
	{
		Actor& a = this->actors[h];

		// if this actor has no parent, simply return the matrix of it's transform
		if (!a.parentActor)
			return a.getTransform().getMatrix();

		// if this actor is parented to another actor, and not to that actor's bone
		if (a.parentActorBone == -1)
			return this->getActorWorldMatrix(a.parentActor) * a.getTransform().getMatrix();

		// if this actor is parented to the bone of its parent actor
		return this->getActorWorldMatrix(a.parentActor) *
			ozzFloat4x4ToGlmMat4(this->actors[a.parentActor].animator->getSimBoneMatrix(a.parentActorBone)) *
			a.getTransform().getMatrix();
	}

	glm::mat4 Scene::getActorWorldRenderMatrix(actor_handle h, float alpha)
	{
		Actor& a = this->actors[h];

		// actor is not dynamic (does not move) so interpolation is not required, simply return it's world matrix
		if (!(a.flags & ACTFLG_DYNAMIC) || !(a.flags & ACTFLG_LERPABLE))
			return this->getActorWorldMatrix(h);

		glm::mat4 selfMat = Transform::interpolateTransforms(a.getPreviousTransform(), a.getTransform(), alpha);

		// if this actor has no parent, simply return the matrix of it's transform
		if (!a.parentActor)
			return selfMat;

		// if this actor is parented to another actor
		if (a.parentActorBone == -1)
			return this->getActorWorldRenderMatrix(a.parentActor, alpha) * selfMat;

		// if we made it here, we know that this actor is parented to a bone of its parent actor
		return this->getActorWorldRenderMatrix(a.parentActor, alpha) *
			ozzFloat4x4ToGlmMat4(this->actors[a.parentActor].animator->getRenderBoneMatrix(a.parentActorBone)) *
			selfMat;
	}

	void Scene::hideActor(actor_handle h)
	{
		Actor& a = this->actors[h];
		a.flags &= ~ACTFLG_VISIBLE;

		for (auto& h2 : a.childActors)
			this->hideActor(h2);
	}

	void Scene::showActor(actor_handle h)
	{
		Actor& a = this->actors[h];
		a.flags |= ACTFLG_VISIBLE;

		for (auto& h2 : a.childActors)
			this->showActor(h2);
	}

	actor_handle HeadlessScene::addActor(Mesh* mesh)
	{
		Actor a{};
		a.mesh = mesh;
		return this->actors.insert(a);
	}

	actor_handle Scene::addActor(Stage* stage, Mesh* mesh, uint32_t flags)
	{
		Actor a{};
		a.mesh = mesh;
		a.flags = flags;



		return this->actors.insert(a);
	}



} // END NAMESPACE