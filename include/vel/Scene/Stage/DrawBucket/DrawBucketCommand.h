#pragma once

#include <cstdint>

namespace vel
{
    struct DrawBucketCommand
    {
        uint32_t count;
        uint32_t instanceCount;
        uint32_t firstIndex;
        int32_t  baseVertex;
        uint32_t baseInstance;

        uint32_t materialIndex;
        uint32_t activeFrame;
    };
    static_assert(sizeof(DrawBucketCommand) == 28);
}