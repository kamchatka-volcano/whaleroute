#pragma once
#include "irequestrouter.h"
#include "irequestprocessor.h"
#include "routespecifier.h"
#include "functionrequestprocessor.h"
#include <whaleroute/types.h>
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
    using IProcessor = IRequestProcessor<TRequest, TResponse>;
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
    auto process(TArgs&&... args) -> std::enable_if_t<std::is_base_of_v<IProcessor, TProcessor>, Route&>
    {
        auto requestProcessor = requestProcessorList_.emplace_back(std::make_unique<TProcessor>(std::forward<TArgs>(args)...)).get();
        processorList_.emplace_back(
                [requestProcessor](const TRequest& request, TResponse& response, const std::vector<std::string>& routeParams) {
                    requestProcessor->processRouterRequest(request, response, routeParams);
                });
        return *this;
    }

    template<typename TProcessor>
    auto process(TProcessor& requestProcessor) -> std::enable_if_t<std::is_base_of_v<IProcessor, TProcessor>, Route&>
    {
        processorList_.emplace_back(
                [&requestProcessor](const TRequest& request, TResponse& response, const std::vector<std::string>& routeParams) {
                    static_cast<IProcessor&>(requestProcessor).processRouterRequest(request, response, routeParams);
                });
        return *this;
    }

    template<typename... TRouteParams>
    Route& process(std::function<void(TRouteParams... params, const TRequest&, TResponse&)> requestProcessor)
    {
        return process<detail::FunctionRequestProcessor<TRequest, TResponse, TRouteParams...>>(std::move(requestProcessor));
    }

    template<typename TCheckResponseValue = _,
            typename = std::enable_if_t<!std::is_same_v<TResponseValue, TCheckResponseValue>>>
    void set(const TResponseValue& responseValue)
    {
        processorList_.emplace_back(
                [responseValue, this](const TRequest&, TResponse& response, const std::vector<std::string>&) mutable {
                    router_.setResponseValue(response, responseValue);
                });
    }

    template<typename... TArgs,
            typename TCheckResponseValue = _,
            typename = std::enable_if_t<!std::is_same_v<TResponseValue, TCheckResponseValue>>>
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
    std::vector<std::unique_ptr<IProcessor>> requestProcessorList_;
    IRequestRouter<TRequest, TResponse, TResponseValue>& router_;
    std::vector<RouteSpecifier<TRequest, TResponse>> routeSpecifiers_;
};

}

