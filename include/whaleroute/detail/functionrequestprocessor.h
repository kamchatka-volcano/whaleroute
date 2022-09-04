#pragma once
#include <whaleroute/requestprocessor.h>
#include <functional>

namespace whaleroute::detail{

template<typename TRequest, typename TResponse, typename... TRouteParams>
class FunctionRequestProcessor : public RequestProcessor<TRequest, TResponse, TRouteParams...>
{
public:
    FunctionRequestProcessor(std::function<void(const TRouteParams&..., const TRequest&, TResponse&)> func)
        : func_{std::move(func)}
    {}

    void process(const TRouteParams&... params, const TRequest& request, TResponse& response) override
    {
        func_(params..., request, response);
    }

private:
    std::function<void(const TRouteParams&..., const TRequest&, TResponse&)> func_;
};

}