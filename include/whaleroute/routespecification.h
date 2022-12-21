#ifndef WHALEROUTE_ROUTESPECIFICATION_H
#define WHALEROUTE_ROUTESPECIFICATION_H

#include "types.h"
#include "detail/utils.h"

namespace whaleroute::config {
template <typename T, typename TRequest, typename TResponse, typename TRouteContext = _>
struct RouteSpecification {
    [[noreturn]] bool operator()(const T&, const TRequest&, TResponse&) const
    {
        static_assert(
                detail::dependent_false<T>,
                "A specialization of RouteSpecification operator() without TRouteContext argument must be provided");
    }

    [[noreturn]] bool operator()(const T&, const TRequest&, TResponse&, TRouteContext&) const
    {
        static_assert(
                detail::dependent_false<T>,
                "A specialization of RouteSpecification operator() with TRouteContext argument must be provided");
    }
};

} // namespace whaleroute::config

#endif // WHALEROUTE_ROUTESPECIFICATION_H