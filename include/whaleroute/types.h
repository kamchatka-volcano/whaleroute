#pragma once
#include <string>
#include <variant>

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


