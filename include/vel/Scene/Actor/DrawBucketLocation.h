#pragma once

#include <cstdint>

namespace vel
{
    enum RenderPass : uint8_t
    {
        RENDER_PASS_OPAQUE,
        RENDER_PASS_TRANSPARENT
    };

    struct DrawBucketLocation
    {
        RenderPass pass = RENDER_PASS_OPAQUE;
        uint32_t index = 0;
    };
}