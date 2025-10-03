#ifndef DATA_SCENE_H
#define DATA_SCENE_H

#ifndef INCLUDE_FROM_SHADER
#    include <berries/util/types.h>
#    pragma pack(push, 1)
namespace data_scene {
#else
#endif

struct Geometry {
    u64 vtxAddress;
    u64 idxAddress;
    u64 normalAddress;
    u64 uvAddress;
};

#ifndef INCLUDE_FROM_SHADER
static_assert(sizeof(Geometry) == 32);
}
#    pragma pack(pop)
#else
layout(buffer_reference, scalar) buffer GeometryDescriptor { Geometry g[]; };
#endif

#endif
