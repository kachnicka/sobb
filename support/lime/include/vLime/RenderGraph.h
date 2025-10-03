#pragma once

#include <array>
#include <functional>
#include <limits>
#include <utility>

#include <berries/util/uid.h>
#include <vLime/CommandPool.h>
#include <vLime/Frame.h>
#include <vLime/Memory.h>
#include <vLime/vLime.h>

namespace lime::rg {

class Graph;
template<typename TaskType>
class Task;

class Rasterization;
class Commands;
class CommandsSync;
class PostExecutionSetup;

struct Resource;

namespace id_meta {
using Type = u32;
template<typename T>
struct ID {
    Type idGen { std::numeric_limits<Type>::max() };
    uid<T, Type> id;

    [[nodiscard]] Type get() const
    {
        return id.get();
    }
};
template<typename T>
inline bool operator==(ID<T> const& lhs, ID<T> const& rhs)
{
    return lhs.idGen == rhs.idGen && lhs.id == rhs.id;
}
template<typename TaskType>
using Task = ID<Task<TaskType>>;
}

namespace id {
using Rasterization = id_meta::Task<Rasterization>;
using Commands = id_meta::Task<Commands>;
using CommandsSync = id_meta::Task<CommandsSync>;
using PostExecutionSetup = id_meta::Task<PostExecutionSetup>;

using Resource = id_meta::ID<Resource>;
}

struct Resource {
    friend class Graph;

    Resource() = default;
    explicit Resource(vk::Format format)
        : format(format)
    {
    }

    vk::Format format { vk::Format::eUndefined };
    vk::SampleCountFlagBits msaaSamples { vk::SampleCountFlagBits::e1 };
    vk::ImageLayout finalLayout { vk::ImageLayout::eUndefined };

    enum class OperationFlagBits : u8 {
        eSampledFrom = 1 << 0,
        eLoadedFrom = 1 << 1,
        eCopiedFrom = 1 << 2,
        eCopiedTo = 1 << 3,
        eStored = 1 << 4,
        eAttachment = 1 << 5,
    };
    using OperationFlags = Flags<OperationFlagBits>;
    OperationFlags operationFlags {};

    [[nodiscard]] bool isReadFrom() const
    {
        return operationFlags.checkFlags(OperationFlagBits::eSampledFrom) || operationFlags.checkFlags(OperationFlagBits::eLoadedFrom) || operationFlags.checkFlags(OperationFlagBits::eCopiedFrom);
    }

    void BindToPhysicalResource(lime::Image::Detail const& detail)
    {
        BindToPhysicalResource(std::vector<lime::Image::Detail> { detail });
    }

    void BindToPhysicalResource(std::vector<lime::Image::Detail> detail)
    {
        extent = detail[0].extent;
        format = detail[0].format;
        physicalResourceDetail = std::move(detail);
    }

    id::Resource inheritExtentFromResource;
    vk::Extent3D extent;

    // to allow multiple buffering, resource might be backed by multiple images
    std::vector<lime::Image::Detail> physicalResourceDetail;

private:
    id::Resource physicalResourceManagedInternally;
};
GENERATE_FLAG_OPERATOR_OVERLOADS(Resource::OperationFlag);

struct RGBuffer {
    lime::Buffer* buffer;
};

[[nodiscard]] inline vk::ClearValue GetClearValue_NEW(vk::Format format)
{
    vk::ClearValue result {};

    if (img_util::IsDepth(format))
        result.depthStencil = vk::ClearDepthStencilValue { 1.f, 0 };
    else
        result.color = vk::ClearColorValue { std::array<f32, 4> { .05f, .05f, .05f, 1.f } };

    return result;
}

enum class Type : u8 {
    eRasterization,
    eCommands,
    eCommandsSync,
    ePostExecutionSetup,
};

struct RasterizationInfo {
    vk::RenderPassBeginInfo renderPassInfo;
    bool buildPipelines { true };
};

struct ResourceAccess {
    std::vector<id::Resource> samples;
    std::vector<id::Resource> loads;
    std::vector<id::Resource> stores;
};

template<typename TaskType>
class Task {
public:
    template<typename... Args>
    explicit Task(Args&&... args)
        : task(std::forward<Args>(args)...)
    {
    }

    TaskType task;
    ResourceAccess resource;

    template<typename... Args>
    void RegisterExecutionCallback(Args&&... args)
    {
        task.RegisterExecutionCallback(std::forward<Args>(args)...);
    }
};

class Rasterization {
    std::vector<vk::AttachmentDescription> attachments;
    std::vector<vk::AttachmentReference> colorAttachmentRefs;
    std::vector<vk::AttachmentReference> resolveAttachmentRefs;
    vk::AttachmentReference depthStencilAttachmentRef;

    // uses the same indexing as attachment references to description vector
    std::vector<id::Resource> mapLogicalToPhysicalResource;
    std::function<void(vk::CommandBuffer, RasterizationInfo const& info)> recordCommandsCallback;
    mutable bool buildPipelines { true };

public:
    vk::UniqueRenderPass renderPass;
    std::vector<vk::UniqueFramebuffer> framebuffer;
    vk::Extent2D extent;
    std::vector<vk::ClearValue> clearValues;

    static Type GetType() { return Type::eRasterization; }

    struct
    {
        bool resolve { true };
        vk::SampleCountFlagBits sampleCount { vk::SampleCountFlagBits::e1 };

        [[nodiscard]] bool resolveRequired() const
        {
            return resolve && (sampleCount != vk::SampleCountFlagBits::e1);
        }
    } msaa;

public:
    void SetupLogicalResources(std::vector<Resource>& resources, ResourceAccess& resource, vk::Device d)
    {
        for (auto const& idStored : resource.stores) {
            auto& imgStored { resources[idStored.get()] };

            // attachment type (color, depth, resolve)
            if (img_util::IsDepth(imgStored.format) || img_util::IsStencil(imgStored.format)) {
                depthStencilAttachmentRef.attachment = csize<u32>(attachments);
                depthStencilAttachmentRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
                imgStored.msaaSamples = msaa.sampleCount;
            } else {
                if (!msaa.resolveRequired()) {
                    imgStored.msaaSamples = msaa.sampleCount;
                    colorAttachmentRefs.emplace_back(csize<u32>(attachments), vk::ImageLayout::eColorAttachmentOptimal);
                } else
                    resolveAttachmentRefs.emplace_back(csize<u32>(attachments), vk::ImageLayout::eColorAttachmentOptimal);
            }

            // create default attachment
            mapLogicalToPhysicalResource.emplace_back(idStored);
            auto& attachment { attachments.emplace_back(
                vk::AttachmentDescriptionFlags {},
                imgStored.format, imgStored.msaaSamples,
                vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
                vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
                vk::ImageLayout::eUndefined, imgStored.finalLayout) };

            // resolve load/store operations and layouts
            if (std::ranges::count(resource.loads, idStored)) {
                attachment.loadOp = vk::AttachmentLoadOp::eLoad;

                attachment.initialLayout = vk::ImageLayout::eColorAttachmentOptimal;
                imgStored.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;
            } else
                attachment.loadOp = vk::AttachmentLoadOp::eClear;

            if (imgStored.isReadFrom() || imgStored.finalLayout == vk::ImageLayout::ePresentSrcKHR)
                attachment.storeOp = vk::AttachmentStoreOp::eStore;
        }

        // create high sample color attachments to be resolved
        for (auto const& resolved : resolveAttachmentRefs) {
            id::Resource idColor { .id = decltype(id::Resource::id) { csize<id_meta::Type>(resources) } };
            auto const idResolve { mapLogicalToPhysicalResource[resolved.attachment] };

            colorAttachmentRefs.emplace_back(csize<u32>(attachments), vk::ImageLayout::eColorAttachmentOptimal);
            mapLogicalToPhysicalResource.emplace_back(idColor);

            resources.emplace_back(resources[idResolve.get()].format);
            resources.back().msaaSamples = msaa.sampleCount;
            resources.back().inheritExtentFromResource = idResolve;

            auto& attachment { attachments.emplace_back(
                vk::AttachmentDescriptionFlags {},
                resources.back().format, msaa.sampleCount,
                vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
                vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
                vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal) };

            if (std::ranges::count(resource.loads, idResolve))
                attachment.loadOp = vk::AttachmentLoadOp::eLoad;
            else
                attachment.loadOp = vk::AttachmentLoadOp::eClear;

            resource.stores.emplace_back(idColor);
        }

        renderPass = createRenderPass(d);
    }

    void SetupPhysicalResources(std::vector<Resource> const& resources, vk::Device d)
    {
        framebuffer.clear();
        vk::FramebufferCreateInfo framebufferCreateInfo;
        std::vector<vk::ImageView> framebufferAttachments;

        // This implementation allows one resource to be backed by multiple physical resources
        id::Resource multipleBufferedResourceId;
        size_t multipleBufferedResourcePosition { 0 };

        extent.width = std::numeric_limits<u32>::max();
        extent.height = std::numeric_limits<u32>::max();

        for (id_meta::Type i { 0 }; i < static_cast<id_meta::Type>(attachments.size()); i++) {
            auto const& img { resources[mapLogicalToPhysicalResource[i].get()] };
            framebufferAttachments.emplace_back(img.physicalResourceDetail[0].imageView);
            clearValues.emplace_back(GetClearValue_NEW(img.format));

            if (img.physicalResourceDetail.size() > 1) {
                multipleBufferedResourceId = mapLogicalToPhysicalResource[i];
                multipleBufferedResourcePosition = framebufferAttachments.size() - 1;
            }

            extent.width = std::min(extent.width, img.physicalResourceDetail[0].extent.width);
            extent.height = std::min(extent.height, img.physicalResourceDetail[0].extent.height);
        }

        framebufferCreateInfo.attachmentCount = csize<u32>(framebufferAttachments);
        framebufferCreateInfo.pAttachments = framebufferAttachments.data();
        framebufferCreateInfo.renderPass = *renderPass;
        framebufferCreateInfo.layers = 1;
        framebufferCreateInfo.width = extent.width;
        framebufferCreateInfo.height = extent.height;

        if (!multipleBufferedResourceId.id.isValid())
            framebuffer.emplace_back(check(d.createFramebufferUnique(framebufferCreateInfo)));
        else
            for (auto const& detail : resources[multipleBufferedResourceId.get()].physicalResourceDetail) {
                framebufferAttachments[multipleBufferedResourcePosition] = detail.imageView;
                framebuffer.emplace_back(check(d.createFramebufferUnique(framebufferCreateInfo)));
            }
    }

    void RegisterExecutionCallback(std::function<void(vk::CommandBuffer, RasterizationInfo const& info)> callback)
    {
        recordCommandsCallback = std::move(callback);
    }

    void RecordCommands(vk::CommandBuffer commandBuffer, size_t multipleBufferingId = 0) const
    {
        assert(recordCommandsCallback);

        RasterizationInfo info {
            .renderPassInfo = {
                .renderPass = renderPass.get(),
                .framebuffer = framebuffer.size() == 1 ? framebuffer.back().get() : framebuffer[multipleBufferingId].get(),
                .renderArea = { {}, extent },
                .clearValueCount = csize<u32>(clearValues),
                .pClearValues = clearValues.data(),
            },
            .buildPipelines = buildPipelines,
        };
        recordCommandsCallback(commandBuffer, info);

        buildPipelines = false;
    }

private:
    [[nodiscard]] vk::UniqueRenderPass createRenderPass(vk::Device d) const
    {
        vk::SubpassDescription subpass;
        subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;

        if (!colorAttachmentRefs.empty()) {
            subpass.colorAttachmentCount = csize<u32>(colorAttachmentRefs);
            subpass.pColorAttachments = colorAttachmentRefs.data();
        }
        if (!resolveAttachmentRefs.empty()) {
            subpass.pResolveAttachments = resolveAttachmentRefs.data();
        }
        if (depthStencilAttachmentRef.layout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
            subpass.pDepthStencilAttachment = &depthStencilAttachmentRef;
        }

        std::array<vk::SubpassDependency, 2> dependencies;

        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
        dependencies[0].dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        dependencies[0].srcAccessMask = vk::AccessFlagBits::eMemoryRead;
        dependencies[0].dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
        dependencies[0].dependencyFlags = vk::DependencyFlagBits::eByRegion;

        dependencies[1].srcSubpass = 0;
        dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[1].srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        dependencies[1].dstStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
        dependencies[1].srcAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
        dependencies[1].dstAccessMask = vk::AccessFlagBits::eMemoryRead;
        dependencies[1].dependencyFlags = vk::DependencyFlagBits::eByRegion;

        vk::RenderPassCreateInfo renderPassInfo;
        renderPassInfo.attachmentCount = csize<u32>(attachments);
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = csize<u32>(dependencies);
        renderPassInfo.pDependencies = dependencies.data();

        return check(d.createRenderPassUnique(renderPassInfo));
    }
};

class Commands {
    std::function<void(vk::CommandBuffer)> recordCommandsCallback;

public:
    static Type GetType() { return Type::eCommands; }

    void RegisterExecutionCallback(std::function<void(vk::CommandBuffer)> callback)
    {
        recordCommandsCallback = std::move(callback);
    }

    void RecordCommands(vk::CommandBuffer commandBuffer, size_t multipleBufferingId = 0) const
    {
        assert(recordCommandsCallback);
        static_cast<void>(multipleBufferingId);
        recordCommandsCallback(commandBuffer);
    }
};

class CommandsSync {
    std::function<void(vk::CommandBuffer)> recordCommandsCallback;

public:
    static Type GetType() { return Type::eCommandsSync; }

    void RegisterExecutionCallback(std::function<void(vk::CommandBuffer)> callback)
    {
        recordCommandsCallback = std::move(callback);
    }

    void RecordCommands(vk::CommandBuffer commandBuffer, size_t multipleBufferingId = 0) const
    {
        assert(recordCommandsCallback);
        static_cast<void>(multipleBufferingId);
        recordCommandsCallback(commandBuffer);
    }
};

class PostExecutionSetup {
    std::function<void(Graph const& rg)> recordCommandsCallback;

public:
    static Type GetType() { return Type::ePostExecutionSetup; }

    void RegisterExecutionCallback(std::function<void(Graph const& rg)> callback)
    {
        recordCommandsCallback = std::move(callback);
    }

    void RecordCommands(Graph const& rg) const
    {
        assert(recordCommandsCallback);
        recordCommandsCallback(rg);
    }
};

class Graph {
    vk::Device d;
    lime::MemoryManager& memory;

    lime::commands::TransientPool transientPool;
    vk::UniqueFence fence;

    id_meta::Type rgGeneration { 0 };
    struct {
        std::vector<Task<Rasterization>> rasterization;
        std::vector<Task<Commands>> commands;
        std::vector<Task<CommandsSync>> commandsSync;
        std::vector<Task<PostExecutionSetup>> postExecutionSetup;
    } tasks;

    using TaskIndex = std::pair<Type, id_meta::Type>;
    std::vector<TaskIndex> orderedTasks;
    std::vector<Resource> resources;
    std::vector<Image> managedImages;
    bool compiled { false };

public:
    explicit Graph(vk::Device d, lime::MemoryManager& memory, lime::Queue transientCommandsQueue)
        : d(d)
        , memory(memory)
        , transientPool(d, transientCommandsQueue)
        , fence(lime::FenceFactory(d))
    {
    }

    [[nodiscard]] bool isCompiled() const
    {
        return compiled;
    }

    void reset()
    {
        tasks.rasterization.clear();
        tasks.commands.clear();
        tasks.commandsSync.clear();
        tasks.postExecutionSetup.clear();

        orderedTasks.clear();
        resources.clear();
        managedImages.clear();

        compiled = false;
        memory.cleanUp();

        rgGeneration++;
    }

    template<typename TaskType>
    id_meta::Task<TaskType> AddTask()
    {
        auto& taskVector { getTaskVector<TaskType>() };

        id_meta::Task<TaskType> id {
            .idGen = rgGeneration,
            .id = decltype(id_meta::Task<TaskType>::id) { csize<id_meta::Type>(taskVector) },
        };
        orderedTasks.emplace_back(TaskType::GetType(), id.get());
        taskVector.emplace_back();
        return id;
    }

    template<typename TaskType>
    Task<TaskType>& GetTask(id_meta::Task<TaskType> id)
    {
        auto& taskVector { getTaskVector<TaskType>() };
        assert(id.get() < taskVector.size());
        return taskVector[id.get()];
    }

    id::Resource AddResource(vk::Format format = vk::Format::eUndefined)
    {
        resources.emplace_back(format);
        return id::Resource {
            .idGen = rgGeneration,
            .id = decltype(id::Resource::id) { csize<id_meta::Type>(resources) - 1 },
        };
    }

    Resource& GetResource(id::Resource id)
    {
        assert(id.get() < resources.size());
        return resources[id.get()];
    }

    template<typename TaskType>
    void SetTaskResourceOperation(id_meta::Task<TaskType> idTask, id::Resource idResource, Resource::OperationFlags operations)
    {
        auto& taskResource { GetTask(idTask).resource };

        if (operations.checkFlags(Resource::OperationFlagBits::eSampledFrom))
            taskResource.samples.emplace_back(idResource);
        if (operations.checkFlags(Resource::OperationFlagBits::eLoadedFrom))
            taskResource.loads.emplace_back(idResource);
        if (operations.checkFlags(Resource::OperationFlagBits::eStored))
            taskResource.stores.emplace_back(idResource);
        if (operations.checkFlags(Resource::OperationFlagBits::eAttachment))
            taskResource.stores.emplace_back(idResource);

        GetResource(idResource).operationFlags |= operations;
    }

    Image& GetManagedBackingResource(id::Resource id)
    {
        auto const& r { GetResource(id) };
        assert(isValid(r.physicalResourceManagedInternally));
        return managedImages[r.physicalResourceManagedInternally.get()];
    }

    void ResetBoundPhysicalResources()
    {
        for (auto& r : resources)
            r.physicalResourceDetail = {};
        managedImages.clear();
    }

    template<typename T>
    [[nodiscard]] bool isValid(id_meta::ID<T> id) const
    {
        return rgGeneration == id.idGen && id.id.isValid();
    }

    void Compile()
    {
        // creates logical representation and relationships between resources; back to front
        for (auto it { orderedTasks.rbegin() }; it != orderedTasks.rend(); ++it) {
            auto [taskType, taskIndex] { *it };
            switch (taskType) {
            case Type::eRasterization: {
                auto& myTask { tasks.rasterization[taskIndex] };
                myTask.task.SetupLogicalResources(resources, myTask.resource, d);
            } break;
            case Type::eCommands:
            case Type::eCommandsSync:
            case Type::ePostExecutionSetup:
                break;
            }
        }
        SetupPhysicalResources();

        compiled = true;
    }

    void SetupPhysicalResources()
    {
        // allocate missing (internally managed) physical resources
        for (auto& r : resources) {
            // already backed by valid image
            if (!r.physicalResourceDetail.empty())
                continue;

            vk::ImageUsageFlags usage;
            if (img_util::IsDepth(r.format) || img_util::IsStencil(r.format))
                usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
            else
                usage = vk::ImageUsageFlagBits::eColorAttachment;

            if (r.operationFlags.checkFlags(Resource::OperationFlagBits::eSampledFrom))
                usage |= vk::ImageUsageFlagBits::eSampled;
            if (r.operationFlags.checkFlags(Resource::OperationFlagBits::eCopiedFrom))
                usage |= vk::ImageUsageFlagBits::eTransferSrc;
            if (r.operationFlags.checkFlags(Resource::OperationFlagBits::eStored))
                usage |= vk::ImageUsageFlagBits::eStorage;

            if (!usage)
                usage = vk::ImageUsageFlagBits::eTransientAttachment;

            vk::ImageCreateInfo cInfo {
                .flags = {},
                .imageType = vk::ImageType::e2D,
                .format = r.format,
                .extent = r.extent,
                .mipLevels = 1,
                .arrayLayers = 1,
                .samples = r.msaaSamples,
                .tiling = vk::ImageTiling::eOptimal,
                .usage = usage,
                .initialLayout = vk::ImageLayout::eUndefined,
            };

            if (isValid(r.inheritExtentFromResource))
                cInfo.extent = resources[r.inheritExtentFromResource.get()].physicalResourceDetail[0].extent;
            assert(cInfo.extent != vk::Extent3D {});

            r.physicalResourceManagedInternally = id::Resource {
                .idGen = rgGeneration,
                .id = decltype(id::Resource::id) { csize<id_meta::Type>(managedImages) },
            };
            managedImages.push_back(memory.alloc({ .memoryUsage = DeviceMemoryUsage::eDeviceOptimal }, cInfo, std::format("rg_internal_texture_{}", managedImages.size()).c_str()));
            managedImages.back().CreateImageView();
            r.physicalResourceDetail.emplace_back(managedImages.back());
        }

        for (auto& task : tasks.rasterization)
            task.task.SetupPhysicalResources(resources, d);
    }

    void SetupExecution(vk::CommandBuffer commandBuffer, size_t multipleBufferingId = 0)
    {
        vk::CommandBuffer commandBufferSync;

        for (auto const& [taskType, taskIndex] : orderedTasks)
            if (taskType == Type::eCommandsSync) {
                commandBufferSync = transientPool.BeginCommands();
                tasks.commandsSync[taskIndex].task.RecordCommands(commandBufferSync, multipleBufferingId);
                transientPool.EndSubmitCommands(commandBufferSync, fence.get());
            }
        for (auto const& [taskType, taskIndex] : orderedTasks) {
            switch (taskType) {
            case Type::eRasterization:
                tasks.rasterization[taskIndex].task.RecordCommands(commandBuffer, multipleBufferingId);
                break;
            case Type::eCommands:
                tasks.commands[taskIndex].task.RecordCommands(commandBuffer, multipleBufferingId);
                break;
            default:
                break;
            }
        }
    }

private:
    template<typename TaskType>
    std::vector<Task<TaskType>>& getTaskVector()
    {
        if constexpr (std::is_same_v<TaskType, Rasterization>)
            return tasks.rasterization;
        else if constexpr (std::is_same_v<TaskType, Commands>)
            return tasks.commands;
        else if constexpr (std::is_same_v<TaskType, CommandsSync>)
            return tasks.commandsSync;
        else if constexpr (std::is_same_v<TaskType, PostExecutionSetup>)
            return tasks.postExecutionSetup;
        else
            static_assert(debug::always_false<TaskType>::value, "Invalid render graph task_old_style type.");
    }

    template<typename TaskType>
    std::vector<Task<TaskType>> const& getTaskVector() const
    {
        if constexpr (std::is_same_v<TaskType, Rasterization>)
            return tasks.rasterization;
        else if constexpr (std::is_same_v<TaskType, Commands>)
            return tasks.commands;
        else if constexpr (std::is_same_v<TaskType, CommandsSync>)
            return tasks.commandsSync;
        else if constexpr (std::is_same_v<TaskType, PostExecutionSetup>)
            return tasks.postExecutionSetup;
        else
            static_assert(debug::always_false<TaskType>::value, "Invalid render graph task_old_style type.");
    }
};
}
