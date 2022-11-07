#ifndef WHALEROUTE_ROUTE_H
#define WHALEROUTE_ROUTE_H

#include "irequestrouter.h"
#include "routespecifier.h"
#include "requestprocessor_utils.h"
#include "utils.h"
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
    using ProcessorFunc = std::function<void(const TRequest&, TResponse&, const std::vector<std::string>&)>;
    friend class RequestRouter<TRequest, TResponse, TResponseValue>;

public:
    Route(IRequestRouter<TRequest, TResponse, TResponseValue>& router,
          std::vector<RouteSpecifier< TRequest, TResponse>> routeSpecifiers,
          std::function<void (const TRequest&, TResponse&, const RouteParameterError&)> routeParameterErrorHandler)
        : router_{router}
        , routeSpecifiers_{std::move(routeSpecifiers)}
        , routeParameterErrorHandler_{std::move(routeParameterErrorHandler)}
    {
    }

    template<typename TProcessor, typename... TArgs>
    auto process(TArgs&&... args) -> std::enable_if_t<std::is_constructible_v<TProcessor, TArgs...>, Route&>
    {
        if constexpr(std::is_copy_constructible_v<TProcessor>){
            auto requestProcessor = TProcessor{std::forward<TArgs>(args)...};
            processorList_.emplace_back(
                    [requestProcessor, this](const TRequest& request, TResponse& response,
                                             const std::vector<std::string>& routeParams) mutable{
                        invokeRequestProcessor<TProcessor, TRequest, TResponse>(requestProcessor, request, response,
                                                                                routeParams,
                                                                                routeParameterErrorHandler_);
                    });
        }
        else {
            auto requestProcessor = std::make_shared<TProcessor>(std::forward<TArgs>(args)...);
            processorList_.emplace_back(
                    [requestProcessor, this](const TRequest& request, TResponse& response,
                                             const std::vector<std::string>& routeParams) {
                        invokeRequestProcessor<TProcessor, TRequest, TResponse>(*requestProcessor, request, response,
                                                                                routeParams,
                                                                                routeParameterErrorHandler_);
                    });
        }
        return *this;
    }

    template<typename TProcessor>
    Route& process(TProcessor&& requestProcessor)
    {
        processorList_.emplace_back(
                [&requestProcessor, this](const TRequest& request, TResponse& response, const std::vector<std::string>& routeParams) {
                    invokeRequestProcessor(requestProcessor, request, response, routeParams, routeParameterErrorHandler_);
                });
        return *this;
    }

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
    IRequestRouter<TRequest, TResponse, TResponseValue>& router_;
    std::vector<RouteSpecifier<TRequest, TResponse>> routeSpecifiers_;
    std::function<void (const TRequest&, TResponse&, const RouteParameterError&)> routeParameterErrorHandler_;
};

}

#endif //WHALEROUTE_ROUTE_H

