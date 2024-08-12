#ifndef WHALEROUTE_REQUESTROUTER_H
#define WHALEROUTE_REQUESTROUTER_H

#include "requestprocessorqueue.h"
#include "route.h"
#include "routeparam.h"
#include "routeparamparser.h"
#include "types.h"
#include "utils.h"
#include "external/sfun/functional.h"
#include "external/sfun/interface.h"
#include <any>
#include <deque>
#include <optional>
#include <regex>
#include <unordered_map>
#include <variant>

namespace whaleroute {

template<typename TRequest, typename TResponse, typename TResponseConverter = _, typename TRouteContext = _>
class RequestRouter : private sfun::interface<RequestRouter<TRequest, TResponse, TResponseConverter, TRouteContext>> {
    using RequestProcessorFunc =
            std::function<void(const TRequest&, TResponse&, const std::vector<std::string>&, TRouteContext&)>;

    struct RegExpRouteMatch {
        std::regex regExp;
        std::any route;
        std::function<std::vector<
                std::function<void(const TRequest&, TResponse&, const std::vector<std::string>&, TRouteContext&)>>()>
                getRouteRequestProcessors;
    };

public:
    RequestRouter()
        : noMatchRoute_{{}, routeParametersErrorHandler()}
    {
    }

    void setTrailingSlashMode(TrailingSlashMode mode)
    {
        trailingSlashMode_ = mode;
    }

    template<typename... TRouteMatcherArgs>
    auto& route(const std::string& path, TRouteMatcherArgs&&... matcherArgs)
    {
        return dynamicRouteImpl(path, {std::forward<TRouteMatcherArgs>(matcherArgs)...});
    }

    auto& route(const std::string& path)
    {
        return dynamicRouteImpl(path);
    }

    template<typename... TRouteMatcherArgs>
    auto& routeRegex(const std::string& regex, TRouteMatcherArgs&&... matcherArgs)
    {
        return makeRegexRoute(regex, {std::forward<TRouteMatcherArgs>(matcherArgs)...});
    }

    auto& routeRegex(const std::string& regex)
    {
        return makeRegexRoute(regex, {});
    }

#if (defined(_MSVC_LANG) && _MSVC_LANG >= 202002L) || (!defined(_MSVC_LANG) && __cplusplus >= 202002L)

    template<detail::StaticString path>
    auto& route()
    {
        return routeImpl<path>({});
    }

    template<detail::StaticString path, typename... TRouteMatcherArgs>
    auto& route(TRouteMatcherArgs&&... matcherArgs)
    {
        return routeImpl<path>({std::forward<TRouteMatcherArgs>(matcherArgs)...});
    }

    template<detail::StaticString regex>
    auto& routeRegex()
    {
        constexpr auto captureGroupCount = detail::countRegexCaptureGroups<regex>();
        return makeRegexRoute<captureGroupCount>(std::string{regex.str()}, {});
    }

    template<detail::StaticString regex, typename... TRouteMatcherArgs>
    auto& routeRegex(TRouteMatcherArgs&&... matcherArgs)
    {
        constexpr auto captureGroupCount = detail::countRegexCaptureGroups<regex>();
        return makeRegexRoute<captureGroupCount>(
                std::string{regex.str()},
                {std::forward<TRouteMatcherArgs>(matcherArgs)...});
    }

#endif

    auto& route()
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
        auto requestProcessorInvokerList = makeRouteRequestProcessorInvokerList(request, response);
        for (const auto& processor : noMatchRoute_.getRequestProcessors())
            requestProcessorInvokerList.emplace_back(
                    [request, response, processor](TRouteContext& routeContext) mutable -> bool
                    {
                        processor(request, response, {}, routeContext);
                        return false;
                    });

        if (requestProcessorInvokerList.empty())
            requestProcessorInvokerList.emplace_back(
                    [request, response, this](TRouteContext&) mutable -> bool
                    {
                        this->processUnmatchedRequest(request, response);
                        return false;
                    });

        return RequestProcessorQueue{requestProcessorInvokerList};
    }

private:
#if (defined(_MSVC_LANG) && _MSVC_LANG >= 202002L) || (!defined(_MSVC_LANG) && __cplusplus >= 202002L)
    template<detail::StaticString path>
    auto& routeImpl(std::vector<detail::RouteMatcherInvoker<TRequest, TRouteContext>> routeMatchers = {})
    {
        constexpr auto paramIds = detail::readPathParamIds<path>();
        constexpr auto paramTraits = detail::paramIdsToParamTraitList<paramIds>();
        auto pathStr = detail::prepareRegexString(path.str());
        paramTraits.for_each(
                [&](auto paramTraitTypeId)
                {
                    using Param = typename decltype(paramTraitTypeId)::type;
                    static_assert(
                            sfun::is_complete_type_v<Param>,
                            "Trying to use an unregistered parameter name in route");
                    pathStr = sfun::replace(
                            pathStr,
                            "\\{" + std::string{Param::name} + "\\}",
                            "(" + std::string{Param::regex} + ")");
                });

        constexpr auto paramTypes = detail::paramIdsToParamTypeList<paramIds>();
        return makeRegexRoute<paramTypes>(pathStr, std::move(routeMatchers));
    }
#endif

    auto& dynamicRouteImpl(
            const std::string& path,
            std::vector<detail::RouteMatcherInvoker<TRequest, TRouteContext>> routeMatchers = {})
    {
        auto params = detail::readPathParams(path);
        auto pathStr = detail::prepareRegexString(path);
        for (const auto& param : params) {
            auto paramRegex = routeParamRegex(param);
            if (!paramRegex.has_value())
                onUnregisteredRouteParameterError(param);

            pathStr = sfun::replace(pathStr, "\\{" + param + "\\}", "(" + std::string{paramRegex.value()} + ")");
        }
        return makeRegexRoute(pathStr, std::move(routeMatchers));
    }

    virtual std::string getRequestPath(const TRequest&) = 0;
    virtual void processUnmatchedRequest(const TRequest&, TResponse&) = 0;
    virtual void onRouteParametersError(const TRequest&, TResponse&, const RouteParameterError&) {};

    virtual std::optional<std::string_view> routeParamRegex(std::string_view /*paramName*/) const
    {
        return std::nullopt;
    }

    virtual void onUnregisteredRouteParameterError(std::string_view paramName) const
    {
        throw std::runtime_error{sfun::join_strings(
                "Regular expression for route parameter '",
                paramName,
                "' isn't registered. Override RequestRouter::routeParamRegex() method to add it.")};
    }

    virtual bool isRouteProcessingFinished(const TRequest&, TResponse&) const
    {
        return true;
    }

    std::function<void(const TRequest&, TResponse&, const RouteParameterError&)> routeParametersErrorHandler()
    {
        return [this](const TRequest& request, TResponse& response, const RouteParameterError& error)
        {
            this->onRouteParametersError(request, response, error);
        };
    }

    auto makeRegexMatchProcessor(const TRequest& request, TResponse& response)
    {
        return [&](const RegExpRouteMatch& match) -> std::vector<std::function<bool(TRouteContext&)>>
        {
            auto matchList = std::smatch{};
            const auto requestPath = this->getRequestPath(request);

            const auto getAlternativeTrailingSlashPath = [](const std::string& requestPath) -> std::string
            {
                if (sfun::ends_with(requestPath, "/"))
                    return {requestPath.begin(), std::prev(requestPath.end())};
                return requestPath + "/";
            };

            auto [result, routeParams] = detail::matchRegex(requestPath, match.regExp);
            if (result)
                return makeRequestProcessorInvokerList(
                        match.getRouteRequestProcessors(),
                        request,
                        response,
                        routeParams);

            if (trailingSlashMode_ == TrailingSlashMode::Optional && requestPath != "/") {
                auto [retryResult, retryRouteParams] =
                        detail::matchRegex(getAlternativeTrailingSlashPath(requestPath), match.regExp);
                if (retryResult)
                    return makeRequestProcessorInvokerList(
                            match.getRouteRequestProcessors(),
                            request,
                            response,
                            retryRouteParams);
            }
            return {};
        };
    }

    std::vector<std::function<bool(TRouteContext&)>> makeRouteRequestProcessorInvokerList(
            const TRequest& request,
            TResponse& response)
    {
        auto result = std::vector<std::function<bool(TRouteContext&)>>{};
        for (auto& match : routeMatchList_)
            detail::concat(result, makeRegexMatchProcessor(request, response)(match));

        return result;
    }

    std::vector<std::function<bool(TRouteContext&)>> makeRequestProcessorInvokerList(
            const std::vector<RequestProcessorFunc>& processorList,
            const TRequest& request,
            TResponse& response,
            const std::vector<std::string>& routeParams)
    {
        auto result = std::vector<std::function<bool(TRouteContext&)>>{};
        for (const auto& processor : processorList) {
            auto checkIfFinished = (&processor == &processorList.back());
            result.emplace_back(
                    [request, response, processor, checkIfFinished, routeParams, this](
                            TRouteContext& routeContext) mutable -> bool
                    {
                        processor(request, response, routeParams, routeContext);
                        if (checkIfFinished)
                            return !isRouteProcessingFinished(request, response);
                        else
                            return true;
                    });
        }
        return result;
    };

    template<auto checkParam = nullptr>
    auto& makeRegexRoute(
            const std::string& regex,
            std::vector<detail::RouteMatcherInvoker<TRequest, TRouteContext>> routeMatchers = {})
    {
        using RouteType = detail::Route<TRequest, TResponse, TResponseConverter, TRouteContext, checkParam>;
        auto routeMatch = RegExpRouteMatch{};
        routeMatch.regExp = std::regex{regex};
        routeMatch.route = RouteType{std::move(routeMatchers), routeParametersErrorHandler()};
        routeMatchList_.emplace_back(routeMatch);
        routeMatchList_.back().getRouteRequestProcessors =
                [&route = std::any_cast<RouteType&>(routeMatchList_.back().route)]()
        {
            return route.getRequestProcessors();
        };
        return std::any_cast<RouteType&>(routeMatchList_.back().route);
    }

private:
    std::deque<RegExpRouteMatch> routeMatchList_;
    detail::Route<TRequest, TResponse, TResponseConverter, TRouteContext, nullptr> noMatchRoute_;
    TrailingSlashMode trailingSlashMode_ = TrailingSlashMode::Optional;
};

} // namespace whaleroute

#endif // WHALEROUTE_REQUESTROUTER_H
