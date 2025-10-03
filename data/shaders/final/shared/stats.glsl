#ifndef STATS_GLSL
#define STATS_GLSL

#if STATS
shared uint sTris;
shared uint sNodes;
shared uint sBVs;
#define STATS_SHARED_RESET  \
    sTris = 0;              \
    sNodes = 0;             \
    sBVs = 0 ;
#define STATS_SHARED_STORE          \
    atomicAdd(sNodes, testedNodes); \
    atomicAdd(sTris, testedTris);   \
    atomicAdd(sBVs, testedBVs);
#define STATS_GLOBAL_STORE                                              \
    TraceStats_ref stats = TraceStats_ref(pc.data.traceStatsAddress);   \
    atomicAdd(stats.val.testedNodes, sNodes);                           \
    atomicAdd(stats.val.testedTriangles, sTris);                        \
    atomicAdd(stats.val.testedBVolumes, sBVs);
#define STATS_LOCAL_DEF     \
    u32 testedNodes = 0;   \
    u32 testedTris = 0;    \
    u32 testedBVs = 0;
#define STATS_LOCAL_RESET   \
    testedNodes = 0;        \
    testedTris = 0;         \
    testedBVs = 0 ;
#define STATS_NODE_PP testedNodes++;
#define STATS_TRI_PP testedTris++;
#define STATS_BV_PP testedBVs++;

#else
#define STATS_SHARED_RESET
#define STATS_SHARED_STORE
#define STATS_GLOBAL_STORE
#define STATS_LOCAL_DEF
#define STATS_LOCAL_RESET
#define STATS_NODE_PP
#define STATS_TRI_PP
#define STATS_BV_PP
#endif

#endif
