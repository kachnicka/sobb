#pragma once

#include "../ApplicationState.h"

#include "../backend/Config.h"
#include "../backend/Stats.h"
#include "../backend/vulkan/Vulkan.h"

#include <queue>

class Application;
namespace module {

class Benchmark {
public:
    explicit Benchmark(Application& app);
    void ProcessGUI(State& state);

    void LoadConfig();
    void SetupBenchmarkRun(State& state);
    void Run();
    void ExportTexTableRows();

    void SetScene(std::string_view name);
    void SetDefaultAsActive();

private:
    Application& app;
    backend::vulkan::Vulkan& backend;

    std::vector<backend::config::BVHPipeline> bPipelines;
    std::vector<std::string> scene;
    std::vector<std::string> method;

    struct {
        std::queue<i32> scenes;
        std::vector<i32> pipelines;

        i32 currentScene { 0 };
        i32 currentPipeline { 0 };
        i32 currentView { 0 };
        bool bRunning { false };
    } rt;

    enum class BenchmarkState {
        eIdle,
        eSceneSet,
        eSceneLoadInProgress,
        ePipelineSet,
        ePipelineGetStats,
        eViewSet,
        eViewGetStats,
        eWaitFrames,
    } bState { BenchmarkState::eIdle };

    struct BPipeline {
        std::string name;
        backend::stats::BVHPipeline statsBuild;
        std::vector<backend::stats::Trace> statsTrace;

        f32 pMRps { 0.0f };
        f32 sMRps { 0.0f };
        f32 pTimeRel { 0.0f };
        f32 sTimeRel { 0.0f };
        f32 timeTotal { 0.0f };
        f32 avgNodesPerRay { 0.f };
        f32 avgTrisPerRay { 0.f };
        f32 avgBVsPerRay { 0.f };
        f32 memory { 0.0f };
    };

    struct SceneBenchmark {
        std::string name;
        std::vector<BPipeline> pipelines;
    };

    struct StatsExport {
        enum class Stat {
            eSA_inner,
            eSA_leaf,
            eSAHCost,
            eAverageLeafSize,
            eMemoryConsumption,
            eBuildTimeTotal,
            eBuildTimeBuild,
            eBuildTimeCollapse,
            eBuildTimeTransform,
            eBuildTimeCompress,
            eAvgTestedNodes,
            eAvgTestedTriangles,
            eAvgTestedBVs,
            ePMRps,
            eSMRps,
            eTraceTimePrimary,
            eTraceTimeSecondary,
            eTraceTimeTotal,
        };

        struct ToPrint {
            Stat stat;
            bool includeRelativeCol;
        };
        std::vector<ToPrint> stats;

        StatsExport();

        void PrintHeader();
        void PrintFooter();
        void PrintScene(std::string_view name);
        void PrintPipeline(backend::config::BVHPipeline& bConfig, BPipeline& p, BPipeline const& pRel);

    private:
        void headerL1(ToPrint const& tp);
        void headerL2(ToPrint const& tp);
        void val_f32(ToPrint const& tp);
        void val_i32(ToPrint const& tp);
    } statExporter;

    std::vector<SceneBenchmark> sceneBenchmarks;
    void ExportPipeline(BPipeline& p, BPipeline const& pRel);
};

}
