#pragma once

#include <optional>

#include <vel/Scene/Stage/Actor/Mesh/VtxLayout.h>

namespace vel
{
    struct GpuGeoPool
    {
        unsigned int VAO;
        unsigned int VBO;
        unsigned int EBO;
    };


    class GeoPool
    {
    public:
        VtxLayout vtxLayout;
        std::vector<unsigned int> indices;
        std::optional<GpuGeoPool> gpuGeoPool;
        bool standalone;

        virtual ~GeoPool() = default;
        virtual unsigned int vertexCount() = 0;

    protected:
        GeoPool(VtxLayout layout, bool standalone) :
            vtxLayout(layout),
            standalone(standalone)
        {}
    };


    template<typename T>
    class GeoPoolT : public GeoPool
    {
    public:
        std::vector<T> vertices;
        
        GeoPoolT(bool standalone = false) :
            GeoPool(T::layout, standalone)
        {}

        unsigned int vertexCount() override
        {
            return this->vertices.size();
        }
    };
}