#pragma once

#include <glm/glm.hpp>

#include <vel/Scene/Scene.h>

namespace vel
{
	class UIElement;

	class UICursor
	{
	private:
		friend class Scene;

		Scene* scene;
		actor_handle pointerActor;
		actor_handle textSelectActor;



	public:
		UIElement*	overElement;
		float		overZ;
		bool		holdInterrupt;


		UICursor(Scene* scene, actor_handle pa, actor_handle tsa);

		const glm::vec3& getPos() const;

		void updatePos(float dx, float dy);
		void showPointer();
		void showTextSelect();

	};
}

