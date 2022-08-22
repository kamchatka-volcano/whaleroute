#pragma once

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

}


