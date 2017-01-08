#pragma once

#include "vulkan_includer.h"
#include "device.h"
#include "pxr/base/gf/matrix4f.h"

namespace hk {
    struct SimpleShape {
        GfMatrix4f object_matrix;
        hk::SimpleBuffer vertex_buffer;
        hk::SimpleBuffer index_buffer;

        inline void release(vk::Device& device) {
            hk::device_clear_resource(device, vertex_buffer);
            hk::device_clear_resource(device, index_buffer);
        }
    };

    template <>
    inline void device_clear_resource<SimpleShape>(vk::Device& device, SimpleShape& resource) {
        resource.release(device);
    }
}
