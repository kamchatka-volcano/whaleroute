#ifndef WHALEROUTE_ROUTEMATCHERINVOKER_H
#define WHALEROUTE_ROUTEMATCHERINVOKER_H

#include "routematcher.h"
#include <functional>
#include <type_traits>
#include <vector>

namespace whaleroute::detail {

template<typename TRouteContext, typename TArg, typename TRequest>
bool match(const TArg& arg, const TRequest& request)
{
    using RouteMatcher = config::RouteMatcher<TArg, TRouteContext>;
    if constexpr (
            !std::is_same_v<sfun::callable_return_type<RouteMatcher>, bool> ||
            !(std::is_same_v<sfun::callable_args<RouteMatcher>, sfun::type_list<const TArg&, const TRequest&>> ||
              std::is_same_v<sfun::callable_args<RouteMatcher>, sfun::type_list<TArg, const TRequest&>>)) {
        static_assert(
                sfun::dependent_false<RouteMatcher>,
                "RouteMatcher<TRouter> functor must implement bool operator()(const TArg&, const "
                "TRequest&)");
    }
    else
        return RouteMatcher{}(arg, request);
}

template<typename TRouteContext, typename TArg, typename TRequest>
bool match(const TArg& arg, const TRequest& request, TRouteContext& routeContext)
{
    using RouteMatcher = config::RouteMatcher<TArg, TRouteContext>;
    if constexpr (
            !std::is_same_v<sfun::callable_return_type<RouteMatcher>, bool> ||
            !(std::is_same_v<
                      sfun::callable_args<RouteMatcher>,
                      sfun::type_list<const TArg&, const TRequest&, const TRouteContext&>> ||
              std::is_same_v<
                      sfun::callable_args<RouteMatcher>,
                      sfun::type_list<TArg, const TRequest&, const TRouteContext&>>)) {
        static_assert(
                sfun::dependent_false<RouteMatcher>,
                "RouteMatcher<TRouter> functor must implement bool operator()(const TArg&, const "
                "TRequest&, TRouteContext&)");
    }
    else
        return RouteMatcher{}(arg, request, routeContext);
}

template<typename TRequest, typename TRouteContext>
class RouteMatcherInvoker {
    using ThisRouteMatcherInvoker = RouteMatcherInvoker<TRequest, TRouteContext>;

public:
    template<
            typename TArg,
            std::enable_if_t<
                    !std::is_base_of_v<ThisRouteMatcherInvoker, std::remove_reference_t<TArg>> &&
                    !std::is_same_v<TArg, std::vector<ThisRouteMatcherInvoker>>>* = nullptr>
    RouteMatcherInvoker(TArg&& arg)
    {
        using RouteMatcher = config::RouteMatcher<TArg, TRouteContext>;
        if constexpr (!detail::IsCompleteType<RouteMatcher>::value)
            static_assert(
                    sfun::dependent_false<RouteMatcher>,
                    "The implementation of the whaleroute::config::config::RouteMatcher<TArg, TRouteContext> functor "
                    "must be provided");

        predicate_ = [arg](const TRequest& request, [[maybe_unused]] TRouteContext& routeContext)
        {
            if constexpr (std::is_same_v<TRouteContext, _>)
                return match<TRouteContext>(arg, request);
            else
                return match<TRouteContext>(arg, request, routeContext);
        };
    }

    bool operator()(const TRequest& request, TRouteContext& routeContext) const
    {
        return predicate_(request, routeContext);
    }

private:
    std::function<bool(const TRequest&, TRouteContext&)> predicate_;
};

} // namespace whaleroute::detail

#endif // WHALEROUTE_ROUTEMATCHERINVOKER_H