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

template <typename TRequest, typename TResponse, typename TResponseValue = _>
class RequestRouter : public detail::IRequestRouter<TRequest, TResponse, TResponseValue> {
    using TRoute = detail::Route<TRequest, TResponse, TResponseValue>;

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
    }

    void setTrailingSlashMode(TrailingSlashMode mode)
    {
        trailingSlashMode_ = mode;
    }

    void setRegexMode(RegexMode mode)
    {
        regexMode_ = mode;
    }

    template<typename... TRouteSpecificationArgs>
    TRoute& route(const std::string& path, TRouteSpecificationArgs&&... spec)
    {
        return pathRouteImpl(path, {std::forward<TRouteSpecificationArgs>(spec)...});
    }

    TRoute& route(const std::string& path)
    {
        return pathRouteImpl(path, {});
    }

    template<typename... TRouteSpecificationArgs>
    TRoute& route(const rx& regExp, TRouteSpecificationArgs&&... spec)
    {
        return regexRouteImpl(regExp, {std::forward<TRouteSpecificationArgs>(spec)...});
    }

    TRoute& route(const rx& regExp)
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
        auto requestProcessorInvokerList = makeRouteRequestProcessorInvokerList(request, response);
        for (const auto& processor : noMatchRoute_.getRequestProcessor(request, response))
            requestProcessorInvokerList.emplace_back([request, response, processor]() mutable -> bool{
                processor(request, response, {});
                return false;
            });

        if (requestProcessorInvokerList.empty())
            requestProcessorInvokerList.emplace_back([request, response, this]() mutable -> bool{
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

    std::vector<std::function<bool()>> makeRouteRequestProcessorInvokerList(const TRequest& request, TResponse& response)
    {
        auto result = std::vector<std::function<bool()>>{};
        for (auto& match : routeMatchList_) {
            std::visit([&](auto& match){
                using T = std::decay_t<decltype(match)>;
                if constexpr(std::is_same_v<T, RegExpRouteMatch>){
                    auto matchList = std::smatch{};
                    const auto requestPath = detail::makePath(this->getRequestPath(request), trailingSlashMode_);
                    if (std::regex_match(requestPath, matchList, match.regExp)) {
                        auto routeParams = std::vector<std::string>{};
                        for (auto i = 1u; i < matchList.size(); ++i)
                            routeParams.push_back(matchList[i].str());

                        detail::concat(result,
                                       makeRequestProcessorInvokerList(
                                               match.route.getRequestProcessor(request, response), request, response, routeParams));
                    }

                }
                else if constexpr(std::is_same_v<T, PathRouteMatch>){
                    if (match.path == detail::makePath(this->getRequestPath(request), trailingSlashMode_))
                        detail::concat(result,
                                       makeRequestProcessorInvokerList(
                                               match.route.getRequestProcessor(request, response), request, response, {}));
                }
            }, match);
        }
        return result;
    }

    std::vector<std::function<bool()>> makeRequestProcessorInvokerList(
            const std::vector<std::function<void(const TRequest&, TResponse&, const std::vector<std::string>&)>>& processorList,
            const TRequest& request,
            TResponse& response,
            const std::vector<std::string>& routeParams)
    {
        auto result = std::vector<std::function<bool()>>{};
        for (const auto& processor : processorList) {
            auto checkIfFinished = (&processor == &processorList.back());
            result.emplace_back([request, response, processor, checkIfFinished, routeParams, this]() mutable -> bool{
                processor(request, response, routeParams);
                if (checkIfFinished)
                    return !isRouteProcessingFinished(request, response);
                else
                    return true;
            });
        }
        return result;
    };

    TRoute& pathRouteImpl(
            const std::string& path,
            std::vector<detail::RouteSpecifier<TRequest, TResponse>> routeSpecifications = {})
    {
        auto routePath = detail::makePath(path, trailingSlashMode_);
        auto& routeMatch = routeMatchList_.emplace_back(PathRouteMatch{routePath, TRoute{*this, routeSpecifications}});
        return std::get<PathRouteMatch>(routeMatch).route;
    }

    TRoute& regexRouteImpl(
            const rx& regExp,
            std::vector<detail::RouteSpecifier<TRequest, TResponse>> routeSpecifications = {})
    {
        auto& routeMatch = routeMatchList_.emplace_back(
                RegExpRouteMatch{detail::makeRegex(regExp, regexMode_, trailingSlashMode_), {*this, std::move(routeSpecifications)}});
        return std::get<RegExpRouteMatch>(routeMatch).route;
    }

private:
    std::deque<RouteMatch> routeMatchList_;
    TRoute noMatchRoute_;
    TrailingSlashMode trailingSlashMode_ = TrailingSlashMode::Optional;
    RegexMode regexMode_ = RegexMode::Regular;
};

}
