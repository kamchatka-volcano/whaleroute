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
        for (auto& match : routeMatchList_){
            if (match.isRegExp()){
                if (processMatch(match.regExp, request, response) && isRouteProcessingFinished(request, response))
                    return;
            }
            else{
                if (processMatch(match.path, request, response) && isRouteProcessingFinished(request, response))
                    return;
            }
        }
        if (noMatchRoute_.processRequest(request, response))
            return;

        this->processUnmatchedRequest(request, response);
    }

private:
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
