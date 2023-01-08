#ifndef WHALEROUTE_ROUTE_H
#define WHALEROUTE_ROUTE_H

#include "irequestrouter.h"
#include "requestprocessor.h"
#include "routespecifier.h"
#include "utils.h"
#include "external/sfun/interface.h"
#include <whaleroute/types.h>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace whaleroute {
template <typename TRouter, typename TRequest, typename TResponse, typename TResponseValue, typename TRouteContext>
class RequestRouter;
}

namespace whaleroute::detail {

template <typename TRouter, typename TRequest, typename TResponse, typename TResponseValue, typename TRouteContext>
class Route {
    using ProcessorFunc =
            std::function<void(const TRequest&, TResponse&, const std::vector<std::string>&, TRouteContext&)>;
    using Router = RequestRouter<TRouter, TRequest, TResponse, TResponseValue, TRouteContext>;

public:
    Route(IRequestRouter<TRequest, TResponse, TResponseValue>& router,
          std::vector<RouteSpecifier<TRouter, TRequest, TResponse, TRouteContext>> routeSpecifiers,
          std::function<void(const TRequest&, TResponse&, const RouteParameterError&)> routeParameterErrorHandler)
        : router_{router}
        , routeSpecifiers_{std::move(routeSpecifiers)}
        , routeParameterErrorHandler_{std::move(routeParameterErrorHandler)}
    {
    }

    template <typename TProcessor, typename... TArgs>
    auto process(TArgs&&... args) -> std::enable_if_t<std::is_constructible_v<TProcessor, TArgs...>, Route&>
    {
        if constexpr (std::is_copy_constructible_v<TProcessor>) {
            auto requestProcessor = TProcessor{std::forward<TArgs>(args)...};
            processorList_.emplace_back(
                    [requestProcessor, this](
                            const TRequest& request,
                            TResponse& response,
                            const std::vector<std::string>& routeParams,
                            TRouteContext& routeContext) mutable
                    {
                        invokeRequestProcessor<TProcessor, TRequest, TResponse>(
                                requestProcessor,
                                request,
                                response,
                                routeParams,
                                routeContext,
                                routeParameterErrorHandler_);
                    });
        }
        else {
            auto requestProcessor = std::make_shared<TProcessor>(std::forward<TArgs>(args)...);
            processorList_.emplace_back(
                    [requestProcessor, this]( //
                            const TRequest& request,
                            TResponse& response,
                            const std::vector<std::string>& routeParams,
                            TRouteContext& routeContext)
                    {
                        invokeRequestProcessor<TProcessor, TRequest, TResponse>(
                                *requestProcessor,
                                request,
                                response,
                                routeParams,
                                routeContext,
                                routeParameterErrorHandler_);
                    });
        }
        return *this;
    }

    template <typename TProcessor>
    Route& process(TProcessor&& requestProcessor)
    {
        if constexpr (std::is_lvalue_reference_v<TProcessor>) {
            processorList_.emplace_back(
                    [&requestProcessor, this]( //
                            const TRequest& request,
                            TResponse& response,
                            const std::vector<std::string>& routeParams,
                            TRouteContext& routeContext)
                    {
                        invokeRequestProcessor(
                                requestProcessor,
                                request,
                                response,
                                routeParams,
                                routeContext,
                                routeParameterErrorHandler_);
                    });
        }
        else {
            processorList_.emplace_back(
                    [requestProcessor = std::move(requestProcessor), this]( //
                            const TRequest& request,
                            TResponse& response,
                            const std::vector<std::string>& routeParams,
                            TRouteContext& routeContext)
                    {
                        invokeRequestProcessor(
                                requestProcessor,
                                request,
                                response,
                                routeParams,
                                routeContext,
                                routeParameterErrorHandler_);
                    });
        }
        return *this;
    }

    template <
            typename... TArgs,
            typename TCheckResponseValue = TResponseValue,
            typename = std::enable_if_t<!std::is_same_v<TCheckResponseValue, _>>>
    void set(TArgs&&... args)
    {
        auto responseValue = TResponseValue{std::forward<TArgs>(args)...};
        processorList_.emplace_back(
                [responseValue, this]( //
                        const TRequest&,
                        TResponse& response,
                        const std::vector<std::string>&,
                        TRouteContext&) mutable
                {
                    router_.setResponseValue(response, responseValue);
                });
    }

    std::vector<ProcessorFunc> getRequestProcessors(sfun::AccessPermission<Router>) const
    {
        auto toFilteredProcessorFunc = [&](const ProcessorFunc& processor) -> ProcessorFunc
        {
            return [this, &processor](
                           const TRequest& request,
                           TResponse& response,
                           const std::vector<std::string>& routeParams,
                           TRouteContext& routeContext)
            {
                if (!std::all_of(
                            routeSpecifiers_.begin(),
                            routeSpecifiers_.end(),
                            [&request, &response, &routeContext](auto& routeSpecifier) -> bool
                            {
                                return routeSpecifier(request, response, routeContext);
                            }))
                    return;
                processor(request, response, routeParams, routeContext);
            };
        };

        auto result = std::vector<ProcessorFunc>{};
        std::transform(
                processorList_.begin(),
                processorList_.end(),
                std::back_inserter(result),
                toFilteredProcessorFunc);
        return result;
    }

private:
    std::vector<ProcessorFunc> processorList_;
    IRequestRouter<TRequest, TResponse, TResponseValue>& router_;
    std::vector<RouteSpecifier<TRouter, TRequest, TResponse, TRouteContext>> routeSpecifiers_;
    std::function<void(const TRequest&, TResponse&, const RouteParameterError&)> routeParameterErrorHandler_;
};

} // namespace whaleroute::detail

#endif // WHALEROUTE_ROUTE_H
