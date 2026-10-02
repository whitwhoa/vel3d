
#include <vel/Scene/UI/UICursor.h>

namespace vel
{
	UICursor::UICursor(Scene* scene, actor_handle pa, actor_handle tsa) :
		scene(scene),
		pointerActor(pa),
		textSelectActor(tsa),
		overElement(nullptr),
		overZ(0.f),
		holdInterrupt(false)
	{}

	const glm::vec3& UICursor::getPos() const
	{
		return this->scene->getActor(this->pointerActor).getTranslation();
	}

	void UICursor::updatePos(float dx, float dy)
	{
		Actor& pa = this->scene->getActor(this->pointerActor);
		Actor& ta = this->scene->getActor(this->textSelectActor);

		glm::vec3 currentPos = pa.getTranslation();

		pa.setTranslation((currentPos + glm::vec3(dx, dy, 0.f)));
		ta.setTranslation((currentPos + glm::vec3(dx, dy, 0.f)));
	}

	void UICursor::showPointer()
	{
		Actor& ta = this->scene->getActor(this->textSelectActor);
		ta.flags &= ~ACTFLG_VISIBLE;

		Actor& pa = this->scene->getActor(this->pointerActor);
		pa.flags |= ACTFLG_VISIBLE;
	}

	void UICursor::showTextSelect()
	{
		Actor& ta = this->scene->getActor(this->textSelectActor);
		ta.flags |= ACTFLG_VISIBLE;

		Actor& pa = this->scene->getActor(this->pointerActor);
		pa.flags &= ~ACTFLG_VISIBLE;
	}
}