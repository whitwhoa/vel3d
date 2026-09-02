#pragma once

#include <optional>
#include <variant>

#include <vel/Scene/GeoPool/GpuGeoPool.h>
#include <vel/Scene/GeoPool/VtxLayout.h>

namespace vel
{
    class GeoPool
    {
    public:
        std::vector<unsigned int>   indices;
        std::optional<GpuGeoPool>   gpuGeoPool;
        VtxLayout                   vtxLayout;

        virtual ~GeoPool() = default;
        virtual unsigned int vertexCount() = 0;

    protected:
        GeoPool(VtxLayout layout) :
            vtxLayout(layout)
        {}
    };


    template<typename T>
    class GeoPoolT : public GeoPool
    {
    public:
        std::vector<T> vertices;
        
        GeoPoolT() :
            GeoPool(T::layout)
        {}

        unsigned int vertexCount() override
        {
            return this->vertices.size();
        }
    };
}