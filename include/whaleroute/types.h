#ifndef WHALEROUTE_TYPES_H
#define WHALEROUTE_TYPES_H

#include <algorithm>
#include <array>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace whaleroute {

#if (defined(_MSVC_LANG) && _MSVC_LANG >= 202002L) || (!defined(_MSVC_LANG) && __cplusplus >= 202002L)
namespace detail {

template<size_t bytesCount>
struct StaticString {
    char data[bytesCount];
    constexpr size_t size() const
    {
        return bytesCount - 1;
    }
    constexpr std::string_view str() const
    {
        return {data, size()};
    }
    constexpr StaticString(const char (&init)[bytesCount])
    {
        std::copy_n(init, bytesCount, data);
    }
};

} //namespace detail

#endif

struct _ {};

inline bool operator==(const _&, const _&)
{
    return true;
}

enum class TrailingSlashMode {
    Optional,
    Strict
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

} // namespace whaleroute

#endif // WHALEROUTE_TYPES_H
