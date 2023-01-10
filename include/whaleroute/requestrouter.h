#ifndef WHALEROUTE_REQUESTROUTER_H
#define WHALEROUTE_REQUESTROUTER_H

#include "requestprocessorqueue.h"
#include "types.h"
#include "detail/external/sfun/functional.h"
#include "detail/external/sfun/interface.h"
#include "detail/irequestrouter.h"
#include "detail/route.h"
#include "detail/utils.h"
#include <deque>
#include <regex>
#include <variant>

namespace whaleroute {

template<typename TRequest, typename TResponse, typename TResponseValue = _, typename TRouteContext = _>
class RequestRouter : private detail::IRequestRouter<TRequest, TResponse, TResponseValue> {
    using Route = detail::Route<TRequest, TResponse, TResponseValue, TRouteContext>;
    using RequestProcessorFunc =
            std::function<void(const TRequest&, TResponse&, const std::vector<std::string>&, TRouteContext&)>;

    struct RegExpRouteMatch {
        std::regex regExp;
        Route route;
    };
    struct PathRouteMatch {
        std::string path;
        Route route;
    };
    using RouteMatch = std::variant<RegExpRouteMatch, PathRouteMatch>;

public:
    RequestRouter()
        : noMatchRoute_{*this, {}, routeParametersErrorHandler()}
    {
    }

    void setTrailingSlashMode(TrailingSlashMode mode)
    {
        trailingSlashMode_ = mode;
    }

    void setRegexMode(RegexMode mode)
    {
        regexMode_ = mode;
    }

    template<typename... TRouteMatcherArgs>
    Route& route(const std::string& path, TRouteMatcherArgs&&... matcherArgs)
    {
        return pathRouteImpl(path, {std::forward<TRouteMatcherArgs>(matcherArgs)...});
    }

    Route& route(const std::string& path)
    {
        return pathRouteImpl(path, {});
    }

    template<typename... TRouteMatcherArgs>
    Route& route(const rx& regExp, TRouteMatcherArgs&&... matcherArgs)
    {
        return regexRouteImpl(regExp, {std::forward<TRouteMatcherArgs>(matcherArgs)...});
    }

    Route& route(const rx& regExp)
    {
        return regexRouteImpl(regExp, {});
    }

    Route& route()
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
        for (const auto& processor : noMatchRoute_.getRequestProcessors(sfun::AccessToken{*this}))
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
            const auto requestPath = detail::makePath(this->getRequestPath(request), trailingSlashMode_);
            if (!std::regex_match(requestPath, matchList, match.regExp))
                return {};

            auto routeParams = std::vector<std::string>{};
            for (auto i = 1u; i < matchList.size(); ++i)
                routeParams.push_back(matchList[i].str());
            return makeRequestProcessorInvokerList(
                    match.route.getRequestProcessors(sfun::AccessToken{*this}),
                    request,
                    response,
                    routeParams);
        };
    }

    auto makePathMatchProcessor(const TRequest& request, TResponse& response)
    {
        return [&](const PathRouteMatch& match) -> std::vector<std::function<bool(TRouteContext&)>>
        {
            if (match.path == detail::makePath(this->getRequestPath(request), trailingSlashMode_))
                return makeRequestProcessorInvokerList(
                        match.route.getRequestProcessors(sfun::AccessToken{*this}),
                        request,
                        response,
                        {});
            else
                return {};
        };
    }

    std::vector<std::function<bool(TRouteContext&)>> makeRouteRequestProcessorInvokerList(
            const TRequest& request,
            TResponse& response)
    {
        auto result = std::vector<std::function<bool(TRouteContext&)>>{};
        const auto matchVisitor =
                sfun::overloaded{makeRegexMatchProcessor(request, response), makePathMatchProcessor(request, response)};
        for (auto& match : routeMatchList_)
            detail::concat(result, std::visit(matchVisitor, match));

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

    Route& pathRouteImpl(
            const std::string& path,
            std::vector<detail::RouteMatcherInvoker<TRequest, TResponse, TRouteContext>> routeMatchers = {})
    {
        auto routePath = detail::makePath(path, trailingSlashMode_);
        auto& routeMatch = routeMatchList_.emplace_back(
                PathRouteMatch{routePath, Route{*this, routeMatchers, routeParametersErrorHandler()}});
        return std::get<PathRouteMatch>(routeMatch).route;
    }

    Route& regexRouteImpl(
            const rx& regExp,
            std::vector<detail::RouteMatcherInvoker<TRequest, TResponse, TRouteContext>> routeMatchers = {})
    {
        auto& routeMatch = routeMatchList_.emplace_back(RegExpRouteMatch{
                detail::makeRegex(regExp, regexMode_, trailingSlashMode_),
                {*this, std::move(routeMatchers), routeParametersErrorHandler()}});
        return std::get<RegExpRouteMatch>(routeMatch).route;
    }

private:
    std::deque<RouteMatch> routeMatchList_;
    Route noMatchRoute_;
    TrailingSlashMode trailingSlashMode_ = TrailingSlashMode::Optional;
    RegexMode regexMode_ = RegexMode::Regular;
};

} // namespace whaleroute

#endif // WHALEROUTE_REQUESTROUTER_H
