#pragma once

#include "exceptions.h"

namespace hk {
    // Rant: This would be so much more elegant in rust + macros
    template <typename T>
    inline void device_clear_resource(vk::Device& device, T& resource) {
        throw hk::vulkan_exception(
            std::string("Destroy resource is not specialized for resource type ") +
            std::string(typeid(T).name()) + std::string("!"));
    }

    template <>
    inline void device_clear_resource<vk::Pipeline>(vk::Device& device, vk::Pipeline& resource) {
        if (resource) {
            device.destroyPipeline(resource);
            resource = nullptr;
        }
    }

    template <>
    inline void device_clear_resource<vk::Fence>(vk::Device& device, vk::Fence& resource) {
        if (resource) {
            device.destroyFence(resource);
            resource = nullptr;
        }
    }

    template <>
    inline void device_clear_resource<vk::ShaderModule>(vk::Device& device, vk::ShaderModule& resource) {
        if (resource) {
            device.destroyShaderModule(resource);
            resource = nullptr;
        }
    }

    template <>
    inline void device_clear_resource<vk::CommandPool>(vk::Device& device, vk::CommandPool& resource) {
        if (resource) {
            device.destroyCommandPool(resource);
            resource = nullptr;
        }
    }

    template <>
    inline void device_clear_resource<vk::DescriptorSetLayout>(vk::Device& device, vk::DescriptorSetLayout& resource) {
        if (resource) {
            device.destroyDescriptorSetLayout(resource);
            resource = nullptr;
        }
    }

    template <>
    inline void device_clear_resource<vk::PipelineLayout>(vk::Device& device, vk::PipelineLayout& resource) {
        if (resource) {
            device.destroyPipelineLayout(resource);
            resource = nullptr;
        }
    }

    template <>
    inline void device_clear_resource<vk::DescriptorPool>(vk::Device& device, vk::DescriptorPool& resource) {
        if (resource) {
            device.resetDescriptorPool(resource);
            device.destroyDescriptorPool(resource);
            resource = nullptr;
        }
    }

    template <>
    inline void device_clear_resource<vk::Buffer>(vk::Device& device, vk::Buffer& resource) {
        if (resource) {
            device.destroyBuffer(resource);
            resource = nullptr;
        }
    }

    template <>
    inline void device_clear_resource<vk::DeviceMemory>(vk::Device& device, vk::DeviceMemory& resource) {
        if (resource) {
            device.freeMemory(resource);
            resource = nullptr;
        }
    }

    template <>
    inline void device_clear_resource<vk::Image>(vk::Device& device, vk::Image& resource) {
        if (resource) {
            device.destroyImage(resource);
            resource = nullptr;
        }
    }

    template <>
    inline void device_clear_resource<vk::ImageView>(vk::Device& device, vk::ImageView& resource) {
        if (resource) {
            device.destroyImageView(resource);
            resource = nullptr;
        }
    }

    template <>
    inline void device_clear_resource<vk::RenderPass>(vk::Device& device, vk::RenderPass& resource) {
        if (resource) {
            device.destroyRenderPass(resource);
            resource = nullptr;
        }
    }

    template <>
    inline void device_clear_resource<vk::Framebuffer>(vk::Device& device, vk::Framebuffer& resource) {
        if (resource) {
            device.destroyFramebuffer(resource);
            resource = nullptr;
        }
    }
}
