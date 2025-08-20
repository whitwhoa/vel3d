#pragma once

namespace vel
{
    enum MaterialOptions
    {
        MTRL_OPT_NONE = 0,
        MTRL_OPT_TRANSLUCENT = 1 << 0, // 0001
        MTRL_OPT_CUTOUT = 1 << 1, // 0010
        // add more as needed
    };
}
