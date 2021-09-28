#pragma once
#include "types.h"
#include "detail/requestprocessorinstancer.h"
#include "detail/irequestrouter.h"
#include "detail/route.h"
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
       : noMatchRoute_(requestProcessorSet_, *this)
    {
    }

    template<typename T = TRequestType>
    auto route(const std::string& path, detail::RouteRequestType<TRequestType> requestType,
               RouteAccess access = RouteAccess::Open) -> std::enable_if_t<!std::is_same_v<T, _>, TRoute&>
    {
        if (routeMatchList_.empty() || routeMatchList_.back().isRegExp())
            routeMatchList_.emplace_back(requestProcessorSet_, *this);

        auto& match = routeMatchList_.back().path;
        auto matchRoute = [](auto& route, auto type) -> TRoute& {
            route.setRequestType(type);
            return route;
        };
        switch (access) {
            case RouteAccess::Authorized: {
                auto& route = match.authorizedRouteMap.emplace(path, TRoute(requestProcessorSet_, *this)).first->second;
                return matchRoute(route, requestType);
            }
            case RouteAccess::Forbidden: {
                auto& route = match.forbiddenRouteMap.emplace(path, TRoute(requestProcessorSet_, *this)).first->second;
                return matchRoute(route, requestType);
            }
            default: {
                auto& route = match.openRouteMap.emplace(path, TRoute(requestProcessorSet_, *this)).first->second;
                return matchRoute(route, requestType);
            }
        }
    }

    template<typename T = TRequestType>
    auto route(const std::string& path,
               RouteAccess access = RouteAccess::Open) -> std::enable_if_t<std::is_same_v<T, _>, TRoute&>
    {
        if (routeMatchList_.empty() || routeMatchList_.back().isRegExp())
            routeMatchList_.emplace_back(requestProcessorSet_, *this);

        auto& match = routeMatchList_.back().path;
        auto matchRoute = [](auto& route, auto type) -> TRoute& {
            route.setRequestType(type);
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

    template<typename T = TRequestType>
    auto route(const std::regex& regExp, detail::RouteRequestType<TRequestType> requestType,
               RouteAccess access = RouteAccess::Open) -> std::enable_if_t<!std::is_same_v<T, _>, TRoute&>
    {
        routeMatchList_.emplace_back(requestProcessorSet_, *this);
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

    template<typename T = TRequestType>
    auto route(const std::regex& regExp,
               RouteAccess access = RouteAccess::Open) -> std::enable_if_t<std::is_same_v<T, _>, TRoute&>
    {
        routeMatchList_.emplace_back(requestProcessorSet_, *this);
        auto& match = routeMatchList_.back().regExp;
        match.setRegexp(regExp);
        auto matchRoute = [](auto& route, auto type) -> TRoute&{
            route.setRequestType(type);
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
    detail::RequestProcessorInstancer<TRequestProcessor> requestProcessorSet_;
};

}
