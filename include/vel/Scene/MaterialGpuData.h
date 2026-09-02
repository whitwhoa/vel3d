#pragma once

#include <cstdint>
#include <glm/glm.hpp>


namespace vel
{
    struct alignas(16) MaterialGpuData
    {
        glm::vec4 color1 = glm::vec4(1.0f);
        glm::vec4 color2 = glm::vec4(1.0f);

        uint32_t textureOffset = 0;
        uint32_t textureCount = 0;

        uint32_t lineColorOffset = 0;
        uint32_t lineColorCount = 0;

        uint32_t ambientCubeOffset = 0;
        uint32_t ambientCubeCount = 0;

        uint32_t flags = 0;
        float f1 = 0.0f;
        float f2 = 0.0f;
    };
    static_assert(sizeof(MaterialGpuData) == 80);
}