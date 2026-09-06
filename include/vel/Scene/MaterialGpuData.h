#pragma once

#include <cstdint>
#include <glm/glm.hpp>


namespace vel
{
    struct MaterialGpuData
    {
        uint32_t textureOffset = 0;
        uint32_t textureCount = 0;

        uint32_t flags = 0;
        float f1 = 0.0f;
        float f2 = 0.0f;
    };
    static_assert(sizeof(MaterialGpuData) == 20);
}