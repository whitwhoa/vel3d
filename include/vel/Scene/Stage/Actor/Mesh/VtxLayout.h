#pragma once

#include <cstdint>

#include <glm/glm.hpp>

namespace vel
{
    enum VtxLayout : uint32_t
    {
        VTX_POS = 0,
        VTX_POS_NRML,
        VTX_POS_NRML_TX,
        VTX_POS_NRML_TX_LM,
        VTX_POS_NRML_TX_SKN
    };

    struct VtxPos
    {
        static constexpr VtxLayout layout = VTX_POS;

        glm::vec3 position;
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
        glm::vec2 textureCoords;
        unsigned int materialUBOIndex = 0;
    };

    struct VtxPosNrmlTxLm
    {
        static constexpr VtxLayout layout = VTX_POS_NRML_TX_LM;

        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 textureCoords;
        glm::vec2 lightmapCoords;
        unsigned int materialUBOIndex = 0;
    };

    struct VtxPosNrmlTxSkn
    {
        static constexpr VtxLayout layout = VTX_POS_NRML_TX_SKN;

        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 textureCoords;
        unsigned int boneIds[4] = {0,0,0,0}; // 4 bones allowed per vertex
        float boneWeights[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        unsigned int materialUBOIndex = 0;
    };

}
