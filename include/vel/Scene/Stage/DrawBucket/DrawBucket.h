#pragma once

#include <cstdint>
#include <vector>

#include <vel/Scene/Stage/DrawBucket/DrawElementsIndirectCommand.h>

namespace vel
{
    struct DrawBucket
    {
        unsigned int shader = 0;
        unsigned int vao = 0;
        unsigned int indirectBuffer = 0;
        uint32_t materialIndexOffset = 0; // gl_DrawID is local to bucket MDI call. This translates it to global per-draw
        std::vector<DrawElementsIndirectCommand> drawCommands; // lockstep with below
        std::vector<uint32_t> localMaterialIndices; // lockstep with above
    };
}