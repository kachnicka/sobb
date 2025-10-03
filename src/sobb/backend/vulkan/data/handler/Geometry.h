#pragma once

#include <vLime/Memory.h>
#include <vLime/Transfer.h>

#include "../../../data/Input.h"
#include <berries/util/UidUtil.h>

namespace backend::vulkan::data {

struct Geometry {
    using ID = uid<Geometry>;
    uint32_t vertexCount { 0 };
    uint32_t indexCount { 0 };
    lime::Buffer::Detail indexBuffer;
    lime::Buffer::Detail vertexBuffer;
    lime::Buffer::Detail uvBuffer;
    lime::Buffer::Detail normalBuffer;
    void reset()
    {
        vertexBuffer.reset();
        indexBuffer.reset();
        uvBuffer.reset();
        normalBuffer.reset();
    }
};

class GeometryHandler {
public:
    VCtx ctx;

    // private:
    uidVector<Geometry> geometries;
    std::vector<lime::Buffer> buffer;
    std::vector<lime::LinearAllocator> linearAllocator;
    vk::DeviceSize alignment { 0 };

public:
    explicit GeometryHandler(VCtx ctx)
        : ctx(ctx)
    {
        allocateBuffer(1);
    }

    Geometry& operator[](Geometry::ID id)
    {
        return geometries[id];
    }

    Geometry const& operator[](Geometry::ID id) const
    {
        return geometries[id];
    }

    [[nodiscard]] auto begin() const
    {
        return geometries.begin();
    }

    [[nodiscard]] auto end() const
    {
        return geometries.end();
    }

    [[nodiscard]] auto size() const
    {
        return geometries.size();
    }

    Geometry::ID add(input::Geometry const& g)
    {
        Geometry geometry {
            .indexBuffer = allocate(sizeof(uint32_t) * g.indexCount),
            .vertexBuffer = allocate(sizeof(float) * 3 * g.vertexCount),
            .uvBuffer = allocate(sizeof(float) * 2 * g.vertexCount),
            .normalBuffer = allocate(sizeof(float) * 3 * g.vertexCount),
        };
        ctx.transfer.ToDeviceSync(g.indexData, geometry.indexBuffer.size, geometry.indexBuffer);
        ctx.transfer.ToDeviceSync(g.vertexData, geometry.vertexBuffer.size, geometry.vertexBuffer);

        if (g.uvData)
            ctx.transfer.ToDeviceSync(g.uvData, geometry.uvBuffer.size, geometry.uvBuffer);
        if (g.normalData)
            ctx.transfer.ToDeviceSync(g.normalData, geometry.normalBuffer.size, geometry.normalBuffer);
        geometry.vertexCount = g.vertexCount;
        geometry.indexCount = g.indexCount;

        return geometries.add(geometry);
    }

    void remove(Geometry::ID id)
    {
        geometries.remove(id);
    }

    void reset()
    {
        geometries.reset();
        buffer.clear();
        linearAllocator.clear();
        allocateBuffer(1);
    }

    // private:
    lime::Buffer::Detail allocate(vk::DeviceSize size)
    {
        for (auto& la : linearAllocator)
            if (auto b { la.alloc(size, alignment) }; b.resource)
                return b;
        allocateBuffer(size);
        return linearAllocator.back().alloc(size, alignment);
    }

    void allocateBuffer(vk::DeviceSize size)
    {
        using Usage = vk::BufferUsageFlagBits;
        auto usage { Usage::eVertexBuffer | Usage::eIndexBuffer | Usage::eTransferDst };
        if (ctx.memory.features.bufferDeviceAddress)
            usage |= Usage::eShaderDeviceAddress;
        if (ctx.capabilities.isAvailable("Ray Tracing KHR"))
            usage |= Usage::eAccelerationStructureBuildInputReadOnlyKHR;

        size = std::max(size, 128 * lime::MB);
        buffer.emplace_back(ctx.memory.alloc({ .memoryUsage = lime::DeviceMemoryUsage::eDeviceOptimal }, { .size = size, .usage = usage }, std::format("geometry_{}", buffer.size()).c_str()));

        linearAllocator.emplace_back(buffer.back());
        alignment = buffer.back().getMemoryRequirements(ctx.memory.d).alignment;
    }
};

}
