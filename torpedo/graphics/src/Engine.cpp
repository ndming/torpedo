#include "torpedo/graphics/Engine.h"
#include "renderer/raster/ForwardRenderer.h"

#include <torpedo/bootstrap/InstanceBuilder.h>

#include <plog/Log.h>

std::unique_ptr<tpd::Engine> tpd::Engine::create(PFN_vkDebugUtilsMessengerCallbackEXT debugCallback) {
    return std::unique_ptr<Engine>(new Engine{ debugCallback });
}

tpd::Engine::Engine(PFN_vkDebugUtilsMessengerCallbackEXT debugCallback) noexcept {
    createInstance(debugCallback);
#ifndef NDEBUG
    createDebugMessenger(debugCallback);
#endif
}

#ifndef NDEBUG
// =====================================================================================================================
// DEBUG MESSENGER
// =====================================================================================================================

VKAPI_ATTR vk::Bool32 VKAPI_CALL tordemoDebugCallback(
    const VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    [[maybe_unused]] const VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    [[maybe_unused]] void* const userData
) {
    switch (messageSeverity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            PLOGV << pCallbackData->pMessage; break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            PLOGI << pCallbackData->pMessage; break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            PLOGW << pCallbackData->pMessage; break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            PLOGE << pCallbackData->pMessage; break;
        default: break;
    }

    return vk::False;
}

void tpd::Engine::createDebugMessenger(PFN_vkDebugUtilsMessengerCallbackEXT debugCallback) {
    const auto debugInfo = bootstrap::createDebugInfo(debugCallback ? debugCallback : tordemoDebugCallback);

    if (bootstrap::createDebugUtilsMessenger(_instance, &debugInfo, &_debugMessenger) == vk::Result::eErrorExtensionNotPresent) {
        throw std::runtime_error("Engine - Failed to set up a debug messenger");
    }
}
#endif

// =====================================================================================================================
// VULKAN INSTANCE
// =====================================================================================================================

void tpd::Engine::createInstance(PFN_vkDebugUtilsMessengerCallbackEXT debugCallback) {
    auto instanceCreateFlags = vk::InstanceCreateFlags{};

#ifdef __APPLE__
    // Beginning with the 1.3.216 Vulkan SDK, the VK_KHR_PORTABILITY_subset extension is mandatory
    // for macOS with the latest MoltenVK SDK
    requiredExtensions.push_back(vk::KHRPortabilityEnumerationExtensionName);
    instanceCreateFlags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
#endif

    _instance = InstanceBuilder()
        .applicationInfo(bootstrap::createApplicationInfo("torpedo", vk::makeApiVersion(0, 1, 3, 0)))
#ifndef NDEBUG
        .debugInfo(bootstrap::createDebugInfo(debugCallback ? debugCallback : tordemoDebugCallback))
        .layers({ "VK_LAYER_KHRONOS_validation" })
#endif
        .extensions(getRequiredExtensions())
        .build(instanceCreateFlags);

}

std::vector<const char*> tpd::Engine::getRequiredExtensions() {
    uint32_t glfwExtensionCount{ 0 };
    const auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    return { glfwExtensions, glfwExtensions + glfwExtensionCount };
}

// =====================================================================================================================
// RENDERER CREATION
// =====================================================================================================================

std::unique_ptr<tpd::Renderer> tpd::Engine::createRenderer(const RenderEngine renderEngine, void* nativeWindow) {
    // Have the renderer pick physical device and create logical device and queues
    auto renderer = std::unique_ptr<Renderer>{};

    // Pick the right renderer impl based on renderEngine
    using enum RenderEngine;
    using namespace tpd::renderer;
    switch (renderEngine) {
    case Forward:
        if (!nativeWindow) throw std::runtime_error("Engine - nativeWindow is null, did you forget to give createRenderer a native window pointer?");
        renderer = std::make_unique<ForwardRenderer>(static_cast<GLFWwindow*>(nativeWindow));
        break;
    default:
        throw std::runtime_error("Engine - Unsupported renderer engine");
    }

    renderer->onCreate(_instance, ENGINE_DEVICE_EXTENSIONS);

    // We will need these objects to handle Vulkan memory operations
    _device = renderer->_device;
    _transferQueue = renderer->_graphicsQueue;

    // Init the resource allocator and create a transfer command pool
    initResourceAllocator(renderer->_physicalDevice);
    createTransferCommandPool(renderer->_graphicsQueueFamily);

    // Give the renderer a reference to the allocator and let it initialize resources
    renderer->setAllocator(_allocator.get());
    renderer->onInitialize();

    return renderer;
}

void tpd::Engine::initResourceAllocator(const vk::PhysicalDevice physicalDevice) {
    _allocator = ResourceAllocator::Builder()
        .flags(VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT | VMA_ALLOCATOR_CREATE_EXT_MEMORY_PRIORITY_BIT)
        .vulkanApiVersion(vk::makeApiVersion(0, 1, 3, 0))
        .build(_instance, physicalDevice, _device);
}

void tpd::Engine::createTransferCommandPool(const uint32_t transferQueueFamily) {
    auto transferPoolInfo = vk::CommandPoolCreateInfo{};
    transferPoolInfo.flags = vk::CommandPoolCreateFlagBits::eTransient;
    transferPoolInfo.queueFamilyIndex = transferQueueFamily;
    _transferCommandPool = _device.createCommandPool(transferPoolInfo);
}

void tpd::Engine::destroyRenderer(const std::unique_ptr<Renderer>& renderer) {
    renderer->onDestroy(_instance);
    _device.destroyCommandPool(_transferCommandPool);
    _allocator.reset();
    _device.destroy();
}

vk::CommandBuffer tpd::Engine::beginSingleTimeTransferCommands() const {
    const auto allocInfo = vk::CommandBufferAllocateInfo{ _transferCommandPool, vk::CommandBufferLevel::ePrimary, 1 };
    const auto commandBuffer = _device.allocateCommandBuffers(allocInfo)[0];

    // Use the command buffer once and wait with returning from the function until the copy operation has finished
    constexpr auto beginInfo = vk::CommandBufferBeginInfo{ vk::CommandBufferUsageFlagBits::eOneTimeSubmit };
    commandBuffer.begin(beginInfo);

    return commandBuffer;
}

void tpd::Engine::endSingleTimeTransferCommands(const vk::CommandBuffer commandBuffer) const {
    commandBuffer.end();
    _transferQueue.submit(vk::SubmitInfo{ {}, {}, {}, 1, &commandBuffer });
    _transferQueue.waitIdle();
    _device.freeCommandBuffers(_transferCommandPool, 1, &commandBuffer);
}

void tpd::Engine::copyBuffer(const vk::Buffer src, const vk::Buffer dst, const vk::BufferCopy& copyInfo) const {
    const auto cmdBuffer = beginSingleTimeTransferCommands();
    cmdBuffer.copyBuffer(src, dst, copyInfo);
    endSingleTimeTransferCommands(cmdBuffer);
}

tpd::Engine::~Engine() {
#ifndef NDEBUG
    bootstrap::destroyDebugUtilsMessenger(_instance, _debugMessenger);
#endif
    _instance.destroy();
}
