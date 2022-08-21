#pragma once
#include <functional>

namespace whaleroute{
template<typename T, typename TRequest, typename TResponse>
struct RouteSpecificationPredicate{
    bool operator()(T value, const TRequest& request, TResponse& response) const;
};

}

namespace whaleroute::detail{

template<typename TRequest, typename TResponse>
class RouteSpecification{
public:
    template<typename TArg>
    RouteSpecification(TArg&& arg)
    {
        predicate_ = [arg](const TRequest& request, TResponse& response){
            return RouteSpecificationPredicate<TArg, TRequest, TResponse>{}(arg, request, response);
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