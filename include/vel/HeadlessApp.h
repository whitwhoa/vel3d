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
		void			removeScene(const std::string& name);
		void			swapScene(const std::string& name);
		bool			sceneExists(const std::string& name);
		HeadlessScene*	getScene(const std::string& name);
		
		void			stepSimulation(float dt);

	};
}