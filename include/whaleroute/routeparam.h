#ifndef WHALEROUTE_ROUTEPARAM_H
#define WHALEROUTE_ROUTEPARAM_H
#include "utils.h"
#include <cstdint>
#include <string>

namespace whaleroute {

constexpr std::uint32_t routeParamId(std::string_view paramTypeName)
{
    return detail::fnv1a(paramTypeName);
}
namespace config {
template<std::uint32_t routeParamTypeId>
struct RouteParam;
}

namespace detail {
template<std::uint32_t routeParamTypeId>
using RouteParamType = typename config::RouteParam<routeParamTypeId>::type;

}

} // namespace whaleroute

#endif //WHALEROUTE_ROUTEPARAM_H
