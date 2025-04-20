#include "torpedo/rendering/Engine.h"
#include "torpedo/rendering/LogUtils.h"

#include <torpedo/bootstrap/DeviceBuilder.h>
#include <torpedo/bootstrap/PhysicalDeviceSelector.h>

void tpd::Engine::init(
    const vk::Instance instance,
    const vk::SurfaceKHR surface,
    std::vector<const char*>&& rendererDeviceExtensions)
{
    auto extensions = getDeviceExtensions();
    extensions.insert(extensions.end(),
        std::make_move_iterator(rendererDeviceExtensions.begin()),
        std::make_move_iterator(rendererDeviceExtensions.end()));

    // Init physical device and create the logical device
    const auto [physicalDevice, graphicsIndex, transferIndex, presentIndex, computeIndex] = pickPhysicalDevice(extensions, instance, surface);
    _physicalDevice = physicalDevice;
    _device = createDevice(extensions, { graphicsIndex, transferIndex, computeIndex, presentIndex });

    PLOGI << "Found a suitable device for " << getName() << ": " << _physicalDevice.getProperties().deviceName.data();

    // Create a device allocator
    _vmaAllocator = vma::Builder()
        .flags(VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT | VMA_ALLOCATOR_CREATE_EXT_MEMORY_PRIORITY_BIT)
        .vulkanApiVersion(VK_API_VERSION_1_3)
        .build(instance, _physicalDevice, _device);

    PLOGI << "Using VMA API version: 1.3";
    PLOGD << "VMA created with the following flags (2):";
    PLOGD << " - VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT";
    PLOGD << " - VMA_ALLOCATOR_CREATE_EXT_MEMORY_PRIORITY_BIT";

    // Init all queue-related info
    _graphicsFamilyIndex = graphicsIndex;
    _transferFamilyIndex = transferIndex;
    _computeFamilyIndex  = computeIndex;
    _presentFamilyIndex  = presentIndex;
}

std::vector<const char*> tpd::Engine::getDeviceExtensions() const {
    auto extensions = std::vector{
        vk::EXTMemoryBudgetExtensionName,    // help VMA estimate memory budget more accurately
        vk::EXTMemoryPriorityExtensionName,  // incorporate memory priority to the allocator
    };
    utils::logDebugExtensions("Device", Engine::getName(), extensions);
    return extensions;
}

tpd::PhysicalDeviceSelection tpd::Engine::pickPhysicalDevice(
    const std::vector<const char*>& deviceExtensions,
    const vk::Instance instance,
    const vk::SurfaceKHR surface) const
{
    auto selector = PhysicalDeviceSelector()
        .requestGraphicsQueueFamily();

    if (_renderer->supportSurfaceRendering()) {
        selector.requestPresentQueueFamily(surface);
    }

    const auto selection = selector.select(instance, deviceExtensions);
    PLOGD << "Queue family indices selected:";
    PLOGD << " - Graphics: " << selection.graphicsQueueFamilyIndex;
    PLOGD << " - Transfer: " << selection.transferQueueFamilyIndex;
    if (_renderer->supportSurfaceRendering()) {
        PLOGD << " - Present:  " << selection.presentQueueFamilyIndex;
    }

    return selection;
}

vk::Device tpd::Engine::createDevice(
    const std::vector<const char*>& deviceExtensions,
    const std::initializer_list<uint32_t> queueFamilyIndices) const
{
    return DeviceBuilder()
        .queueFamilyIndices(queueFamilyIndices)
        .build(_physicalDevice, deviceExtensions);
}

void tpd::Engine::destroy() noexcept {
    if (_initialized) {
        _initialized = false;
        vma::destroy(_vmaAllocator);

        // It's possible that the Context has bound to another Engine, in which case resetting the renderer
        // would destroy the incorrect resources. We check the pointer state to tell if this has happened.
        if (_renderer) {
            _renderer->resetEngine();
            _renderer = nullptr;
        }

        _device.destroy();
        _device = nullptr;
        _physicalDevice = nullptr;
    }
}
