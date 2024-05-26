#pragma once

namespace vel
{
	class Actor;
	class GPU;

	class SkinnedMaterialMixin
	{
	private:
		

	public:
		SkinnedMaterialMixin();
		void updateBones(float alphaTime, GPU* gpu, Actor* a);

	};
}