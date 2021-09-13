#pragma once

namespace whaleroute{

struct _{};

inline bool operator == (const _& lhs, const _& rhs)
{
    return true;
}

enum class RouteAccess{
    Authorized,
    Forbidden,
    Open
};

}
