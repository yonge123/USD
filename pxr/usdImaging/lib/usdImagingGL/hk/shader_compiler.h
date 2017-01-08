#pragma once

#include "vulkan_includer.h"

namespace hk {
    std::vector<unsigned int> compile_spv(const char* shader_code, vk::ShaderStageFlagBits stage);
    void initialize_compiler();
    void finalize_compiler();
}
