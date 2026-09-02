#pragma once

#include <cstdint>
#include <glm/glm.hpp>


namespace vel
{
    struct alignas(16) ActorGpuData
    {
        glm::mat4 model;

        int32_t activeBillboardTexture = 0;
        uint64_t lightmapHandle = 0;

        glm::vec4 colorMultiplier = glm::vec4(1.0f);
    };
    static_assert(sizeof(ActorGpuData) == 96);
}