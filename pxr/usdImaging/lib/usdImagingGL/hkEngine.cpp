#include "hkEngine.h"

#define VULKAN_HPP_NO_EXCEPTIONS

#include <vulkan/vulkan.hpp>

namespace {
    template<typename T, typename... Args>
    std::unique_ptr<T> make_unique(Args&& ... args)
    {
        return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
    }

    namespace {
        int get_device_type_order(vk::PhysicalDeviceType device_type)
        {
            switch (device_type)
            {
                case vk::PhysicalDeviceType::eIntegratedGpu:
                    return 1;
                case vk::PhysicalDeviceType::eDiscreteGpu:
                    return 0;
                case vk::PhysicalDeviceType::eVirtualGpu:
                    return 2;
                case vk::PhysicalDeviceType::eCpu:
                    return 3;
                default:
                    return std::numeric_limits<int>::max();
            }
        }
    }

    template<typename T>
    void memset_t(T& target)
    {
        memset(&target, 0, sizeof(T));
    }

    template<typename T>
    void memset_t(T* target)
    {
        memset(target, 0, sizeof(T));
    }
}

struct UsdImagingGLHkEngine::Impl {
    using gpu_devices_t = std::vector<std::pair<vk::PhysicalDevice, vk::PhysicalDeviceProperties>>;
    using queue_families_ordering_t = std::vector<std::pair<uint32_t, vk::QueueFamilyProperties>>;

    Impl() : is_valid(false)
    {
        // Creating instance
        vk::ApplicationInfo app_info{
            "Usd Imaging", 1, "hk", 1, VK_API_VERSION_1_0
        };
        vk::InstanceCreateInfo instance_info{
            vk::InstanceCreateFlags(), &app_info // rest is fine with their default values for now
        };
        auto instance_result = vk::createInstance(instance_info);
        if (instance_result.result != vk::Result::eSuccess) {
            TF_WARN("[hk] Error creating instance! (%u)",
                    static_cast<unsigned int>(instance_result.result));
            return;
        }
        instance = instance_result.value;

        // Selectring the right physical_device to use
        auto physical_devices_result = instance.enumeratePhysicalDevices();
        if (physical_devices_result.result != vk::Result::eSuccess || physical_devices_result.value.size() == 0) {
            TF_WARN("[hk] No Vulkan physical physical device present. (%u)",
                    static_cast<unsigned int>(physical_devices_result.result));
            return;
        }

        for (auto current_device : physical_devices_result.value) {
            physical_devices.emplace_back(current_device, current_device.getProperties());
        }
        // We need a stable sort so we can maintain the discovery order, so in case of multiple physical GPUs
        // the first one will be the one connected to the monitor (hopefully)
        std::stable_sort(physical_devices.begin(), physical_devices.end(), [](const gpu_devices_t::value_type& a,
                                                            const gpu_devices_t::value_type& b) -> bool {
            if (a.second.deviceType == b.second.deviceType)
                return false;
            else
                return get_device_type_order(a.second.deviceType) < get_device_type_order(b.second.deviceType);
        });
        physical_device = physical_devices.front().first;
        physical_device_properties = physical_devices.front().second;
        physical_device_features = physical_device.getFeatures();

        // We have to find the queue family, that supports the most features we want,
        // and offers the most queues
        // One trick, we also have to track the queue id, or else we won't know which of the queues
        // query from the device (that works based on position from the official vector)
        // The ordering policy is quire simple, we look at the number of queues supported first
        // then having the most variety in supported features.
        auto device_queue_family_properties = physical_device.getQueueFamilyProperties();
        if (device_queue_family_properties.size() == 0) {
            TF_WARN("[hk] No queue families found for the current device!");
            return;
        }
        queue_families_ordering_t queue_family_properties_ordered;
        queue_family_properties_ordered.reserve(device_queue_family_properties.size());
        int queue_family_counter = 0;
        for (const auto& queue_family : device_queue_family_properties) {
            queue_family_properties_ordered.emplace_back(queue_family_counter++, queue_family);
        }
        std::sort(queue_family_properties_ordered.begin(), queue_family_properties_ordered.end(),
            [](const queue_families_ordering_t::value_type& a, const queue_families_ordering_t::value_type& b) {
                if (a.second.queueCount > b.second.queueCount)
                    return true;
                else if (static_cast<unsigned int>(a.second.queueFlags) & 0x0000000F >
                    static_cast<unsigned int>(b.second.queueFlags) & 0x0000000F)
                    return true;
                else
                    return a.first < b.first;
            });
        // Then we find the first queue, that supports graphics, compute and transfer at the same time, because
        // we require that.
        // TODO: check the specification if this is required.
        selected_queue_familiy_index = std::numeric_limits<uint32_t>::max();
        for (const auto& queue_family : queue_family_properties_ordered) {
            if (queue_family.second.queueFlags & vk::QueueFlagBits::eGraphics &&
                queue_family.second.queueFlags & vk::QueueFlagBits::eCompute &&
                queue_family.second.queueFlags & vk::QueueFlagBits::eTransfer) {
                queue_family_properties = queue_family.second;
                selected_queue_familiy_index = queue_family.first;
                break;
            }
        }
        if (selected_queue_familiy_index == std::numeric_limits<uint32_t>::max()) {
            TF_WARN("[hk] Couldn't find a queue that fits our requirements!");
            return;
        }

        // We are just creating one queue for now
        float queue_priorities[] = {1.0f};
        vk::DeviceQueueCreateInfo device_queue_info{
            vk::DeviceQueueCreateFlags(), selected_queue_familiy_index, 1, queue_priorities
        };
        // No features, extensions or layers are enabled at this moment
        auto device_result = physical_device.createDevice(
            {
                vk::DeviceCreateFlags(),
                1, &device_queue_info
            });
        if (device_result.result != vk::Result::eSuccess) {
            TF_WARN("[hk] Error creating device! (%u)", static_cast<unsigned int>(device_result.result));
            return;
        }
        device = device_result.value;
        queue = device.getQueue(selected_queue_familiy_index, 0);

        is_valid = true;
    }

    ~Impl()
    {
        if (device)
            device.destroy();
        if (instance)
            instance.destroy();
    }

    vk::Device device;
    vk::Queue queue;
    vk::Instance instance;
    bool is_valid;

    // These datas are accessed less frequently, so they are pushed back to the end of the struct
    gpu_devices_t physical_devices;
    vk::PhysicalDevice physical_device;
    vk::PhysicalDeviceProperties physical_device_properties;
    vk::PhysicalDeviceFeatures physical_device_features;
    vk::QueueFamilyProperties queue_family_properties;
    uint32_t selected_queue_familiy_index;

};

UsdImagingGLHkEngine::UsdImagingGLHkEngine() : pimpl(make_unique<UsdImagingGLHkEngine::Impl>())
{

}

UsdImagingGLHkEngine::~UsdImagingGLHkEngine()
{

}

void UsdImagingGLHkEngine::Render(const UsdPrim& root, RenderParams params)
{
    std::cerr << "Calling render on " << (pimpl->is_valid ? "valid" : "invalid") << " hk engine" << std::endl;
};

void UsdImagingGLHkEngine::InvalidateBuffers()
{
    std::cerr << "Calling invalidate buffers on hk engine" << std::endl;
};
