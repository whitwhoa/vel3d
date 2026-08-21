#pragma once

#include <cstdint>

#include <glm/glm.hpp>

namespace vel
{
    enum VtxLayout : uint32_t
    {
        VTX_POS_NRML = 0,
        VTX_POS_NRML_TX,
        VTX_POS_NRML_TX_LM,
        VTX_POS_NRML_TX_SKN
    };

    struct VtxPosNrml
    {
        static constexpr VtxLayout layout = VTX_POS_NRML;

        glm::vec3 position;
        glm::vec3 normal;
    };

    struct VtxPosNrmlTx
    {
        static constexpr VtxLayout layout = VTX_POS_NRML_TX;

        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 textureCoordinates;
        unsigned int materialUBOIndex;
    };

    struct VtxPosNrmlTxLm
    {
        static constexpr VtxLayout layout = VTX_POS_NRML_TX_LM;

        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 textureCoordinates;
        glm::vec2 lightmapCoordinates;
        unsigned int materialUBOIndex;
    };

    struct VtxPosNrmlTxSkn
    {
        static constexpr VtxLayout layout = VTX_POS_NRML_TX_SKN;

        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 textureCoordinates;
        unsigned int boneIds[4]; // 4 bones allowed per vertex
        float boneWeights[4];
        unsigned int materialUBOIndex;
    };

}
