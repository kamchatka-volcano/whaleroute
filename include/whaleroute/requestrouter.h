#pragma once
#include "types.h"
#include "requestprocessorqueue.h"
#include "detail/requestprocessorinstancer.h"
#include "detail/irequestrouter.h"
#include "detail/route.h"
#include "detail/utils.h"
#include <regex>

namespace whaleroute{

template <typename TRequest, typename TResponse, typename TRequestType = _, typename TRequestProcessor = _, typename TResponseValue = _>
class RequestRouter : public detail::IRequestRouter<TRequest, TResponse, TRequestType, TRequestProcessor, TResponseValue> {
    using TRoute = detail::Route<TRequest, TResponse, TRequestType, TRequestProcessor, TResponseValue>;
    struct PathRouteMatch{
        std::unordered_map<std::string, TRoute> authorizedRouteMap;
        std::unordered_map<std::string, TRoute> forbiddenRouteMap;
        std::unordered_map<std::string, TRoute> openRouteMap;
    };

    struct RegExpRouteMatch{
        RegExpRouteMatch(detail::RequestProcessorInstancer<TRequestProcessor>& requestProcessorSet,
                         detail::IRequestRouter<TRequest, TResponse, TRequestType, TRequestProcessor, TResponseValue>& router)
            : authorizedRoute(requestProcessorSet, router)
            , forbiddenRoute(requestProcessorSet, router)
            , openRoute(requestProcessorSet, router)
        {}

        void setRegexp(const std::regex& regex)
        {
            regExp_ = regex;
            isValid_ = true;
        }

        bool isValid() const
        {
            return isValid_;
        }

        bool matches(const std::string& path)
        {
            return std::regex_match(path, regExp_);
        }

        TRoute authorizedRoute;
        TRoute forbiddenRoute;
        TRoute openRoute;

    private:
        bool isValid_ = false;
        std::regex regExp_;
    };

    struct RouteMatch{
        RouteMatch(detail::RequestProcessorInstancer<TRequestProcessor>& requestProcessorSet,
                   detail::IRequestRouter<TRequest, TResponse, TRequestType, TRequestProcessor, TResponseValue>& router)
            : regExp(requestProcessorSet, router)
        {}
        PathRouteMatch path;
        RegExpRouteMatch regExp;
        bool isRegExp() const
        {
            return regExp.isValid();
        }

    };

public:
    RequestRouter()
       : noMatchRoute_(requestProcessorInstancer_, *this)
    {
        if constexpr(!std::is_same_v<TRequestProcessor, whaleroute::_>)
            static_assert(std::has_virtual_destructor_v<TRequestProcessor>, "TRequestProcessor must have a virtual destructor");
    }

    template<typename T = TRequestType>
    auto route(const std::string& path,
               detail::RouteRequestType<TRequestType> requestType,
               RouteAccess access = RouteAccess::Open) -> std::enable_if_t<!std::is_same_v<T, _>, TRoute&>
    {
        return stringRouteImpl(path, requestType, access);
    }

    template<typename T = TRequestType>
    auto route(const std::string& path,
               RouteAccess access = RouteAccess::Open) -> std::enable_if_t<std::is_same_v<T, _>, TRoute&>
    {
        return stringRouteImpl(path, _{}, access);
    }

    template<typename T = TRequestType>
    auto route(const std::regex& regExp, detail::RouteRequestType<TRequestType> requestType,
               RouteAccess access = RouteAccess::Open) -> std::enable_if_t<!std::is_same_v<T, _>, TRoute&>
    {
        return regexRouteImpl(regExp, requestType, access);
    }

    template<typename T = TRequestType>
    auto route(const std::regex& regExp,
               RouteAccess access = RouteAccess::Open) -> std::enable_if_t<std::is_same_v<T, _>, TRoute&>
    {
        return regexRouteImpl(regExp, _{}, access);
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
            if (match.isRegExp() && match.regExp.matches(this->getRequestPath(request))) {
                detail::concat(requestProcessorInvokerList,
                               makeRequestProcessorInvokerList(match.regExp.openRoute.getRequestProcessor(request, response), request, response));
                detail::concat(requestProcessorInvokerList,
                               makeRequestProcessorInvokerList(match.regExp.authorizedRoute.getRequestProcessor(request, response), request, response, true));
                detail::concat(requestProcessorInvokerList,
                               makeRequestProcessorInvokerList(match.regExp.forbiddenRoute.getRequestProcessor(request, response), request, response, false));
            }
            else {
                const auto requestPath = this->getRequestPath(request);
                if (match.path.openRouteMap.count(requestPath))
                    detail::concat(requestProcessorInvokerList,
                                   makeRequestProcessorInvokerList(
                                           match.path.openRouteMap.at(requestPath).getRequestProcessor(
                                                   request, response), request, response));

                if (match.path.authorizedRouteMap.count(requestPath))
                    detail::concat(requestProcessorInvokerList,
                                   makeRequestProcessorInvokerList(
                                           match.path.authorizedRouteMap.at(requestPath).getRequestProcessor(
                                                   request, response), request, response, true));

                if (match.path.forbiddenRouteMap.count(requestPath))
                    detail::concat(requestProcessorInvokerList,
                                   makeRequestProcessorInvokerList(
                                           match.path.forbiddenRouteMap.at(requestPath).getRequestProcessor(
                                                   request, response), request, response, false));

            }
        }

        for (const auto& processor : noMatchRoute_.getRequestProcessor(request, response))
            requestProcessorInvokerList.emplace_back([request, response, processor, this]() mutable -> std::optional<bool>{
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
            TResponse& response,
            std::optional<bool> checkAuthorizationState = {})
    {
        auto result = std::vector<std::function<std::optional<bool>()>>{};
        for (const auto& processor : processorList) {
            auto checkIfFinished = (&processor == &processorList.back());
            result.emplace_back([request, response, processor, checkIfFinished, checkAuthorizationState, this]() mutable -> std::optional<bool>{
                if (!checkAuthorizationState)
                    processor(request, response);
                else {
                    if (isAccessAuthorized(request) == *checkAuthorizationState)
                        processor(request, response);
                    else
                        return std::nullopt;
                }
                if (checkIfFinished)
                    return !isRouteProcessingFinished(request, response);
                else
                    return true;
            });
        }
        return result;
    };

    TRoute& stringRouteImpl(const std::string& path,
                            detail::RouteRequestType<TRequestType> requestType,
                            RouteAccess access = RouteAccess::Open)
    {
        if (routeMatchList_.empty() || routeMatchList_.back().isRegExp())
            routeMatchList_.emplace_back(requestProcessorInstancer_, *this);

        auto& match = routeMatchList_.back().path;
        auto matchRoute = [](auto& route, auto type) -> TRoute& {
            route.setRequestType(type);
            return route;
        };
        switch (access) {
            case RouteAccess::Authorized: {
                auto& route = match.authorizedRouteMap.emplace(path, TRoute(requestProcessorInstancer_, *this)).first->second;
                return matchRoute(route, requestType);
            }
            case RouteAccess::Forbidden: {
                auto& route = match.forbiddenRouteMap.emplace(path, TRoute(requestProcessorInstancer_, *this)).first->second;
                return matchRoute(route, requestType);
            }
            default: {
                auto& route = match.openRouteMap.emplace(path, TRoute(requestProcessorInstancer_, *this)).first->second;
                return matchRoute(route, requestType);
            }
        }
    }

    TRoute& regexRouteImpl(const std::regex& regExp,
                            detail::RouteRequestType<TRequestType> requestType,
                            RouteAccess access = RouteAccess::Open)
    {
        routeMatchList_.emplace_back(requestProcessorInstancer_, *this);
        auto& match = routeMatchList_.back().regExp;
        match.setRegexp(regExp);
        auto matchRoute = [](auto& route, auto type) -> TRoute&{
            route.setRequestType(type);
            return route;
        };
        switch (access){
            case RouteAccess::Authorized:
                return matchRoute(match.authorizedRoute, requestType);
            case RouteAccess::Forbidden:
                return matchRoute(match.forbiddenRoute, requestType);
            default:
                return matchRoute(match.openRoute, requestType);
        }
    }

    virtual bool isAccessAuthorized(const TRequest&) const
    {
        return true;
    }

    virtual bool isRouteProcessingFinished(const TRequest&, TResponse&) const
    {
        return true;
    }

private:
    std::vector<RouteMatch> routeMatchList_;
    TRoute noMatchRoute_;
    detail::RequestProcessorInstancer<TRequestProcessor> requestProcessorInstancer_;
};

}
