#ifndef WHALEROUTE_IREQUESTROUTER_H
#define WHALEROUTE_IREQUESTROUTER_H

#include <whaleroute/types.h>

namespace whaleroute::detail {

template <typename TResponse, typename TResponseValue>
class IRequestRouterWithResponseValue {
public:
    virtual ~IRequestRouterWithResponseValue() = default;
    virtual void setResponseValue(TResponse&, const TResponseValue&) = 0;
};

template <typename T>
class Without {};

template <typename TResponse, typename TResponseValue>
using MaybeWithResponseValue = std::conditional_t<
        !std::is_same_v<TResponseValue, _>,
        IRequestRouterWithResponseValue<TResponse, TResponseValue>,
        Without<IRequestRouterWithResponseValue<TResponse, TResponseValue>>>;

template <typename TRequest, typename TResponse, typename TResponseValue>
class IRequestRouter : public MaybeWithResponseValue<TResponse, TResponseValue> {
public:
    virtual ~IRequestRouter() = default;
    virtual std::string getRequestPath(const TRequest&) = 0;
    virtual void processUnmatchedRequest(const TRequest&, TResponse&) = 0;
    virtual void onRouteParametersError(const TRequest&, TResponse&, const RouteParameterError&){};
};

} // namespace whaleroute::detail

#endif // WHALEROUTE_IREQUESTROUTER_H