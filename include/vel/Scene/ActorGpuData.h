#pragma once

#include <cstdint>
#include <glm/glm.hpp>


namespace vel
{
    struct alignas(16) ActorGpuData
    {
        glm::mat4 model;

        glm::vec4 colorMultiplier = glm::vec4(1.0f);

        uint64_t lightmapHandle = 0;

        uint32_t ambientCubeOffset = 0;
        uint32_t boneMatrixOffset = 0;
    };
    static_assert(sizeof(ActorGpuData) == 96);
}