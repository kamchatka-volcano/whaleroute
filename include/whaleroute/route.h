#ifndef WHALEROUTE_ROUTE_H
#define WHALEROUTE_ROUTE_H

#include "irequestrouter.h"
#include "requestprocessor.h"
#include "routematcherinvoker.h"
#include "types.h"
#include "utils.h"
#include "external/sfun/interface.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace whaleroute {
template<typename TRequest, typename TResponse, typename TResponseConverter, typename TRouteContext>
class RequestRouter;
}

namespace whaleroute::detail {

template<typename TRequest, typename TResponse, typename TResponseConverter, typename TRouteContext>
class Route {
    using ProcessorFunc =
            std::function<void(const TRequest&, TResponse&, const std::vector<std::string>&, TRouteContext&)>;
    using Router = RequestRouter<TRequest, TResponse, TResponseConverter, TRouteContext>;
    friend Router;

public:
    Route(std::vector<RouteMatcherInvoker<TRequest, TRouteContext>> routeMatchers,
          std::function<void(const TRequest&, TResponse&, const RouteParameterError&)> routeParameterErrorHandler)
        : routeMatchers_{std::move(routeMatchers)}
        , routeParameterErrorHandler_{std::move(routeParameterErrorHandler)}
    {
    }

    template<typename TProcessor, typename... TArgs>
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
                        invokeRequestProcessor<TResponseConverter>(
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
                        invokeRequestProcessor<TResponseConverter>(
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

    template<typename TProcessor>
    Route& process(TProcessor&& requestProcessor)
    {
        if constexpr (std::is_lvalue_reference_v<decltype(requestProcessor)>) {
            processorList_.emplace_back(
                    [&requestProcessor, this]( //
                            const TRequest& request,
                            TResponse& response,
                            const std::vector<std::string>& routeParams,
                            TRouteContext& routeContext)
                    {
                        invokeRequestProcessor<TResponseConverter>(
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
                    [requestProcessor = std::forward<TProcessor>(requestProcessor), this]( //
                            const TRequest& request,
                            TResponse& response,
                            const std::vector<std::string>& routeParams,
                            TRouteContext& routeContext)
                    {
                        invokeRequestProcessor<TResponseConverter>(
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

    template<
            typename... TArgs,
            typename TCheckResponseConverter = TResponseConverter,
            typename = std::enable_if_t<!std::is_same_v<TCheckResponseConverter, _>>>
    void set(TArgs&&... args)
    {
        processorList_.emplace_back(
                [=]( //
                        const TRequest&,
                        TResponse& response,
                        const std::vector<std::string>&,
                        TRouteContext&) mutable
                {
                    TResponseConverter{}(response, std::forward<TArgs>(args)...);
                });
    }

private:
    std::vector<ProcessorFunc> getRequestProcessors() const
    {
        auto toMatchedProcessorFunc = [&](const ProcessorFunc& processor) -> ProcessorFunc
        {
            return [this, &processor](
                           const TRequest& request,
                           TResponse& response,
                           const std::vector<std::string>& routeParams,
                           TRouteContext& routeContext)
            {
                if (!std::all_of(
                            routeMatchers_.begin(),
                            routeMatchers_.end(),
                            [&request, &routeContext](auto& routeMatcher) -> bool
                            {
                                return routeMatcher(request, routeContext);
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
                toMatchedProcessorFunc);
        return result;
    }

private:
    std::vector<ProcessorFunc> processorList_;
    std::vector<RouteMatcherInvoker<TRequest, TRouteContext>> routeMatchers_;
    std::function<void(const TRequest&, TResponse&, const RouteParameterError&)> routeParameterErrorHandler_;
};

} // namespace whaleroute::detail

#endif // WHALEROUTE_ROUTE_H
