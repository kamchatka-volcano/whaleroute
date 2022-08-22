#pragma once
#include <whaleroute/routespecification.h>
#include <functional>

namespace whaleroute::detail{

template<typename TRequest, typename TResponse>
class RouteSpecifier{
public:
    template<typename TArg>
    RouteSpecifier(TArg&& arg)
    {
        predicate_ = [arg = std::forward<TArg>(arg)](const TRequest& request, TResponse& response){
            return traits::RouteSpecification<TArg, TRequest, TResponse>{}(arg, request, response);
        };
    }

    bool operator()(const TRequest& request, TResponse& response) const
    {
        return predicate_(request, response);
    }

private:
    std::function<bool(const TRequest&, TResponse&)> predicate_;
};

}