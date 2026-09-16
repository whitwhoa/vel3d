#pragma once

#include <cstdint>
#include <glm/glm.hpp>


namespace vel
{
    enum ActorGpuFlags : uint32_t
    {
        ACTOR_GPU_HAS_AMBIENT_CUBE = 1u << 0,
        ACTOR_GPU_IS_BILLBOARD = 1u << 1,
        ACTOR_GPU_BILLBOARD_LOCK_Y = 1u << 2
    };

    struct alignas(16) ActorGpuData
    {
        glm::mat4 model;

        glm::vec4 colorMultiplier = glm::vec4(1.0f);

        uint64_t lightmapHandle = 0;

        uint32_t flags = 0;
        uint32_t ambientCubeOffset = 0;
        uint32_t boneMatrixOffset = 0;
    };
    static_assert(sizeof(ActorGpuData) == 112);
}