#pragma once

#include <cstdint>
#include <vector>

#include <vel/Scene/Stage/DrawBucket/DrawBucketCommand.h>

namespace vel
{
    struct DrawBucket
    {
        uint32_t    shader = 0;
        uint32_t    vao = 0;
        uint32_t    indirectBuffer = 0;

        std::vector<DrawBucketCommand> drawCommands;
    };
}