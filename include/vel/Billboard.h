#pragma once

#include "vel/Actor.h"
#include "vel/Camera.h"

namespace vel
{
	class Billboard
	{
	private:
		vel::Actor*		billboardActor;
		vel::Camera*	parentCamera;
		bool			lockXZ;

	public:
		Billboard(vel::Actor* billboardActor, vel::Camera* parentCamera);

		vel::Actor* getActor() const;
		vel::Camera* getCamera() const;
		
		void lockXZRotation(bool b = true);

		/*
			Billboarding orientation notes (OpenGL-style):
			- Model local axes: +X right, +Y up, -Z forward (mesh faces down -Z by default).
			- We compute dir = cameraPos - objectPos (object->camera).
			- We set forward = -dir so the model's LOCAL -Z points toward the camera in WORLD space.
			- worldUp = +Y.
			- Basis: right = normalize(cross(worldUp, forward));
					 up    = normalize(cross(forward, right));
			- R columns: [ right | up | forward ] -> quat -> setRotation(q).

			If your mesh is authored with +Z as forward, use `forward = dir` instead.
		*/
		void update();

		
	};
}