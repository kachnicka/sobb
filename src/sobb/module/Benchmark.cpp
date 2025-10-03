#include "Benchmark.h"

#include "../Application.h"
#include "../core/ConfigFiles.h"
#include <final/shared/data_bvh.h>

#define FRAMES_WAIT 2
#define FRAMES_MEASURE 3

namespace module {

Benchmark::Benchmark(Application& app)
    : app(app)
    , backend(app.backend)
{
    LoadConfig();
    backend.SetPipelineConfiguration(bPipelines[0]);
}

void Benchmark::LoadConfig()
{
    bPipelines = app.configFiles.bvhPipelines;
    scene = app.configFiles.benchmarkScenes;
    method = app.configFiles.benchmarkPipelines;
}

void Benchmark::SetupBenchmarkRun(State& state)
{
    sceneBenchmarks.clear();
    rt = {};

    for (auto const& name : scene) {
        i32 i = 0;
        for (auto const& s : app.configFiles.GetScenes()) {
            if (name == s.name) {
                rt.scenes.push(i);
                break;
            }
            i++;
        }
    }
    for (auto const& name : method) {
        i32 i = 0;
        for (auto const& p : bPipelines) {
            if (name == p.name) {
                rt.pipelines.emplace_back(i);
                break;
            }
            i++;
        }
    }

    state.shaderHotReload = false;
    backend.state.samplesPerPixel = 64;
    backend.state.selectedRenderer = backend::vulkan::State::Renderer::ePathTracingCompute;
    backend.OnRenderModeChange();
    backend.ResetAccumulation();

    statExporter.PrintHeader();
    rt.bRunning = true;
    bState = BenchmarkState::eSceneSet;
}
void Benchmark::Run()
{
    switch (bState) {
    case BenchmarkState::eIdle:
        return;
    case BenchmarkState::eSceneSet: {
        if (rt.scenes.empty()) {
            bState = BenchmarkState::eIdle;
            rt.bRunning = false;

            statExporter.PrintFooter();
            return;
        }
        rt.currentScene = rt.scenes.front();
        rt.scenes.pop();

        app.state.sceneToLoad = app.configFiles.GetScenes()[rt.currentScene];
        sceneBenchmarks.push_back({ .name = app.state.sceneToLoad.name, .pipelines = {} });

        statExporter.PrintScene(app.state.sceneToLoad.name);
        bState = BenchmarkState::eSceneLoadInProgress;
    } break;
    case BenchmarkState::eSceneLoadInProgress: {
        if (backend.HaveScene())
            bState = BenchmarkState::ePipelineSet;
    } break;
    case BenchmarkState::ePipelineSet: {
        static int rtPipelineIdx { 0 };
        if (rtPipelineIdx >= csize<i32>(rt.pipelines)) {
            rtPipelineIdx = 0;
            bState = BenchmarkState::eSceneSet;
            return;
        }
        rt.currentPipeline = rt.pipelines[rtPipelineIdx++];
        auto& pCfg { bPipelines[rt.currentPipeline] };

        backend.SetPipelineConfiguration(pCfg);
        sceneBenchmarks.back().pipelines.push_back({ .name = pCfg.name, .statsBuild = {}, .statsTrace = {} });
        bState = BenchmarkState::ePipelineGetStats;
    } break;
    case BenchmarkState::ePipelineGetStats: {
        sceneBenchmarks.back().pipelines.back().statsBuild = backend.GetStatsBuild();

        bState = BenchmarkState::eViewSet;
    } break;
    case BenchmarkState::eViewSet: {
        if (rt.currentView >= csize<i32>(app.cameraManager.GetCameras())) {
            rt.currentView = 0;
            bState = BenchmarkState::ePipelineSet;

            statExporter.PrintPipeline(bPipelines[rt.currentPipeline], sceneBenchmarks.back().pipelines.back(), sceneBenchmarks.back().pipelines[0]);
            return;
        }
        app.cameraManager.SetActiveCamera(rt.currentView++);
        bState = BenchmarkState::eWaitFrames;
    } break;
    case BenchmarkState::eWaitFrames: {
        static int frames { 0 };
        if (frames++ >= FRAMES_WAIT) {
            frames = 0;
            bState = BenchmarkState::eViewGetStats;
        }
    } break;
    case BenchmarkState::eViewGetStats: {
        static int samples { 0 };
        if (samples++ >= FRAMES_MEASURE) {
            samples = 0;
            bState = BenchmarkState::eViewSet;
        }
        sceneBenchmarks.back().pipelines.back().statsTrace.push_back(backend.GetStatsTrace());
    } break;
    }
}

void Benchmark::SetScene(std::string_view name)
{
    rt.currentScene = -1;
    auto& scenes { app.configFiles.GetScenes() };
    for (i32 i { 0 }; i < csize<i32>(scenes); ++i)
        if (scenes[i].name == name)
            rt.currentScene = i;
}

void Benchmark::ExportTexTableRows()
{
    for (auto& s : sceneBenchmarks) {
        fmt::print("{} \\\\ \n", s.name);
        for (auto& p : s.pipelines)
            ExportPipeline(p, s.pipelines[0]);
    }
}
void Benchmark::SetDefaultAsActive()
{
    backend.SetPipelineConfiguration(bPipelines[0]);
    rt.currentPipeline = 0;
}

void Benchmark::ExportPipeline(BPipeline& p, BPipeline const& pRel)
{
    u64 pRayCount { 0 };
    f32 pTraceTimeMs { 0.f };
    u64 sRayCount { 0 };
    f32 sTraceTimeMs { 0.f };
    for (auto const& t : p.statsTrace) {
        pRayCount += t.data[0].rayCount;
        pTraceTimeMs += t.data[0].traceTimeMs;
        for (int i = 1; i < 8; i++) {
            sRayCount += t.data[i].rayCount;
            sTraceTimeMs += t.data[i].traceTimeMs;
        }
    }
    p.pMRps = (static_cast<f32>(pRayCount) * 1e-6f) / (pTraceTimeMs * 1e-3f);
    p.sMRps = (static_cast<f32>(sRayCount) * 1e-6f) / (sTraceTimeMs * 1e-3f);

    p.pTimeRel = pTraceTimeMs / (pTraceTimeMs + sTraceTimeMs);
    p.sTimeRel = sTraceTimeMs / (pTraceTimeMs + sTraceTimeMs);

    std::string_view name { p.name };
    f32 sal { p.statsBuild.rearrangement.saIntersect };
    f32 sal_r { sal / pRel.statsBuild.rearrangement.saIntersect };
    f32 sai { p.statsBuild.rearrangement.saTraverse };
    f32 sai_r { sai / pRel.statsBuild.rearrangement.saTraverse };
    f32 avgl { p.statsBuild.rearrangement.leafSizeAvg };
    f32 sat { p.statsBuild.rearrangement.costTotal };
    f32 sat_r { sat / pRel.statsBuild.rearrangement.costTotal };
    f32 pMRps { p.pMRps };
    f32 pMRps_r { pMRps / pRel.pMRps };
    f32 sMRps { p.sMRps };
    f32 sMRps_r { sMRps / pRel.sMRps };
    f32 buildTime { p.statsBuild.plocpp.timeTotal + p.statsBuild.collapsing.timeTotal + p.statsBuild.transformation.timeTotal + p.statsBuild.rearrangement.timeTotal };
    f32 buildTime_ref { pRel.statsBuild.plocpp.timeTotal + pRel.statsBuild.collapsing.timeTotal + pRel.statsBuild.transformation.timeTotal + pRel.statsBuild.rearrangement.timeTotal };
    f32 buildTime_r { buildTime / buildTime_ref };
    // empty & BV & SA leaves & rel & SA internal & rel & SA total & rel & avg. leaf size & pMRpS & rel & sMRpS & rel & build time
    fmt::print(" & {} & {:.1f} & ({:.2f}) & {:.1f} & ({:.2f}) & {:.1f} & {:.1f} & ({:.2f}) & {:.1f} & ({:.2f}) & {:.1f} & ({:.2f}) & {:.1f} & ({:.2f}) \\\\\n",
        name, sai, sai_r, sal, sal_r, avgl, sat, sat_r, pMRps, pMRps_r, sMRps, sMRps_r, buildTime, buildTime_r);
}

Benchmark::StatsExport::StatsExport()
{
    stats = {
        { Stat::eAverageLeafSize, false },
        { Stat::eBuildTimeTotal, true },
        { Stat::eMemoryConsumption, true },
        { Stat::eAvgTestedBVs, true },
        { Stat::eAvgTestedTriangles, true },
        { Stat::eTraceTimeTotal, true },
        { Stat::eTraceTimeSecondary, false },
        { Stat::ePMRps, true },
        { Stat::eSMRps, true },
    };
}
void Benchmark::StatsExport::PrintHeader()
{
    std::cout << R"(
\begin{table*}
\scriptsize
\renewcommand{\tabcolsep}{0.05cm}
  \caption{}
  \label{tab:}
)";
    u32 cols = 0;
    for (auto const& s : stats) {
        cols++;
        if (s.includeRelativeCol)
            cols++;
    }
    std::string colAlign;
    colAlign.reserve(cols + 3);
    colAlign = "l|l";
    for (u32 i = 0; i < cols; i++)
        colAlign += 'r';
    std::cout << "  \\begin{tabular}{" << colAlign << "}\n";
    std::cout << "    \\toprule\n";

    std::cout << "Scene &\n";
    std::cout << "Bounding &\n";
    for (size_t i = 0; i < stats.size(); i++) {
        headerL1(stats[i]);
        std::cout << (i == (stats.size() - 1) ? " \\\\\n" : " &\n");
    }
    std::cout << "\\multicolumn{1}{r|}{\\#triangles} &\n";
    std::cout << "volume &\n";
    for (size_t i = 0; i < stats.size(); i++) {
        headerL2(stats[i]);
        std::cout << (i == (stats.size() - 1) ? " \\\\\n" : " &\n");
    }

    std::cout << "    \\midrule\n\n";
}
void Benchmark::StatsExport::PrintFooter()
{
    std::cout << R"(
    \bottomrule
  \end{tabular}
\end{table*}
)";
}
void Benchmark::StatsExport::PrintScene(std::string_view name)
{
    std::cout << name << " \\\\\n";
}

void Benchmark::StatsExport::PrintPipeline(backend::config::BVHPipeline& bConfig, BPipeline& p, BPipeline const& pRel)
{
    u64 pRayCount { 0 };
    f32 pTraceTimeMs { 0.f };
    u64 sRayCount { 0 };
    f32 sTraceTimeMs { 0.f };
    u64 totalTestedNodes { 0 };
    u64 totalTestedTris { 0 };
    u64 totalTestedBVs { 0 };
    for (auto const& t : p.statsTrace) {
        pRayCount += t.data[0].rayCount;
        pTraceTimeMs += t.data[0].traceTimeMs;
        for (int i = 1; i < 8; i++) {
            sRayCount += t.data[i].rayCount;
            sTraceTimeMs += t.data[i].traceTimeMs;
        }
        totalTestedNodes += t.testedNodes;
        totalTestedTris += t.testedTriangles;
        totalTestedBVs += t.testedBVolumes;
    }
    p.pMRps = (static_cast<f32>(pRayCount) * 1e-6f) / (pTraceTimeMs * 1e-3f);
    p.sMRps = (static_cast<f32>(sRayCount) * 1e-6f) / (sTraceTimeMs * 1e-3f);
    p.pTimeRel = pTraceTimeMs / (pTraceTimeMs + sTraceTimeMs);
    p.sTimeRel = sTraceTimeMs / (pTraceTimeMs + sTraceTimeMs);
    p.timeTotal = pTraceTimeMs + sTraceTimeMs;
    p.avgNodesPerRay = (static_cast<f32>(totalTestedNodes) / static_cast<f32>(pRayCount + sRayCount));
    p.avgTrisPerRay = (static_cast<f32>(totalTestedTris) / static_cast<f32>(pRayCount + sRayCount));
    p.avgBVsPerRay = (static_cast<f32>(totalTestedBVs) / static_cast<f32>(pRayCount + sRayCount));

    auto const getNodeSize { [](backend::config::NodeLayout layout, backend::config::BV bv) -> u64 {
        switch (layout) {
        case backend::config::NodeLayout::eBVH2:
            switch (bv) {
            case backend::config::BV::eAABB:
                return sizeof(data_bvh::NodeBVH2_AABB_c);
            case backend::config::BV::eDOP14split:
                return sizeof(data_bvh::NodeBVH2_DOP3_c);
            case backend::config::BV::eOBB:
                return sizeof(data_bvh::NodeBVH2_OBB_c);
            case backend::config::BV::eDOP14:
                return sizeof(data_bvh::NodeBVH2_DOP14_c);
            case backend::config::BV::eSOBB_d:
                return sizeof(data_bvh::NodeBVH2_SOBB_c);
            case backend::config::BV::eSOBB_i32:
            case backend::config::BV::eSOBB_i48:
            case backend::config::BV::eSOBB_i64:
                return sizeof(data_bvh::NodeBVH2_SOBBi_c);
            default:;
            }
        default:;
        }
        return 0;
    } };
    auto const tSize { [](auto leafType) {
        static_cast<void>(leafType);
        return 48;
    } };

    p.memory = 0.f;
    p.memory += 1e-6f * p.statsBuild.rearrangement.nodeCountTotal * getNodeSize(bConfig.rearrangement.layout, bConfig.rearrangement.bv);
    p.memory += 1e-6f * .5f * (p.statsBuild.plocpp.nodeCountTotal + 1) * tSize(bConfig.rearrangement.bv);

    std::cout << " & " << p.name << " & ";
    for (size_t i = 0; i < stats.size(); i++) {
        switch (stats[i].stat) {
        case Stat::ePMRps:
            std::cout << std::format("{:.1f}", p.pMRps);
            if (stats[i].includeRelativeCol)
                std::cout << std::format(" & ({:.2f})", (p.pMRps / pRel.pMRps));
            break;
        case Stat::eSMRps:
            std::cout << std::format("{:.1f}", p.sMRps);
            if (stats[i].includeRelativeCol)
                std::cout << std::format(" & ({:.2f})", (p.sMRps / pRel.sMRps));
            break;
        case Stat::eTraceTimePrimary:
            std::cout << std::format("{:.2f}", p.pTimeRel);
            if (stats[i].includeRelativeCol)
                std::cout << std::format(" & ({:.2f})", (p.pTimeRel / pRel.pTimeRel));
            break;
        case Stat::eTraceTimeSecondary:
            std::cout << std::format("{:.2f}", p.sTimeRel);
            if (stats[i].includeRelativeCol)
                std::cout << std::format(" & ({:.2f})", (p.sTimeRel / pRel.sTimeRel));
            break;
        case Stat::eTraceTimeTotal:
            std::cout << std::format("{:.1f}", p.timeTotal);
            if (stats[i].includeRelativeCol)
                std::cout << std::format(" & ({:.2f})", (p.timeTotal / pRel.timeTotal));
            break;
        case Stat::eMemoryConsumption:
            std::cout << std::format("{:.2f}", p.memory);
            if (stats[i].includeRelativeCol)
                std::cout << std::format(" & ({:.2f})", (p.memory / pRel.memory));
            break;
        case Stat::eAvgTestedNodes:
            std::cout << std::format("{:.1f}", p.avgNodesPerRay);
            if (stats[i].includeRelativeCol)
                std::cout << std::format(" & ({:.2f})", (p.avgNodesPerRay / pRel.avgNodesPerRay));
            break;
        case Stat::eAvgTestedTriangles:
            std::cout << std::format("{:.1f}", p.avgTrisPerRay);
            if (stats[i].includeRelativeCol)
                std::cout << std::format(" & ({:.2f})", (p.avgTrisPerRay / pRel.avgTrisPerRay));
            break;
        case Stat::eAvgTestedBVs:
            std::cout << std::format("{:.1f}", p.avgBVsPerRay);
            if (stats[i].includeRelativeCol)
                std::cout << std::format(" & ({:.2f})", (p.avgBVsPerRay / pRel.avgBVsPerRay));
            break;
        case Stat::eBuildTimeTotal: {
            auto const t {
                p.statsBuild.plocpp.timeTotal
                + p.statsBuild.collapsing.timeTotal
                + p.statsBuild.transformation.timeTotal
                + p.statsBuild.rearrangement.timeTotal
            };
            auto const tRel {
                pRel.statsBuild.plocpp.timeTotal
                + pRel.statsBuild.collapsing.timeTotal
                + pRel.statsBuild.transformation.timeTotal
                + pRel.statsBuild.rearrangement.timeTotal
            };
            std::cout << std::format("{:.2f}", t);
            if (stats[i].includeRelativeCol)
                std::cout << std::format(" & ({:.2f})", (t / tRel));
        } break;
        case Stat::eBuildTimeBuild: {
            auto const t { p.statsBuild.plocpp.timeTotal };
            auto const tRel { pRel.statsBuild.plocpp.timeTotal };
            std::cout << std::format("{:.2f}", t);
            if (stats[i].includeRelativeCol)
                std::cout << std::format(" & ({:.2f})", (t / tRel));
        } break;
        case Stat::eBuildTimeCollapse: {
            auto const t { p.statsBuild.collapsing.timeTotal };
            auto const tRel { pRel.statsBuild.collapsing.timeTotal };
            std::cout << std::format("{:.2f}", t);
            if (stats[i].includeRelativeCol)
                std::cout << std::format(" & ({:.2f})", (t / tRel));
        } break;
        case Stat::eBuildTimeTransform: {
            auto const t { p.statsBuild.transformation.timeTotal };
            auto const tRel { pRel.statsBuild.transformation.timeTotal };
            std::cout << std::format("{:.2f}", t);
            if (stats[i].includeRelativeCol)
                std::cout << std::format(" & ({:.2f})", (t / tRel));
        } break;
        case Stat::eBuildTimeCompress: {
            auto const t { p.statsBuild.rearrangement.timeTotal };
            auto const tRel { pRel.statsBuild.rearrangement.timeTotal };
            std::cout << std::format("{:.2f}", t);
            if (stats[i].includeRelativeCol)
                std::cout << std::format(" & ({:.2f})", (t / tRel));
        } break;
        case Stat::eAverageLeafSize:
            // std::cout << std::format("{:.1f}", std::max(1.f, p.statsBuild.collapsing.leafSizeAvg));
            if (p.statsBuild.collapsing.leafSizeAvg > 0.f) {
                std::cout << std::format("{:.1f}", p.statsBuild.collapsing.leafSizeAvg);
                if (stats[i].includeRelativeCol) {
                    auto& base = pRel.statsBuild.collapsing.leafSizeAvg > 0 ? pRel.statsBuild.collapsing.leafSizeAvg : pRel.statsBuild.plocpp.leafSizeAvg;
                    std::cout << std::format(" & ({:.2f})", (p.statsBuild.collapsing.leafSizeAvg / base));
                }
            } else {
                std::cout << std::format("{:.1f}", p.statsBuild.plocpp.leafSizeAvg);
                if (stats[i].includeRelativeCol) {
                    auto& base = pRel.statsBuild.collapsing.leafSizeAvg > 0 ? pRel.statsBuild.collapsing.leafSizeAvg : pRel.statsBuild.plocpp.leafSizeAvg;
                    std::cout << std::format(" & ({:.2f})", (p.statsBuild.plocpp.leafSizeAvg / base));
                }
            }
            break;
        case Stat::eSA_leaf:
            std::cout << std::format("{:.1f}", p.statsBuild.rearrangement.saIntersect);
            if (stats[i].includeRelativeCol)
                std::cout << std::format(" & ({:.2f})", (p.statsBuild.rearrangement.saIntersect / pRel.statsBuild.rearrangement.saIntersect));
            break;
        case Stat::eSA_inner:
            std::cout << std::format("{:.1f}", p.statsBuild.rearrangement.saTraverse);
            if (stats[i].includeRelativeCol)
                std::cout << std::format(" & ({:.2f})", (p.statsBuild.rearrangement.saTraverse / pRel.statsBuild.rearrangement.saTraverse));
            break;
        default:
            std::cout << "[N/A]";
            if (stats[i].includeRelativeCol)
                std::cout << " & [N/A]";
        }
        std::cout << (i == (stats.size() - 1) ? " \\\\\n" : " & ");
    }
}

void Benchmark::StatsExport::headerL1(ToPrint const& tp)
{
    static auto constexpr L1 { [](Stat s) {
        switch (s) {
        case Stat::ePMRps:
        case Stat::eTraceTimePrimary:
            return "Primary";
        case Stat::eSMRps:
        case Stat::eTraceTimeSecondary:
            return "Secondary";
        case Stat::eAvgTestedNodes:
            return "Avg. nodes";
        case Stat::eAvgTestedTriangles:
            return "Avg. tris";
        case Stat::eAvgTestedBVs:
            return "Avg. BVs";
        case Stat::eBuildTimeTotal:
        case Stat::eTraceTimeTotal:
            return "Total";
        case Stat::eBuildTimeBuild:
            return "Build";
        case Stat::eBuildTimeCollapse:
            return "Collapse";
        case Stat::eBuildTimeTransform:
            return "Transform";
        case Stat::eBuildTimeCompress:
            return "Compress";
        case Stat::eAverageLeafSize:
            return "Avg. leaf";
        case Stat::eSA_leaf:
        case Stat::eSA_inner:
            return "Area";
        case Stat::eMemoryConsumption:
            return "Memory req.";
        default:
            return "n/a";
        }
    } };
    if (tp.includeRelativeCol)
        std::cout << "\\multicolumn{2}{l}{" << L1(tp.stat) << "} ";
    else
        std::cout << L1(tp.stat) << " ";
}
void Benchmark::StatsExport::headerL2(ToPrint const& tp)
{
    static auto constexpr L2 { [](Stat s) {
        switch (s) {
        case Stat::ePMRps:
        case Stat::eSMRps:
            return "(MRps)";
        case Stat::eTraceTimePrimary:
        case Stat::eTraceTimeSecondary:
            return "time (\\%)";
        case Stat::eAvgTestedNodes:
        case Stat::eAvgTestedTriangles:
        case Stat::eAvgTestedBVs:
            return "per ray";
        case Stat::eBuildTimeTotal:
        case Stat::eBuildTimeBuild:
        case Stat::eBuildTimeCollapse:
        case Stat::eBuildTimeTransform:
        case Stat::eBuildTimeCompress:
        case Stat::eTraceTimeTotal:
            return "(ms)";
        case Stat::eAverageLeafSize:
            return "size";
        case Stat::eSA_leaf:
            return "leaf";
        case Stat::eSA_inner:
            return "inner";
        case Stat::eMemoryConsumption:
            return "(MB)";
        default:
            return "n/a";
        }
    } };
    if (tp.includeRelativeCol)
        std::cout << "\\multicolumn{2}{r}{" << L2(tp.stat) << "} ";
    else
        std::cout << L2(tp.stat) << " ";
}

void Benchmark::StatsExport::val_f32(ToPrint const& tp)
{
    static_cast<void>(tp);
}

void Benchmark::StatsExport::val_i32(ToPrint const& tp)
{
    static_cast<void>(tp);
}
}
