#pragma once
#include <whaleroute/types.h>

namespace whaleroute::detail{

template<typename TRequest, typename TResponse, typename TRequestProcessor>
class IRequestRouterWithRequestProcessor {
public:
    virtual ~IRequestRouterWithRequestProcessor() = default;

    virtual void callRequestProcessor(TRequestProcessor&, const TRequest&, TResponse&) = 0;
};

template<typename TResponse, typename TResponseValue>
class IRequestRouterWithResponseValue {
public:
    virtual ~IRequestRouterWithResponseValue() = default;

    virtual void setResponseValue(TResponse&, const TResponseValue&) = 0;
};

template <typename T>
class Without{};

template <typename TRequest, typename TResponse, typename TRequestProcessor>
using MaybeWithRequestProcessor = std::conditional_t<!std::is_same_v<TRequestProcessor, _>,
                                                     IRequestRouterWithRequestProcessor<TRequest, TResponse, TRequestProcessor>,
                                                     Without<IRequestRouterWithRequestProcessor<TRequest, TResponse, TRequestProcessor>>>;

template <typename TResponse, typename TResponseValue>
using MaybeWithResponseValue = std::conditional_t<!std::is_same_v<TResponseValue, _>,
                                                  IRequestRouterWithResponseValue<TResponse, TResponseValue>,
                                                  Without<IRequestRouterWithResponseValue<TResponse, TResponseValue>>>;

template<typename TRequest, typename TResponse, typename TRequestProcessor, typename TResponseValue>
class IRequestRouter : public MaybeWithRequestProcessor<TRequest, TResponse, TRequestProcessor>,
                       public MaybeWithResponseValue<TResponse, TResponseValue>{
public:
    virtual ~IRequestRouter() = default;
    virtual std::string getRequestPath(const TRequest&) = 0;
    virtual void processUnmatchedRequest(const TRequest&, TResponse&) = 0;
};

}