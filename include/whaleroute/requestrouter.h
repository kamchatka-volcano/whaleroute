#pragma once
#include "types.h"
#include "requestprocessorqueue.h"
#include "detail/irequestrouter.h"
#include "detail/route.h"
#include "detail/utils.h"
#include <deque>
#include <regex>
#include <variant>

namespace whaleroute{

enum class TrailingSlashMode{
    Optional,
    Strict
};

template <typename TRequest, typename TResponse, typename TRequestProcessor = _, typename TResponseValue = _>
class RequestRouter : public detail::IRequestRouter<TRequest, TResponse, TRequestProcessor, TResponseValue> {
    using TRoute = detail::Route<TRequest, TResponse, TRequestProcessor, TResponseValue>;

    struct RegExpRouteMatch{
        std::regex regExp;
        TRoute route;
    };
    struct PathRouteMatch{
        std::string path;
        TRoute route;
    };
    using RouteMatch = std::variant<RegExpRouteMatch, PathRouteMatch>;

public:
    RequestRouter()
       : noMatchRoute_{*this, {}}
    {
        if constexpr(!std::is_same_v<TRequestProcessor, whaleroute::_>)
            static_assert(std::has_virtual_destructor_v<TRequestProcessor>, "TRequestProcessor must have a virtual destructor");
    }

    void setTrailingSlashMode(TrailingSlashMode mode)
    {
        trailingSlashMode_ = mode;
    }

    template<typename... TRouteSpecificationArgs>
    TRoute& route(const std::string& path, TRouteSpecificationArgs&&... spec)
    {
        return stringRouteImpl(path, {std::forward<TRouteSpecificationArgs>(spec)...});
    }

    TRoute& route(const std::string& path)
    {
        return stringRouteImpl(path, {});
    }

    template<typename... TRouteSpecificationArgs>
    TRoute& route(const std::regex& regExp, TRouteSpecificationArgs&&... spec)
    {
        return regexRouteImpl(regExp, {std::forward<TRouteSpecificationArgs>(spec)...});
    }

    TRoute& route(const std::regex& regExp)
    {
        return regexRouteImpl(regExp, {});
    }

    TRoute& route()
    {
        return noMatchRoute_;
    }

    void process(const TRequest& request, TResponse& response)
    {
        auto queue = makeRequestProcessorQueue(request, response);
        queue.launch();
    }

    RequestProcessorQueue makeRequestProcessorQueue(const TRequest& request, TResponse& response)
    {
        auto requestProcessorInvokerList = std::vector<std::function<std::optional<bool>()>>{};
        for (auto& match : routeMatchList_) {
            std::visit([&](auto& match){
                using T = std::decay_t<decltype(match)>;
                if constexpr(std::is_same_v<T, RegExpRouteMatch>){
                    if (std::regex_match(this->getRequestPath(request), match.regExp))
                        detail::concat(requestProcessorInvokerList,
                                       makeRequestProcessorInvokerList(
                                               match.route.getRequestProcessor(request, response), request, response));

                }
                else if constexpr(std::is_same_v<T, PathRouteMatch>){
                    auto requestPath = this->getRequestPath(request);
                    if (trailingSlashMode_ == TrailingSlashMode::Optional &&
                        requestPath != "/" &&
                        !requestPath.empty() &&
                        requestPath.back() == '/')
                        requestPath.pop_back();
                    if (match.path == requestPath)
                        detail::concat(requestProcessorInvokerList,
                                       makeRequestProcessorInvokerList(
                                               match.route.getRequestProcessor(request, response), request, response));
                }
            }, match);
        }

        for (const auto& processor : noMatchRoute_.getRequestProcessor(request, response))
            requestProcessorInvokerList.emplace_back([request, response, processor]() mutable -> std::optional<bool>{
                processor(request, response);
                return false;
            });

        if (requestProcessorInvokerList.empty())
            requestProcessorInvokerList.emplace_back([request, response, this]() mutable -> std::optional<bool>{
                this->processUnmatchedRequest(request, response);
                return false;
            });

        return RequestProcessorQueue{requestProcessorInvokerList,
                                     [this, request, response]() mutable {
                                         this->processUnmatchedRequest(request, response);
                                     }};
    }

private:
    std::vector<std::function<std::optional<bool>()>> makeRequestProcessorInvokerList(
            const std::vector<std::function<void(const TRequest&, TResponse&)>>& processorList,
            const TRequest& request,
            TResponse& response)
    {
        auto result = std::vector<std::function<std::optional<bool>()>>{};
        for (const auto& processor : processorList) {
            auto checkIfFinished = (&processor == &processorList.back());
            result.emplace_back([request, response, processor, checkIfFinished, this]() mutable -> std::optional<bool>{
                processor(request, response);
                if (checkIfFinished)
                    return !isRouteProcessingFinished(request, response);
                else
                    return true;
            });
        }
        return result;
    };

    TRoute& stringRouteImpl(
            const std::string& path,
            std::vector<detail::RouteSpecification<TRequest, TResponse>> routeSpecifications = {})
    {
        auto routePath = path;
        if (trailingSlashMode_ == TrailingSlashMode::Optional &&
                !routePath.empty() && routePath != "/" && routePath.back() == '/')
            routePath.pop_back();

        auto& routeMatch = routeMatchList_.emplace_back(PathRouteMatch{routePath, TRoute{*this, routeSpecifications}});
        return std::get<PathRouteMatch>(routeMatch).route;
    }

    TRoute& regexRouteImpl(
            const std::regex& regExp,
            std::vector<detail::RouteSpecification<TRequest, TResponse>> routeSpecifications = {})
    {
        auto& routeMatch = routeMatchList_.emplace_back(
                RegExpRouteMatch{regExp, {*this, std::move(routeSpecifications)}});
        return std::get<RegExpRouteMatch>(routeMatch).route;
    }

    virtual bool isRouteProcessingFinished(const TRequest&, TResponse&) const
    {
        return true;
    }

private:
    std::deque<RouteMatch> routeMatchList_;
    TRoute noMatchRoute_;
    TrailingSlashMode trailingSlashMode_ = TrailingSlashMode::Optional;
};

}
