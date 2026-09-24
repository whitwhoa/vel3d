#include <spdlog/spdlog.h>

#include <vel/Runtime.h>
#include <vel/Util/functions.h>
#include <vel/Util/Assert.h>
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
		auto& childActors = this->actors[this->actors[child].parentActor].childActors;
		auto it = std::find(childActors.begin(), childActors.end(), child);
		if (it != childActors.end())
			childActors.erase(it);

		this->actors[child].parentActor = actor_handle{};
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
		auto& childActors = this->actors[this->actors[child].parentActor].childActors;
		auto it = std::find(childActors.begin(), childActors.end(), child);
		if (it != childActors.end())
			childActors.erase(it);

		Actor& c = this->actors[child];
		c.parentActor = actor_handle{};
		c.parentActorBone = -1;
	}

	glm::mat4 HeadlessScene::getActorWorldMatrix(Actor& a)
	{
		// if this actor has no parent, simply return the matrix of it's transform
		if (!a.parentActor)
			return a.getTransform().getMatrix();

		// if this actor is parented to another actor, and not to that actor's bone
		if (a.parentActorBone == -1)
			return this->getActorWorldMatrix(this->actors[a.parentActor]) * a.getTransform().getMatrix();

		// if this actor is parented to the bone of its parent actor
		return this->getActorWorldMatrix(this->actors[a.parentActor]) *
			ozzFloat4x4ToGlmMat4(this->actors[a.parentActor].animator->getSimBoneMatrix(a.parentActorBone)) *
			a.getTransform().getMatrix();
	}

	glm::mat4 Scene::getActorWorldRenderMatrix(Actor& a, float alpha)
	{
		// actor is not dynamic (does not move) so interpolation is not required, simply return it's world matrix
		if (!(a.flags & ACTFLG_DYNAMIC) || !(a.flags & ACTFLG_LERPABLE))
			return this->getActorWorldMatrix(a);

		glm::mat4 selfMat = Transform::interpolateTransforms(a.getPreviousTransform(), a.getTransform(), alpha);

		// if this actor has no parent, simply return the matrix of it's transform
		if (!a.parentActor)
			return selfMat;

		// if this actor is parented to another actor
		if (a.parentActorBone == -1)
			return this->getActorWorldRenderMatrix(this->actors[a.parentActor], alpha) * selfMat;

		// if we made it here, we know that this actor is parented to a bone of its parent actor
		return this->getActorWorldRenderMatrix(this->actors[a.parentActor], alpha) *
			ozzFloat4x4ToGlmMat4(this->actors[a.parentActor].animator->getRenderBoneMatrix(a.parentActorBone)) *
			selfMat;
	}

	glm::mat4 Scene::getActorWorldRenderMatrix(Actor& a, float alpha)
	{
		glm::mat4 localMatrix;

		if ((a.flags & ACTFLG_DYNAMIC) && (a.flags & ACTFLG_LERPABLE) && a.transformUpdatedThisTick())
		{
			// actor requires interpolation
			localMatrix = Transform::interpolateTransforms(a.getPreviousTransform(), a.getTransform(), alpha);
		}
		else
		{
			// actor is not dynamic (does not move) so interpolation is not required, simply return it's world matrix
			localMatrix = a.getTransform().getMatrix();
		}

		// if this actor has no parent, simply return the matrix of it's transform
		if (!a.parentActor)
			return localMatrix;

		// actor has a parent

		// if actor is parented to a bone of another actor
		if (a.parentActorBone == -1)
			return this->getActorWorldRenderMatrix(this->actors[a.parentActor], alpha) * localMatrix;

		return this->getActorWorldRenderMatrix(this->actors[a.parentActor], alpha) *
			ozzFloat4x4ToGlmMat4(this->actors[a.parentActor].animator->getRenderBoneMatrix(a.parentActorBone)) *
			localMatrix;
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

	DrawBucketLocation Scene::findOrCreateDrawBucket(Stage* stage, RenderPass pass, uint32_t shader, uint32_t vao)
	{
		std::vector<DrawBucket>& buckets = pass == RENDER_PASS_OPAQUE ? stage->opaqueBuckets : stage->transparentBuckets;

		for (uint32_t i = 0; i < buckets.size(); i++)
		{
			if (buckets[i].shader == shader && buckets[i].vao == vao)
				return { pass, i };
		}

		DrawBucket& bucket = buckets.emplace_back();
		bucket.shader = shader;
		bucket.vao = vao;
		Runtime::_gpu->createBuffer(&bucket.indirectBuffer);

		return { pass, static_cast<uint32_t>(buckets.size() - 1) };
	}

	actor_handle HeadlessScene::addActor(Mesh* mesh)
	{
		Actor a{};
		a.mesh = mesh;
		return this->actors.insert(a);
	}

	actor_handle Scene::addActor(Stage* stage, Mesh* mesh, std::vector<material_handle> materials, uint32_t flags)
	{
		Actor a{};
		a.stage = stage;
		a.mesh = mesh;
		a.flags = flags;
		a.materialIndices = materials;

		return this->actors.insert(a);
	}

	void Scene::initActorDrawBuckets()
	{
		for (auto& a : this->actors)
		{
			for (uint32_t i = 0; i < a.materialIndices.size(); i++)
			{
				Material& m = this->materials[a.materialIndices[i]];

				if (m.flags & MTLFLG_HAS_AMBIENT_CUBE)
				{
					a.flags |= ACTFLG_AMBIENT_CUBE; // make sure actor has ambient cube flag if one of its materials has it
					if (a.ambientCube.size() == 0)
					{
						// prime a value (should be overwritten by application specific logic)
						a.ambientCube = {
							{1.f, 1.f, 1.f}, {1.f, 1.f, 1.f}, {1.f, 1.f, 1.f},
							{1.f, 1.f, 1.f}, {1.f, 1.f, 1.f}, {1.f, 1.f, 1.f}
						};
					}
				}

				if ((m.flags & MTLFLG_IS_TRANSPARENT) || (m.flags & MTLFLG_IS_RGBA) || (m.flags & MTLFLG_IS_TEXT) || (m.flags & MTLFLG_IS_ALPHA_MASK))
					a.drawBuckets.push_back(this->findOrCreateDrawBucket(a.stage, RENDER_PASS_TRANSPARENT, m.shaderProgramId, a.mesh->gp->gpuGeoPool->VAO));
				else
					a.drawBuckets.push_back(this->findOrCreateDrawBucket(a.stage, RENDER_PASS_OPAQUE, m.shaderProgramId, a.mesh->gp->gpuGeoPool->VAO));

				if (a.flags & ACTFLG_ANIMATED_MATERIAL)
					a.materialAnimators.emplace_back(m.textures.size(), 24.f);
			}
		}
	}

	actor_handle Scene::addActor(Stage* stage, Mesh* mesh, SkelAnimator* animator, std::vector<material_handle> materials, uint32_t flags)
	{
		Actor a{};
		a.stage = stage;
		a.mesh = mesh;
		a.flags = flags;
		a.materialIndices = materials;
		a.animator = animator;

		unsigned int index = 0;
		for (auto& meshBone : a.mesh->bones)
		{
			int skelBoneIndex = a.animator->getBoneIndex(meshBone.name);
			VEL_ASSERT(skelBoneIndex != -1, ("Actor::setAnimator(): Skeleton does not contain bone with name " + meshBone.name).c_str());

			a.activeBones.push_back(std::pair<unsigned int, unsigned int>(skelBoneIndex, index));
			index++;
		}

		return this->actors.insert(a);
	}





} // END NAMESPACE