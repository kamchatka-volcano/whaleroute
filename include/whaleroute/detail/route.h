#ifndef WHALEROUTE_ROUTE_H
#define WHALEROUTE_ROUTE_H

#include "irequestrouter.h"
#include "irequestprocessor.h"
#include "routespecifier.h"
#include "functionrequestprocessor.h"
#include <whaleroute/types.h>
#include "utils.h"
#include <functional>
#include <vector>
#include <string>
#include <memory>

namespace whaleroute{
template <typename TRequest, typename TResponse, typename TResponseValue>
class RequestRouter;
}

namespace whaleroute::detail{

template <typename TRequest, typename TResponse, typename TResponseValue>
class Route{
    //using IProcessor = IRequestProcessor<TRequest, TResponse>;
    using ProcessorFunc = std::function<void(const TRequest&, TResponse&, const std::vector<std::string>&)>;
    friend class RequestRouter<TRequest, TResponse, TResponseValue>;

public:
    Route(IRequestRouter<TRequest, TResponse, TResponseValue>& router,
          std::vector<RouteSpecifier< TRequest, TResponse>> routeSpecifiers)
        : router_{router}
        , routeSpecifiers_{std::move(routeSpecifiers)}
    {
    }

    template<typename TProcessor, typename... TArgs>
    auto process(TArgs&&... args) -> std::enable_if_t<std::is_constructible_v<TProcessor, TArgs...>, Route&>
    {
        //auto requestProcessor = requestProcessorList_.emplace_back(std::make_unique<TProcessor>(std::forward<TArgs>(args)...)).get();
        auto requestProcessor = std::make_shared<TProcessor>(std::forward<TArgs>(args)...);
        processorList_.emplace_back(
                [requestProcessor](const TRequest& request, TResponse& response, const std::vector<std::string>& routeParams) {
                    invokeRequestProcessor<TProcessor, TRequest, TResponse>(*requestProcessor, request, response, routeParams);
                });
        return *this;
    }

    template<typename TProcessor>
    Route& process(TProcessor&& requestProcessor) //-> std::enable_if_t<std::is_base_of_v<IProcessor, TProcessor>, Route&>
    {
        processorList_.emplace_back(
                [&requestProcessor](const TRequest& request, TResponse& response, const std::vector<std::string>& routeParams) {
                    invokeRequestProcessor(requestProcessor, request, response, routeParams);
                });
        return *this;
    }

//    template<typename... TRouteParams, typename TFunc>
//    auto process(TFunc requestProcessor) -> std::enable_if_t<std::is_invocable_v<TFunc, TRouteParams..., const TRequest&, TResponse&>, Route&>
//    {
//        return process<detail::FunctionRequestProcessor2<TFunc, TRequest, TResponse, TRouteParams...>>(std::move(requestProcessor));
//    }

    template<typename... TArgs,
            typename TCheckResponseValue = TResponseValue,
            typename = std::enable_if_t<!std::is_same_v<TCheckResponseValue, _>>>
    void set(TArgs&&... args)
    {
        auto responseValue = TResponseValue{std::forward<TArgs>(args)...};
        processorList_.emplace_back(
                [responseValue, this](const TRequest&, TResponse& response, const std::vector<std::string>&) mutable {
                    router_.setResponseValue(response, responseValue);
                });
    }

private:
    std::vector<ProcessorFunc> getRequestProcessor(const TRequest& request, TResponse& response)
    {
        if (!std::all_of(routeSpecifiers_.begin(), routeSpecifiers_.end(),
                 [&request, &response](auto& routeSpecifier) -> bool {
                     return routeSpecifier(request, response);
                 }))
            return {};

        return processorList_;
    }

private:
    std::vector<ProcessorFunc> processorList_;
//    std::vector<std::unique_ptr<IProcessor>> requestProcessorList_;
    IRequestRouter<TRequest, TResponse, TResponseValue>& router_;
    std::vector<RouteSpecifier<TRequest, TResponse>> routeSpecifiers_;
};

}

#endif //WHALEROUTE_ROUTE_H

