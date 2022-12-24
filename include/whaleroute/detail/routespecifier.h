#ifndef WHALEROUTE_ROUTESPECIFIER_H
#define WHALEROUTE_ROUTESPECIFIER_H

#include <whaleroute/routespecification.h>
#include <functional>
#include <type_traits>
#include <vector>

namespace whaleroute::detail {

template <typename TRequest, typename TResponse>
class RouteSpecifier {
public:
    template <
            typename TArg,
            std::enable_if_t<
                    !std::is_base_of_v<RouteSpecifier<TRequest, TResponse>, std::remove_reference_t<TArg>> &&
                    !std::is_same_v<TArg, std::vector<RouteSpecifier<TRequest, TResponse>>>>* = nullptr>
    RouteSpecifier(TArg&& arg)
    {
        predicate_ = [arg = std::forward<TArg>(arg)](const TRequest& request, TResponse& response)
        {
            return config::RouteSpecification<TArg, TRequest, TResponse>{}(arg, request, response);
        };
    }

    bool operator()(const TRequest& request, TResponse& response) const
    {
        return predicate_(request, response);
    }

private:
    std::function<bool(const TRequest&, TResponse&)> predicate_;
};

} // namespace whaleroute::detail

#endif // WHALEROUTE_ROUTESPECIFIER_H