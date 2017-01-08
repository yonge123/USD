#pragma once

#include <vector>
#include "vulkan_includer.h"

namespace hk {
    vk::Instance create_instance(bool enable_validation);
    std::vector<vk::PhysicalDevice> get_sorted_physical_devices(vk::Instance instance);
    bool check_for_extensions(const std::vector<const char*> required_extensions,
                              const std::vector<vk::ExtensionProperties>& available_extensions);
    bool check_for_layers(const std::vector<const char*> required_layers,
                          const std::vector<vk::LayerProperties>& available_layers);
}
