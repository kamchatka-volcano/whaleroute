#ifndef WHALEROUTE_TYPES_H
#define WHALEROUTE_TYPES_H

#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace whaleroute {

struct _ {};

inline bool operator==(const _&, const _&)
{
    return true;
}

enum class TrailingSlashMode {
    Optional,
    Strict
};

enum class RegexMode {
    Regular,
    TildaEscape
};

struct rx {
    std::string value;
};

namespace detail {
struct RouteParameters {
    std::vector<std::string> value;
};
} // namespace detail

template<int minSize = 0>
struct RouteParameters : detail::RouteParameters {
    using MinSize = std::integral_constant<int, minSize>;
};

struct RouteParameterCountMismatch {
    int expectedNumber;
    int actualNumber;
};
struct RouteParameterReadError {
    int index;
    std::string value;
};
using RouteParameterError = std::variant<RouteParameterCountMismatch, RouteParameterReadError>;

namespace string_literals {
inline rx operator""_rx(const char* args, std::size_t size)
{
    return {std::string(args, size)};
}
} // namespace string_literals

} // namespace whaleroute

#endif // WHALEROUTE_TYPES_H
