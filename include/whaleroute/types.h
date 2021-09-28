#pragma once
#include "detail/emptytype.h"

namespace whaleroute{

enum class RouteAccess{
    Authorized,
    Forbidden,
    Open
};

using _ = detail::_;

}
