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

struct RouteParameterCountMismatch{
    int expectedNumber;
    int actualNumber;
};
struct RouteParameterReadError{
    int index;
    std::string value;
};
using RouteParameterError = std::variant<RouteParameterCountMismatch, RouteParameterReadError>;

}


