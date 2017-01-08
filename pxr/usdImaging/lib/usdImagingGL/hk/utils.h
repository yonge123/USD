#pragma once

#include <memory>
#include <cstring>
#include <vector>

namespace hk {
    template<typename T, typename... Args>
    inline std::unique_ptr<T> make_unique(Args&& ... args)
    {
        return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
    }

    template<typename T>
    inline void memset_t(T& target)
    {
        memset(&target, 0, sizeof(T));
    }

    template<typename T>
    inline void memset_t(T* target)
    {
        memset(target, 0, sizeof(T));
    }

    template <typename T>
    inline bool overwrite_if_different(T& out, const T& in)
    {
        if (out == in)
            return false;
        else
        {
            out = in;
            return true;
        }
    }

    template<typename T>
    inline uint32_t vector_bsize_u32(const std::vector<T>& d)
    {
        return static_cast<uint32_t>(d.size() * sizeof(T));
    }

    template<typename T>
    inline uint64_t vector_bsize_u64(const std::vector<T>& d)
    {
        return d.size() * sizeof(T);
    }

    template <typename T>
    bool is_in_sorted_vector(const std::vector<T>& v, const T& e) {
        const auto first = std::lower_bound(v.begin(), v.end(), e);
        return (first != v.end()) && (e == *first);
    }
}
