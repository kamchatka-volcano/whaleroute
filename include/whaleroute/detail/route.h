#pragma once
#include "irequestrouter.h"
#include <whaleroute/types.h>
#include <functional>
#include <vector>
#include <string>
#include <memory>

namespace whaleroute{
template <typename TRequestProcessor, typename TRequest, typename TRequestMethod, typename TResponse, typename TResponseValue>
class RequestRouter;
}

namespace whaleroute::detail{
template<typename TRequestMethod>
class RouteRequestMethod{
public:
    template <typename T = TRequestMethod, typename = std::enable_if_t<!std::is_same_v<T, _>>>
    RouteRequestMethod(TRequestMethod method)
        : requestMethod_(method)
        , isAny_(false)
    {}

    RouteRequestMethod(_)
        : isAny_(true)
    {}

    operator TRequestMethod() const
    {
        return requestMethod_;
    }

    bool isAny() const
    {
        return isAny_;
    }


private:
    TRequestMethod requestMethod_;
    bool isAny_;
};


template <typename TRequestProcessor>
class RequestProcessorSet;

template <typename TRequestProcessor, typename TRequest, typename TRequestMethod, typename TResponse, typename TResponseValue>
class Route{    
    friend class RequestRouter<TRequestProcessor, TRequest, TRequestMethod, TResponse, TResponseValue>;
    using ProcessingFunc = std::function<void(const TRequest&, TResponse&)>;
    using Processor = std::tuple<RouteRequestMethod<TRequestMethod>, ProcessingFunc>;

public:
    Route(RequestProcessorSet<TRequestProcessor>& requestProcessorSet, IRequestRouter<TRequestProcessor, TRequest, TRequestMethod, TResponse, TResponseValue>& router)
        : requestMethod_(_{})
        , requestProcessorSet_(requestProcessorSet)
        , router_(router)
    {
    }

    template<typename TProcessor, typename T = TRequestProcessor>
    auto process() -> std::enable_if_t<!std::is_same_v<T, _>, Route&>
    {
        static_assert(std::is_base_of<TRequestProcessor, TProcessor>::value, "TProcessor must inherit from RequestProcessor");
        auto& requestProcessor = requestProcessorSet_.template get<TProcessor>();
        auto processor = std::make_tuple(requestMethod_,
                                         [&requestProcessor, this](const TRequest& request, TResponse& response)
                                         {
                                             router_.callRequestProcessor(requestProcessor, request, response);
                                         });
        processorList_.emplace_back(std::move(processor));
        return *this;
    }

    template<typename TProcessor, typename T = TRequestProcessor, typename... TArgs>
    auto process(TArgs&&... args) -> std::enable_if_t<!std::is_same_v<T, _>, Route&>
    {
        static_assert(std::is_base_of<TRequestProcessor, TProcessor>::value, "TProcessor must inherit from RequestProcessor");
        auto& requestProcessor = requestProcessorSet_.template get<TProcessor>(std::forward<TArgs>(args)...);
        auto processor = std::make_tuple(requestMethod_,
                                         [&requestProcessor, this](const TRequest& request, TResponse& response)
                                         {
                                             router_.callRequestProcessor(requestProcessor, request, response);
                                         });
        processorList_.emplace_back(std::move(processor));
        return *this;
    }

    template<typename TProcessor, typename T = TRequestProcessor>
    auto process(TProcessor& requestProcessor)-> std::enable_if_t<!std::is_same_v<T, _>, Route&>
    {
        static_assert(std::is_base_of<TRequestProcessor, TProcessor>::value, "TProcessor must inherit from RequestProcessor");
        auto processor = std::make_tuple(requestMethod_,
                                         [&requestProcessor, this](const TRequest& request, TResponse& response)
                                         {
                                             router_.callRequestProcessor(requestProcessor, request, response);
                                         });
        processorList_.emplace_back(std::move(processor));
        return *this;
    }

    Route& process(std::function<void(const TRequest&, TResponse&)> requestProcessor)
    {
        auto processor = std::make_tuple(requestMethod_,
                                         [requestProcessor](const TRequest& request, TResponse& response)
                                         {
                                             requestProcessor(request, response);
                                         });
        processorList_.emplace_back(std::move(processor));
        return *this;
    }

    template<typename TCheckResponseValue = _,
            typename = std::enable_if_t<!std::is_same_v<TResponseValue, TCheckResponseValue>>>
    void set(const TResponseValue& responseValue)
    {
        auto processor = std::make_tuple(requestMethod_,
                                         [responseValue, this](const TRequest&, TResponse& response) mutable
                                         {
                                             router_.setResponse(response, responseValue);
                                         });
        processorList_.emplace_back(std::move(processor));
    }

private:
    void setRequestMethod(RouteRequestMethod<TRequestMethod> requestMethod)
    {
        requestMethod_ = requestMethod;
    }

    bool processRequest(const TRequest &request, TResponse& response)
    {
        auto matched = false;
        for (auto& processor : processorList_){
            auto[processorRequestMethod, processingFunc] = processor;
            if (processorRequestMethod.isAny() ||
                static_cast<TRequestMethod>(processorRequestMethod) == router_.getRequestMethod(request)){
                processingFunc(request, response);
                matched = true;
            }
        }
        return matched;
    }

private:
    RouteRequestMethod<TRequestMethod> requestMethod_;
    std::vector<Processor> processorList_;
    RequestProcessorSet<TRequestProcessor>& requestProcessorSet_;
    IRequestRouter<TRequestProcessor, TRequest, TRequestMethod, TResponse, TResponseValue>& router_;
};

}

