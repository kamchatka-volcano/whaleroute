#ifndef WHALEROUTE_ROUTESPECIFIER_H
#define WHALEROUTE_ROUTESPECIFIER_H

#include <whaleroute/routespecification.h>
#include <functional>
#include <type_traits>
#include <vector>

namespace whaleroute::detail {

template <typename TRouter, typename TArg, typename TRequest, typename TResponse>
bool filter(const TArg& arg, const TRequest& request, TResponse& response)
{
    using RouteSpecification = config::RouteSpecification<TRouter, TArg>;
    if constexpr (
            !std::is_same_v<sfun::callable_return_type<RouteSpecification>, bool> ||
            !(std::is_same_v<
                      sfun::callable_args<RouteSpecification>,
                      sfun::type_list<const TArg&, const TRequest&, TResponse&>> ||
              std::is_same_v<
                      sfun::callable_args<RouteSpecification>,
                      sfun::type_list<TArg, const TRequest&, TResponse&>>)) {
        static_assert(
                sfun::dependent_false<RouteSpecification>,
                "RouteSpecification<TRouter> functor must implement bool operator()(const TArg&, const "
                "TRequest&, TResponse&)");
    }
    else
        return RouteSpecification{}(arg, request, response);
}

template <typename TRouter, typename TArg, typename TRequest, typename TResponse, typename TRouteContext>
bool filter(const TArg& arg, const TRequest& request, TResponse& response, TRouteContext& routeContext)
{
    using RouteSpecification = config::RouteSpecification<TRouter, TArg>;
    if constexpr (
            !std::is_same_v<sfun::callable_return_type<RouteSpecification>, bool> ||
            !(std::is_same_v<
                      sfun::callable_args<RouteSpecification>,
                      sfun::type_list<const TArg&, const TRequest&, TResponse&, TRouteContext&>> ||
              std::is_same_v<
                      sfun::callable_args<RouteSpecification>,
                      sfun::type_list<TArg, const TRequest&, TResponse&, TRouteContext&>>)) {
        static_assert(
                sfun::dependent_false<RouteSpecification>,
                "RouteSpecification<TRouter> functor must implement bool operator()(const TArg&, const "
                "TRequest&, TResponse&, TRouteContext&)");
    }
    else
        return RouteSpecification{}(arg, request, response, routeContext);
}

template <typename TRouter, typename TRequest, typename TResponse, typename TRouteContext>
class RouteSpecifier {
    using ThisRouteSpecifier = RouteSpecifier<TRouter, TRequest, TResponse, TRouteContext>;

public:
    template <
            typename TArg,
            std::enable_if_t<
                    !std::is_base_of_v<ThisRouteSpecifier, std::remove_reference_t<TArg>> &&
                    !std::is_same_v<TArg, std::vector<ThisRouteSpecifier>>>* = nullptr>
    RouteSpecifier(TArg&& arg)
    {
        using RouteSpecification = config::RouteSpecification<TRouter, TArg>;
        if constexpr (!detail::IsCompleteType<RouteSpecification>::value)
            static_assert(
                    sfun::dependent_false<RouteSpecification>,
                    "The implementation of the whaleroute::config::config::RouteSpecification<TRouter> functor "
                    "must be provided");

        predicate_ = [arg](const TRequest& request, TResponse& response, TRouteContext& routeContext)
        {
            if constexpr (std::is_same_v<TRouteContext, _>)
                return filter<TRouter>(arg, request, response);
            else
                return filter<TRouter>(arg, request, response, routeContext);
        };
    }

    bool operator()(const TRequest& request, TResponse& response, TRouteContext& routeContext) const
    {
        return predicate_(request, response, routeContext);
    }

private:
    std::function<bool(const TRequest&, TResponse&, TRouteContext&)> predicate_;
};

} // namespace whaleroute::detail

#endif // WHALEROUTE_ROUTESPECIFIER_H