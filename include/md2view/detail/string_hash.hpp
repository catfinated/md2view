#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <string_view>

namespace detail {

struct StringHash {
    using is_transparent = void; // Enables heterogeneous lookup

    std::size_t operator()(std::string_view sv) const {
        return std::hash<std::string_view>{}(sv);
    }
    std::size_t operator()(std::string const& str) const {
        return std::hash<std::string_view>{}(str);
    }
    std::size_t operator()(char const* str) const {
        return std::hash<std::string_view>{}(str);
    }
};

} // namespace detail
