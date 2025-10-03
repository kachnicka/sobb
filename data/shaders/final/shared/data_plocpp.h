#ifndef PLOCPP_DATA_NEW_H
#define PLOCPP_DATA_NEW_H

#ifndef INCLUDE_FROM_SHADER
#    include <berries/util/types.h>
using vec3 = f32[3];
#    pragma pack(push, 1)
namespace data_plocpp {
#else
#endif

struct PC_MortonGlobal {
    vec3 sceneAabbCubedMin;
    f32 sceneAabbNormalizationScale;
    u64 mortonAddress;
    u64 bvhAddress;
    u64 bvhTrianglesAddress;
    u64 bvhTriangleIndicesAddress;
    u64 auxBufferAddress;
};

struct PC_MortonPerGeometry {
    u64 idxAddress;
    u64 vtxAddress;
    u32 globalTriangleIdBase;
    u32 sceneNodeId;
    u32 triangleCount;
};

struct PC_CopySortedNodeIds {
    u64 mortonAddress;
    u64 nodeIdAddress;
    u32 clusterCount;
};

struct PC_PlocppIterationIndirect {
    u64 bvhAddress;
    u64 nodeId0Address;
    u64 nodeId1Address;
    u64 dlWorkBufAddress;
    u64 runtimeDataAddress;
    u64 auxBufferAddress;
    u64 idbAddress;
};

struct PC_DiscoverPairs {
    u64 bvhAddress;
    u64 nodeId0Address;
    u64 nodeId1Address;
    u64 dlWorkBufAddress;
    u64 runtimeDataAddress;
    u64 idbAddress;

    u64 scratchNodesAddress;
    u64 scratchIds0Address;
    u64 scratchIds1Address;
    u64 scratchTriIdsAddress;

    u64 bvhTriangleIndicesAddress;
    u64 geometryDescriptorAddress;
};

struct PC_Collapse {
    u64 bvhAddress;
    u64 bvhTrianglesAddress;
    u64 bvhTriangleIndicesAddress;
    u64 bvhCollapsedAddress;
    u64 bvhCollapsedTrianglesAddress;
    u64 bvhCollapsedTriangleIndicesAddress;

    u64 counterAddress;
    u64 nodeStateAddress;
    u64 sahCostAddress;
    u64 leafNodeAddress;
    u64 newNodeIdAddress;
    u64 newTriIdAddress;
    u64 collapsedTreeNodeCountsAddress;

    u64 schedulerDataAddress;
    u64 indirectDispatchBufferAddress;

    u64 debugAddress;

    u32 leafNodeCount;
    u32 maxLeafSize;

    f32 c_t;
    f32 c_i;
};

struct PC_TransformToDOP {
    u64 bvhAddress;
    u64 bvhTriangleIndicesAddress;
    u64 bvhDOPAddress;

    u64 bvhNodeCountsAddress;
    u64 geometryDescriptorAddress;
    u64 countersAddress;

    u32 leafNodeCount;
};

struct PC_TransformToOBB {
    u64 bvhInAddress;
    u64 bvhInTriangleIndicesAddress;
    u64 bvhOutAddress;

    u64 ditoPointsAddress;
    u64 obbAddress;

    u64 geometryDescriptorAddress;
    u64 traversalCounterAddress;
    u64 schedulerDataAddress;
    u64 timesAddress_TMP;

    u32 leafNodeCount;
};

struct PC_TransformToSOBB {
    u64 bvhAddress;
    u64 bvhTriangleIndicesAddress;
    u64 bvhSOBBAddress;

    u64 geometryDescriptorAddress;
    u64 countersAddress;

    u64 dopBaseAddress;
    u64 dopRefAddress;

    u64 statsAddress;

    u32 leafNodeCount;
};

struct PC_Rearrange {
    u64 bvhAddress;
    u64 bvhWideAddress;
    u64 workBufferAddress;
    u64 runtimeDataAddress;
    u64 auxBufferAddress;
    u32 leafNodeCount;
};

struct SC {
    u32 sizeWorkgroup;
    u32 sizeSubgroup;
    u32 plocRadius;

#ifndef INCLUDE_FROM_SHADER
    [[nodiscard]] u32 getChunkSize() const
    {
        return sizeWorkgroup - 4 * plocRadius;
    }
#endif
};

struct Morton32KeyVal {
    u32 key;
    u32 mortonCode;
};

struct DLPartitionData {
    u32 aggregate;
    u32 prefix;
};

struct PLOCData {
    u32 bvOffset;
    u32 iterationClusterCount;
    u32 iterationTaskCount;
    u32 fill;
    u32 iterationCounter;
};

struct IndirectClusters {
    u32 cntClustersTotal;
    u32 cntTriangles;
    u32 cntPgrams;
    u32 cntButterflies;

    u32 wg_x;
    u32 wg_y;
    u32 wg_z;
};

#ifndef INCLUDE_FROM_SHADER
static_assert(sizeof(PC_MortonGlobal) == 56);
static_assert(sizeof(PC_MortonPerGeometry) == 28);
static_assert(sizeof(PC_PlocppIterationIndirect) == 56);
static_assert(sizeof(PC_DiscoverPairs) == 96);
static_assert(sizeof(PC_CopySortedNodeIds) == 20);
static_assert(sizeof(PC_Collapse) == 144);
static_assert(sizeof(PC_TransformToDOP) == 52);
static_assert(sizeof(PC_TransformToOBB) == 76);
static_assert(sizeof(PC_TransformToSOBB) == 68);
static_assert(sizeof(PC_Rearrange) == 44);
static_assert(sizeof(IndirectClusters) == 28);
}
#    pragma pack(pop)
#else
layout(buffer_reference, scalar) buffer Morton32KeyVals { Morton32KeyVal keyval[]; };
layout(buffer_reference, scalar) buffer DLWork_buf { DLPartitionData val[]; };
layout(buffer_reference, scalar) buffer IndirectClusters_ref { IndirectClusters ic; };

#endif

#endif
