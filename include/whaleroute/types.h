#ifndef WHALEROUTE_TYPES_H
#define WHALEROUTE_TYPES_H

#include <string>
#include <variant>
#include <vector>
#include <optional>

namespace whaleroute{

struct _{};

inline bool operator == (const _&, const _&)
{
    return true;
}

enum class TrailingSlashMode{
    Optional,
    Strict
};

enum class RegexMode{
    Regular,
    TildaEscape
};

struct rx{
    std::string value;
};

namespace detail{
struct RouteParameters{
    std::optional<int> numOfElements;
    std::vector<std::string> value;
};
}

template<int size = 0>
struct RouteParameters : detail::RouteParameters{
    RouteParameters()
    {
        if constexpr (size > 0)
            numOfElements = size;
    }
};



struct RouteParameterCountMismatch{
    int expectedNumber;
    int actualNumber;
};
struct RouteParameterReadError{
    int index;
    std::string value;
};
using RouteParameterError = std::variant<RouteParameterCountMismatch, RouteParameterReadError>;

namespace string_literals{
    inline rx operator ""_rx(const char* args, std::size_t size)
    {
        return {std::string(args, size)};
    }
}

}

#endif //WHALEROUTE_TYPES_H


