#pragma once
#include "detail/requestprocessorset.h"
#include "detail/irequestrouter.h"
#include "types.h"
#include "detail/route.h"
#include <regex>

namespace whaleroute{

template <typename TRequestProcessor, typename TRequest, typename TRequestMethod, typename TResponse, typename TResponseValue>
using EnabledRequestProcessorDisabledRequestType = std::conditional_t<std::is_same_v<TResponseValue, _>,
        detail::IRequestRouterWithoutResponseSetterAndRequestType<TRequestProcessor, TRequest, TRequestMethod, TResponse, TResponseValue>,
        detail::IRequestRouterWithoutRequestType<TRequestProcessor, TRequest, TRequestMethod, TResponse, TResponseValue>>;

template <typename TRequestProcessor, typename TRequest, typename TRequestMethod, typename TResponse, typename TResponseValue>
using DisabledRequestProcessorDisabledRequestType = std::conditional_t<std::is_same_v<TResponseValue, _>,
        detail::IRequestRouterWithoutRequestProcessorAndResponseSetterAndRequestType<TRequestProcessor, TRequest, TRequestMethod, TResponse, TResponseValue>,
        detail::IRequestRouterWithoutRequestProcessorAndRequestType<TRequestProcessor, TRequest, TRequestMethod, TResponse, TResponseValue>>;

template <typename TRequestProcessor, typename TRequest, typename TRequestMethod, typename TResponse, typename TResponseValue>
using EnabledRequestProcessor = std::conditional_t<std::is_same_v<TResponseValue, _>,
        detail::IRequestRouterWithoutResponseSetter<TRequestProcessor, TRequest, TRequestMethod, TResponse, TResponseValue>,
        detail::IRequestRouter<TRequestProcessor, TRequest, TRequestMethod, TResponse, TResponseValue>>;

template <typename TRequestProcessor, typename TRequest, typename TRequestMethod, typename TResponse, typename TResponseValue>
using DisabledRequestProcessor = std::conditional_t<std::is_same_v<TResponseValue, _>,
        detail::IRequestRouterWithoutRequestProcessorAndResponseSetter<TRequestProcessor, TRequest, TRequestMethod, TResponse, TResponseValue>,
        detail::IRequestRouterWithoutRequestProcessor<TRequestProcessor, TRequest, TRequestMethod, TResponse, TResponseValue>>;

template <typename TRequestProcessor, typename TRequest, typename TRequestMethod, typename TResponse, typename TResponseValue>
using EnabledRequestType = std::conditional_t<std::is_same_v<TRequestProcessor, _>,
        DisabledRequestProcessor<TRequestProcessor, TRequest, TRequestMethod, TResponse, TResponseValue>,
        EnabledRequestProcessor<TRequestProcessor, TRequest, TRequestMethod, TResponse, TResponseValue>>;

template <typename TRequestProcessor, typename TRequest, typename TRequestMethod, typename TResponse, typename TResponseValue>
using DisabledRequestType = std::conditional_t<std::is_same_v<TRequestProcessor, _>,
        DisabledRequestProcessorDisabledRequestType<TRequestProcessor, TRequest, TRequestMethod, TResponse, TResponseValue>,
        EnabledRequestProcessorDisabledRequestType<TRequestProcessor, TRequest, TRequestMethod, TResponse, TResponseValue>>;

template <typename TRequestProcessor, typename TRequest, typename TRequestMethod, typename TResponse, typename TResponseValue>
using RequestRouterInterface = std::conditional_t<std::is_same_v<TRequestMethod, _>,
        DisabledRequestType<TRequestProcessor, TRequest, TRequestMethod, TResponse, TResponseValue>,
        EnabledRequestType<TRequestProcessor, TRequest, TRequestMethod, TResponse, TResponseValue>>;

template <typename TRequestProcessor, typename TRequest, typename TRequestMethod, typename TResponse, typename TResponseValue = _>
class RequestRouter : public RequestRouterInterface<TRequestProcessor, TRequest, TRequestMethod, TResponse, TResponseValue> {
    using TRoute = detail::Route<TRequestProcessor, TRequest, TRequestMethod, TResponse, TResponseValue>;
    struct PathRouteMatch{
        std::unordered_map<std::string, TRoute> authorizedRouteMap;
        std::unordered_map<std::string, TRoute> forbiddenRouteMap;
        std::unordered_map<std::string, TRoute> openRouteMap;
    };

    struct RegExpRouteMatch{
        RegExpRouteMatch(detail::RequestProcessorSet<TRequestProcessor>& requestProcessorSet,
                         detail::IRequestRouter<TRequestProcessor, TRequest, TRequestMethod, TResponse, TResponseValue>& router)
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
        RouteMatch(detail::RequestProcessorSet<TRequestProcessor>& requestProcessorSet,
                   detail::IRequestRouter<TRequestProcessor, TRequest, TRequestMethod, TResponse, TResponseValue>& router)
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
       : noMatchRoute_(requestProcessorSet_, *this)
    {
    }

    template<typename T = TRequestMethod>
    auto route(const std::string& path, TRequestMethod requestMethod,
               RouteAccess access = RouteAccess::Open) -> std::enable_if_t<!std::is_same_v<T, _>, TRoute&>
    {
        if (routeMatchList_.empty() || routeMatchList_.back().isRegExp())
            routeMatchList_.emplace_back(requestProcessorSet_, *this);

        auto& match = routeMatchList_.back().path;
        auto matchRoute = [](TRoute& route, TRequestMethod method) -> TRoute& {
            route.setRequestMethod(method);
            return route;
        };
        switch (access) {
            case RouteAccess::Authorized: {
                auto& route = match.authorizedRouteMap.emplace(path, TRoute(requestProcessorSet_, *this)).first->second;
                return matchRoute(route, requestMethod);
            }
            case RouteAccess::Forbidden: {
                auto& route = match.forbiddenRouteMap.emplace(path, TRoute(requestProcessorSet_, *this)).first->second;
                return matchRoute(route, requestMethod);
            }
            default: {
                auto& route = match.openRouteMap.emplace(path, TRoute(requestProcessorSet_, *this)).first->second;
                return matchRoute(route, requestMethod);
            }
        }
    }

    template<typename T = TRequestMethod>
    auto route(const std::string& path,
               RouteAccess access = RouteAccess::Open) -> std::enable_if_t<std::is_same_v<T, _>, TRoute&>
    {
        if (routeMatchList_.empty() || routeMatchList_.back().isRegExp())
            routeMatchList_.emplace_back(requestProcessorSet_, *this);

        auto& match = routeMatchList_.back().path;
        auto matchRoute = [](TRoute& route, TRequestMethod method) -> TRoute& {
            route.setRequestMethod(method);
            return route;
        };
        switch (access) {
            case RouteAccess::Authorized: {
                auto& route = match.authorizedRouteMap.emplace(path, TRoute(requestProcessorSet_, *this)).first->second;
                return matchRoute(route, _{});
            }
            case RouteAccess::Forbidden: {
                auto& route = match.forbiddenRouteMap.emplace(path, TRoute(requestProcessorSet_, *this)).first->second;
                return matchRoute(route, _{});
            }
            default: {
                auto& route = match.openRouteMap.emplace(path, TRoute(requestProcessorSet_, *this)).first->second;
                return matchRoute(route, _{});
            }
        }
    }

    template<typename T = TRequestMethod>
    auto route(const std::regex& regExp, TRequestMethod requestMethod,
               RouteAccess access = RouteAccess::Open) -> std::enable_if_t<!std::is_same_v<T, _>, TRoute&>
    {
        routeMatchList_.emplace_back(requestProcessorSet_, *this);
        auto& match = routeMatchList_.back().regExp;
        match.setRegexp(regExp);
        auto matchRoute = [](TRoute& route, TRequestMethod method) -> TRoute&{
            route.setRequestMethod(method);
            return route;
        };
        switch (access){
            case RouteAccess::Authorized:
                return matchRoute(match.authorizedRoute, requestMethod);
            case RouteAccess::Forbidden:
                return matchRoute(match.forbiddenRoute, requestMethod);
            default:
                return matchRoute(match.openRoute, requestMethod);
        }
    }

    template<typename T = TRequestMethod>
    auto route(const std::regex& regExp,
               RouteAccess access = RouteAccess::Open) -> std::enable_if_t<std::is_same_v<T, _>, TRoute&>
    {
        routeMatchList_.emplace_back(requestProcessorSet_, *this);
        auto& match = routeMatchList_.back().regExp;
        match.setRegexp(regExp);
        auto matchRoute = [](TRoute& route, TRequestMethod method) -> TRoute&{
            route.setRequestMethod(method);
            return route;
        };
        switch (access){
            case RouteAccess::Authorized:
                return matchRoute(match.authorizedRoute, _{});
            case RouteAccess::Forbidden:
                return matchRoute(match.forbiddenRoute, _{});
            default:
                return matchRoute(match.openRoute, _{});
        }
    }

    TRoute& route()
    {
        return noMatchRoute_;
    }

    void process(const TRequest& request, TResponse& response)
    {
        for (auto& match : routeMatchList_){
            if (match.isRegExp()){
                if (processMatch(match.regExp, request, response))
                    return;
            }
            else{
                if (processMatch(match.path, request, response))
                    return;
            }
        }
        if (noMatchRoute_.processRequest(request, response))
            return;

        this->processUnmatchedRequest(request, response);
    }

private:
    bool processMatch(PathRouteMatch& match, const TRequest& request, TResponse& response)
    {
        const auto requestPath = this->getRequestPath(request);
        if (match.openRouteMap.count(requestPath)){
            if (match.openRouteMap.at(requestPath).processRequest(request, response))
                return true;
        }

        if (isAccessAuthorized(request) && match.authorizedRouteMap.count(requestPath)){
            if (match.authorizedRouteMap.at(requestPath).processRequest(request, response))
                return true;
        }

        if (!isAccessAuthorized(request) && match.forbiddenRouteMap.count(requestPath)){
            if (match.forbiddenRouteMap.at(requestPath).processRequest(request, response))
                return true;
        }
        return false;
    }

    bool processMatch(RegExpRouteMatch& match, const TRequest& request, TResponse& response)
    {
        if (!match.matches(this->getRequestPath(request)))
            return false;

        if (match.openRoute.processRequest(request, response))
            return true;

        if (isAccessAuthorized(request)){
            if (match.authorizedRoute.processRequest(request, response))
                return true;
        }
        else{
            if(match.forbiddenRoute.processRequest(request, response))
                return true;
        }
        return false;
    }

    virtual bool isAccessAuthorized(const TRequest&) const
    {
        return true;
    }

private:
    std::vector<RouteMatch> routeMatchList_;
    TRoute noMatchRoute_;
    detail::RequestProcessorSet<TRequestProcessor> requestProcessorSet_;
};

}
