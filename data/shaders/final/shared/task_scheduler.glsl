#ifndef TASK_SCHEDULER_GLSL
#define TASK_SCHEDULER_GLSL 1

#ifndef SCHEDULER_DATA_ADDRESS
#define SCHEDULER_DATA_ADDRESS pc.schedulerDataAddress
#endif

#include "type_32.glsl"

#ifndef SCHEDULER_GLOBAL_TASK_ID
struct Task {
    u32 phase;
    u32 id;
};
#else
struct Task {
    u32 phase;
    u32 id;
    u32 idGlobal;
};
#endif

struct Scheduler {
    u32 phase;
    u32 idStart;
    u32 idEnd;
    u32 idToAssign;
    u32 numFinished;
};

layout(buffer_reference, scalar) buffer SchedRef {
    Scheduler sched;
};
shared u32 sharedTaskId;

void allocTasks(in u32 numTasks, in u32 phase)
{
    SchedRef data = SchedRef(SCHEDULER_DATA_ADDRESS);

    const u32 end = data.sched.idEnd;
    data.sched.phase = phase;
    data.sched.idStart = end;
    memoryBarrier(gl_ScopeDevice, gl_StorageSemanticsBuffer,
        gl_SemanticsAcquireRelease | gl_SemanticsMakeAvailable | gl_SemanticsMakeVisible);
    data.sched.idEnd = end + numTasks;
}

Task beginTask(const u32 localId)
{
    SchedRef data = SchedRef(SCHEDULER_DATA_ADDRESS);

    if (localId == 0)
        sharedTaskId = atomicAdd(data.sched.idToAssign, 1);
    barrier();
    const u32 myTaskId = sharedTaskId;

    do {
        memoryBarrier(gl_ScopeDevice, gl_StorageSemanticsBuffer,
            gl_SemanticsAcquireRelease | gl_SemanticsMakeAvailable | gl_SemanticsMakeVisible);
    } while (myTaskId >= data.sched.idEnd);

    #ifndef SCHEDULER_GLOBAL_TASK_ID
    return Task(data.sched.phase, myTaskId - data.sched.idStart);
    #else
    return Task(data.sched.phase, myTaskId - data.sched.idStart, myTaskId);
    #endif
}

bool endTask(const u32 localId)
{
    SchedRef data = SchedRef(SCHEDULER_DATA_ADDRESS);

    controlBarrier(gl_ScopeWorkgroup, gl_ScopeDevice, gl_StorageSemanticsBuffer,
        gl_SemanticsAcquireRelease | gl_SemanticsMakeAvailable | gl_SemanticsMakeVisible);
    if (localId == 0) {
        const u32 endPhaseIndex = data.sched.idEnd;
        const u32 finishedTaskId = atomicAdd(data.sched.numFinished, 1);
        return (finishedTaskId == (endPhaseIndex - 1));
    }
    return false;
}

u32 getTaskCount()
{
    SchedRef data = SchedRef(SCHEDULER_DATA_ADDRESS);
    return data.sched.idEnd - data.sched.idStart;
}

#endif

