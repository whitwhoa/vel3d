#pragma once

#include <cstdint>

#include <vel/Scene/HeadlessScene.h>

namespace vel
{
	class HeadlessApp
	{
	protected:
		std::vector<std::unique_ptr<HeadlessScene>>		scenes;
		HeadlessScene*									activeScene;
		uint32_t										currentSimTick;

	public:
		HeadlessApp();
		~HeadlessApp();

		void			addScene(std::unique_ptr<HeadlessScene> scene, bool makeActive = false);
		void			removeScene(unsigned int id);
		void			swapScene(unsigned int id);
		
		void			stepSimulation(float dt);

	};
}