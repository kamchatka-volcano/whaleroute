#ifndef WHALEROUTE_ROUTESPECIFIER_H
#define WHALEROUTE_ROUTESPECIFIER_H

#include <whaleroute/routespecification.h>
#include <functional>
#include <type_traits>
#include <vector>

namespace whaleroute::detail {

template <typename TRequest, typename TResponse, typename TRouteContext>
class RouteSpecifier {
    using ThisRouteSpecifier = RouteSpecifier<TRequest, TResponse, TRouteContext>;

public:
    template <
            typename TArg,
            std::enable_if_t<
                    !std::is_base_of_v<ThisRouteSpecifier, std::remove_reference_t<TArg>> &&
                    !std::is_same_v<TArg, std::vector<ThisRouteSpecifier>>>* = nullptr>
    RouteSpecifier(TArg&& arg)
    {
        predicate_ = [arg = std::forward<TArg>(arg)]( //
                             const TRequest& request,
                             TResponse& response,
                             TRouteContext& routeContext)
        {
            if constexpr (std::is_same_v<TRouteContext, _>)
                return config::RouteSpecification<TArg, TRequest, TResponse, TRouteContext>{}(arg, request, response);
            else
                return config::RouteSpecification<TArg, TRequest, TResponse, TRouteContext>{}(
                        arg,
                        request,
                        response,
                        routeContext);
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