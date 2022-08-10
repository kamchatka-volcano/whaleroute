#pragma once
#include "irequestrouter.h"
#include "routerequesttype.h"
#include <whaleroute/types.h>
#include <functional>
#include <vector>
#include <string>
#include <memory>

namespace whaleroute{
template <typename TRequest, typename TResponse, typename TRequestType, typename TRequestProcessor, typename TResponseValue>
class RequestRouter;
}

namespace whaleroute::detail{

template <typename TRequest, typename TResponse, typename TRequestType, typename TRequestProcessor, typename TResponseValue>
class Route{
    friend class RequestRouter<TRequest, TResponse, TRequestType, TRequestProcessor, TResponseValue>;
    using ProcessingFunc = std::function<void(const TRequest&, TResponse&)>;
    using Processor = std::tuple<RouteRequestType<TRequestType>, ProcessingFunc>;

public:
    Route(IRequestRouter<TRequest, TResponse, TRequestType, TRequestProcessor, TResponseValue>& router)
        : requestType_{_{}}
        , router_{router}
    {
    }

    template<typename TProcessor, typename T = TRequestProcessor, typename... TArgs>
    auto process(TArgs&&... args) -> std::enable_if_t<!std::is_same_v<T, _>, Route&>
    {
        static_assert(std::is_base_of<TRequestProcessor, TProcessor>::value, "TProcessor must inherit from RequestProcessor");
        requestProcessor_ = std::make_unique<TProcessor>(std::forward<TArgs>(args)...);
        auto processor = std::make_tuple(requestType_,
                                         [this](const TRequest& request, TResponse& response)
                                         {
                                             router_.callRequestProcessor(*requestProcessor_, request, response);
                                         });
        processorList_.emplace_back(std::move(processor));
        return *this;
    }

    template<typename TProcessor, typename T = TRequestProcessor>
    auto process(TProcessor& requestProcessor)-> std::enable_if_t<!std::is_same_v<T, _>, Route&>
    {
        static_assert(std::is_base_of<TRequestProcessor, TProcessor>::value, "TProcessor must inherit from RequestProcessor");
        auto processor = std::make_tuple(requestType_,
                                         [&requestProcessor, this](const TRequest& request, TResponse& response)
                                         {
                                             router_.callRequestProcessor(requestProcessor, request, response);
                                         });
        processorList_.emplace_back(std::move(processor));
        return *this;
    }

    Route& process(std::function<void(const TRequest&, TResponse&)> requestProcessor)
    {
        auto processor = std::make_tuple(requestType_,
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
        auto processor = std::make_tuple(requestType_,
                                         [responseValue, this](const TRequest&, TResponse& response) mutable
                                         {
                                             router_.setResponseValue(response, responseValue);
                                         });
        processorList_.emplace_back(std::move(processor));
    }

private:
    void setRequestType(RouteRequestType<TRequestType> requestType)
    {
        requestType_ = requestType;
    }

    template<typename T = TRequestType>
    auto getRequestProcessor(const TRequest &request, TResponse&) -> std::enable_if_t<!std::is_same_v<T, _>, std::vector<ProcessingFunc>>
    {
        auto result = std::vector<ProcessingFunc>{};
        for (auto& processor : processorList_){
            auto[requestType, processingFunc] = processor;
            if (requestType.isAny() ||
                requestType == router_.getRequestType(request)){
                result.emplace_back(std::move(processingFunc));
            }
        }
        return result;
    }

    template<typename T = TRequestType>
    auto getRequestProcessor(const TRequest &, TResponse&) -> std::enable_if_t<std::is_same_v<T, _>, std::vector<ProcessingFunc>>
    {
        auto result = std::vector<ProcessingFunc>{};
        for (auto& processor : processorList_){
            auto[requestType, processingFunc] = processor;
            result.emplace_back(std::move(processingFunc));
        }
        return result;
    }

private:
    RouteRequestType<TRequestType> requestType_;
    std::vector<Processor> processorList_;
    std::unique_ptr<TRequestProcessor> requestProcessor_;
    IRequestRouter<TRequest, TResponse, TRequestType, TRequestProcessor, TResponseValue>& router_;
};

}

