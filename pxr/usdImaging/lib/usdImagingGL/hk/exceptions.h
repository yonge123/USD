#pragma once

namespace hk {
    struct vulkan_exception {
        vulkan_exception(const std::string& _description) : description(_description)
        { }
        std::string description;
    };

    struct vulkan_exception_error_code : public vulkan_exception {
        vulkan_exception_error_code(const std::string& _description, vk::Result _error_code)
            : vulkan_exception(_description),
              error_code(_error_code)
        { }
        vk::Result error_code;
    };

    struct vulkan_exception_empty_vector : public vulkan_exception {
        vulkan_exception_empty_vector(const std::string& _description) : vulkan_exception(_description)
        { }
    };

    // rusty approach
    template<typename T>
    inline T unwrap(const vk::ResultValue<T>& result, const char* error)
    {
        if (result.result != vk::Result::eSuccess) {
            throw vulkan_exception_error_code{error, result.result};
        }
        return result.value;
    }

    // TODO: use std::move, RVO probably does not work here
    template<typename T>
    inline std::vector<T> unwrap_vector(vk::ResultValue<std::vector<T>>&& result, const char* error)
    {
        if (result.result != vk::Result::eSuccess || result.value.size() == 0) {
            throw vulkan_exception_error_code{error, result.result};
        } else if (result.value.size() == 0) {
            throw vulkan_exception_empty_vector(error);
        }
        return std::move(result.value);
    }

    inline void unwrap_void(vk::Result result, const char* error)
    {
        if (result != vk::Result::eSuccess) {
            throw vulkan_exception_error_code(error, result);
        }
    }
}

// We have to use macros for these
#define STRINGIZE_DETAIL(x) #x
#define STRINGIZE(x) STRINGIZE_DETAIL(x)
#define MARK_ERROR(x) __FILE__ ":" STRINGIZE(__LINE__) " " x
#define MARK_NONE() " " __FILE__ ":" STRINGIZE(__LINE__)
