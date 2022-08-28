#pragma once
#include "irequestrouter.h"
#include "routespecifier.h"
#include <whaleroute/types.h>
#include <functional>
#include <vector>
#include <string>
#include <memory>

namespace whaleroute{
template <typename TRequest, typename TResponse, typename TRequestProcessor, typename TResponseValue>
class RequestRouter;
}

namespace whaleroute::detail{

template <typename TRequest, typename TResponse, typename TRequestProcessor, typename TResponseValue>
class Route{
    friend class RequestRouter<TRequest, TResponse, TRequestProcessor, TResponseValue>;
    using Processor = std::function<void(const TRequest&, TResponse&)>;

public:
    Route(IRequestRouter<TRequest, TResponse, TRequestProcessor, TResponseValue>& router,
          std::vector<RouteSpecifier< TRequest, TResponse>> routeSpecifiers)
        : router_{router}
        , routeSpecifiers_{std::move(routeSpecifiers)}
    {
    }

    template<typename TProcessor, typename T = TRequestProcessor, typename... TArgs>
    auto process(TArgs&&... args) -> std::enable_if_t<!std::is_same_v<T, _>, Route&>
    {
        static_assert(std::is_base_of<TRequestProcessor, TProcessor>::value, "TProcessor must inherit from RequestProcessor");
        requestProcessor_ = std::make_unique<TProcessor>(std::forward<TArgs>(args)...);
        processorList_.emplace_back(
                [this](const TRequest& request, TResponse& response) {
                    router_.callRequestProcessor(*requestProcessor_, request, response);
                });
        return *this;
    }

    template<typename TProcessor, typename T = TRequestProcessor>
    auto process(TProcessor& requestProcessor)-> std::enable_if_t<!std::is_same_v<T, _>, Route&>
    {
        static_assert(std::is_base_of<TRequestProcessor, TProcessor>::value, "TProcessor must inherit from RequestProcessor");
        processorList_.emplace_back(
                [&requestProcessor, this](const TRequest& request, TResponse& response) {
                    router_.callRequestProcessor(requestProcessor, request, response);
                });
        return *this;
    }

    Route& process(std::function<void(const TRequest&, TResponse&)> requestProcessor)
    {
        processorList_.emplace_back(
                [requestProcessor](const TRequest& request, TResponse& response) {
                    requestProcessor(request, response);
                });
        return *this;
    }

    template<typename TCheckResponseValue = _,
            typename = std::enable_if_t<!std::is_same_v<TResponseValue, TCheckResponseValue>>>
    void set(const TResponseValue& responseValue)
    {
        processorList_.emplace_back(
                [responseValue, this](const TRequest&, TResponse& response) mutable {
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
                [responseValue, this](const TRequest&, TResponse& response) mutable {
                    router_.setResponseValue(response, responseValue);
                });
    }

private:
    std::vector<Processor> getRequestProcessor(const TRequest& request, TResponse& response)
    {
        if (!std::all_of(routeSpecifiers_.begin(), routeSpecifiers_.end(),
                 [&request, &response](auto& routeSpecifier) -> bool {
                     return routeSpecifier(request, response);
                 }))
            return {};

        return processorList_;
    }

private:
    std::vector<Processor> processorList_;
    std::unique_ptr<TRequestProcessor> requestProcessor_;
    IRequestRouter<TRequest, TResponse, TRequestProcessor, TResponseValue>& router_;
    std::vector<RouteSpecifier < TRequest, TResponse>> routeSpecifiers_;
};

}

