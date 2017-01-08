#pragma once

#include "vulkan_includer.h"
#include "exceptions.h"
#include "device.h"

namespace hk {
    uint32_t get_memory_type(const vk::PhysicalDeviceMemoryProperties& memory_properties,
                             uint32_t type_bits,
                             vk::MemoryPropertyFlags properties)
    {
        for (uint32_t i = 0; i < memory_properties.memoryTypeCount; ++i) {
            if ((type_bits & 1) == 1) {
                if ((memory_properties.memoryTypes[i].propertyFlags & properties) == properties) {
                    return i;
                }
            }
            type_bits >>= 1;
        }
        return memory_properties.memoryTypeCount;
    };

    struct SimpleBuffer {
        vk::Buffer buffer;
        vk::DeviceMemory memory;
        uint32_t data_size;

        inline SimpleBuffer()
            : data_size(0) {
        }

        inline SimpleBuffer(const SimpleBuffer& other)
            : buffer(other.buffer), memory(other.memory), data_size(other.data_size) {

        }

        inline SimpleBuffer(
            vk::Device& device,
            const vk::PhysicalDeviceMemoryProperties& physical_device_memory_properties,
            uint32_t buffer_size,
            vk::BufferUsageFlags usage_flags,
            vk::MemoryPropertyFlags property_flags,
            vk::SharingMode sharing_mode = vk::SharingMode::eExclusive
        ) : data_size(buffer_size) {
            buffer = hk::unwrap(device.createBuffer(
                {
                    vk::BufferCreateFlags(), data_size, usage_flags
                }), MARK_ERROR("Error creating buffer!"));
            auto buffer_reqs = device.getBufferMemoryRequirements(buffer);
            memory = hk::unwrap(device.allocateMemory(
                {
                    buffer_reqs.size,
                    get_memory_type(physical_device_memory_properties,
                                    buffer_reqs.memoryTypeBits,
                                    property_flags)
                }), MARK_ERROR("Error allocating buffer memory!"));
            hk::unwrap_void(device.bindBufferMemory(buffer, memory, 0),
                            MARK_ERROR("Error binding buffer to memory!"));
        }

        inline void copy_to(vk::CommandBuffer& copy_buffer, SimpleBuffer& to) {
            copy_buffer.copyBuffer(buffer, to.buffer, {0, 0, std::min(data_size, to.data_size)});
        }

        inline void release(vk::Device& device) {
            device_clear_resource(device, buffer);
            device_clear_resource(device, memory);
        }
    };

    struct SimpleStagingBuffer : public SimpleBuffer{
        inline SimpleStagingBuffer() : SimpleBuffer() {
        }

        inline SimpleStagingBuffer(const SimpleStagingBuffer& other)
            : SimpleBuffer(other) {
        }

        inline SimpleStagingBuffer(
            vk::Device& device,
            const vk::PhysicalDeviceMemoryProperties& physical_device_memory_properties,
            SimpleBuffer& target_buffer
        ) {
            data_size = target_buffer.data_size;
            buffer = hk::unwrap(device.createBuffer(
                {
                    vk::BufferCreateFlags(), data_size,
                    vk::BufferUsageFlagBits::eTransferSrc, vk::SharingMode::eExclusive
                }), MARK_ERROR("Error creating staging!"));
            auto staging_reqs = device.getBufferMemoryRequirements(buffer);
            memory = hk::unwrap(device.allocateMemory(
                {
                    staging_reqs.size,
                    get_memory_type(physical_device_memory_properties,
                                    staging_reqs.memoryTypeBits,
                                    vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent)
                }), MARK_ERROR("Error allocating device memory!"));
            hk::unwrap_void(device.bindBufferMemory(buffer, memory, 0),
                            MARK_ERROR("Error binding vertex buffer to memory!"));
        }

        inline SimpleStagingBuffer(
            vk::Device& device,
            const vk::PhysicalDeviceMemoryProperties& physical_device_memory_properties,
            SimpleBuffer& target_buffer,
            vk::CommandBuffer& copy_buffer, const void* data)
            : SimpleStagingBuffer(device, physical_device_memory_properties, target_buffer) {
            write_from_host(device, data);
            stage(copy_buffer, target_buffer);
        }

        inline void stage(vk::CommandBuffer& copy_buffer, SimpleBuffer& to) {
            assert(data_size == to.data_size);
            copy_buffer.copyBuffer(buffer, to.buffer, {0, 0, std::min(data_size, to.data_size)});
        }

        template <typename T>
        inline void write_from_host(vk::Device& device, const T& data) {
            assert(data_size == sizeof(data));
            memcpy(hk::unwrap(device.mapMemory(memory, 0, data_size, vk::MemoryMapFlags()),
                              MARK_ERROR("Error mapping vertex memory to host!")), &data, data_size);
            device.unmapMemory(memory);
        }

        inline void write_from_host(vk::Device& device, const void* data) {
            memcpy(hk::unwrap(device.mapMemory(memory, 0, data_size, vk::MemoryMapFlags()),
                              MARK_ERROR("Error mapping vertex memory to host!")), data, data_size);
            device.unmapMemory(memory);
        }
    };

    template <>
    inline void device_clear_resource<SimpleBuffer>(vk::Device& device, SimpleBuffer& resource) {
        resource.release(device);
    }

    template <>
    inline void device_clear_resource<SimpleStagingBuffer>(vk::Device& device, SimpleStagingBuffer& resource) {
        resource.release(device);
    }
}
