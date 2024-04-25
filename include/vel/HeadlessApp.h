#pragma once

#include "vel/AssetManager.h"
#include "vel/HeadlessScene.h"

namespace vel
{
	class HeadlessApp
	{
	private:
		AssetManager*									assetManager;
		std::vector<std::unique_ptr<HeadlessScene>>		scenes;
		HeadlessScene*									activeScene;
		int												currentSimTick;

	public:
		HeadlessApp(AssetManager* am);
		~HeadlessApp();

		void addScene(std::unique_ptr<HeadlessScene> scene, bool makeActive = false);
		void stepSimulation(float dt);
	};
}