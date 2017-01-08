#include "instance.h"

#include "exceptions.h"

#include <cstring>
#include <sstream>

namespace hk {
    // Note, enabled validation requires the lunarg standard validation layer,
    // the lunarg sdk installed and configured. VK_LAYER_PATH pointing to the layers
    // and PATH / LD_LIBRARY_PATH setup.
    vk::Instance create_instance(bool enable_validation) {
        std::vector<const char*> required_extensions;
        std::vector<const char*> required_layers;
        if (enable_validation) {
            required_extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
            required_layers.push_back("VK_LAYER_LUNARG_standard_validation");
        }
        auto available_extensions = hk::unwrap_vector(vk::enumerateInstanceExtensionProperties(),
                                                      MARK_ERROR("Error querying instance extension properties"));
        auto available_layers = hk::unwrap_vector(vk::enumerateInstanceLayerProperties(),
                                                  MARK_ERROR("Error querying instance layer properties"));
        if (!check_for_extensions(required_extensions, available_extensions)) {
            std::stringstream ss;
            ss << "Can't find requested extensions" << std::endl;
            for (const auto& required_extension : required_extensions) {
                ss << "\t" << required_extension << std::endl;
            }
            ss << "In the available list of extensions" << std::endl;
            for (const auto& available_extension : available_extensions) {
                ss << "\t" << available_extension.extensionName << std::endl;
            }
            throw vulkan_exception(ss.str());
        }
        if (!check_for_layers(required_layers, available_layers)) {
            std::stringstream ss;
            ss << "Can't find requested layers" << std::endl;
            for (const auto& required_layer : required_layers) {
                ss << "\t" << required_layer << std::endl;
            }
            ss << "In the available list of layers" << std::endl;
            for (const auto& available_layer : available_layers) {
                ss << "\t" << available_layer.layerName << std::endl;
            }
            throw vulkan_exception(ss.str());
        }
        vk::ApplicationInfo app_info{
            "Usd Imaging", 1, "hk", 1, VK_API_VERSION_1_0
        };
        vk::InstanceCreateInfo instance_info{
            vk::InstanceCreateFlags(), &app_info,
            static_cast<uint32_t>(required_layers.size()), required_layers.data(),
            static_cast<uint32_t>(required_extensions.size()), required_extensions.data()
        };

        return hk::unwrap(vk::createInstance(instance_info), MARK_ERROR("Error creating instance!"));
    }

    int get_device_type_order(vk::PhysicalDeviceType device_type)
    {
        switch (device_type) {
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

    std::vector<vk::PhysicalDevice> get_sorted_physical_devices(vk::Instance instance) {
        using gpu_devices_t = std::vector<std::pair<vk::PhysicalDevice, vk::PhysicalDeviceProperties>>;
        gpu_devices_t physical_devices;
        for (auto current_device : hk::unwrap_vector(instance.enumeratePhysicalDevices(),
                                                     MARK_ERROR("No Vulkan physical device present!"))) {
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
        // RVO
        std::vector<vk::PhysicalDevice> ret;
        ret.reserve(physical_devices.size());
        for (const auto& physical_device : physical_devices) {
            ret.push_back(physical_device.first);
        }
        return ret;
    }

    bool check_for_extensions(const std::vector<const char*> required_extensions,
                              const std::vector<vk::ExtensionProperties>& available_extensions) {
        for (const auto& required_extension : required_extensions) {
            bool extension_found = false;
            for (const auto& available_extension : available_extensions) {
                if (strcmp(required_extension, available_extension.extensionName) == 0) {
                    extension_found = true;
                    break;
                }
            }
            if (!extension_found) {
                return false;
            }
        }
        return true;
    }

    bool check_for_layers(const std::vector<const char*> required_layers,
                          const std::vector<vk::LayerProperties>& available_layers) {
        for (const auto& required_layer : required_layers) {
            bool layer_found = false;
            for (const auto& available_layer : available_layers) {
                if (strcmp(required_layer, available_layer.layerName) == 0) {
                    layer_found = true;
                    break;
                }
            }
            if (!layer_found) {
                return false;
            }
        }
        return true;
    }
}
