#pragma once

namespace vel
{
	struct BufferIds
	{
		unsigned int cameraUbo = 0;
		unsigned int actorDataSsbo = 0;
		unsigned int materialDataSsbo = 0;
		unsigned int materialTextureHandlesSsbo = 0;
		unsigned int actorAmbientCubeSsbo = 0;
	};
}