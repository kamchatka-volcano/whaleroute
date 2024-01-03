#ifndef WHALEROUTE_IREQUESTROUTER_H
#define WHALEROUTE_IREQUESTROUTER_H

#include "types.h"
#include "external/sfun/interface.h"

namespace whaleroute::detail {

template<typename TRequest, typename TResponse>
class IRequestRouter : private sfun::interface<IRequestRouter<TRequest, TResponse>> {
public:
    virtual std::string getRequestPath(const TRequest&) = 0;
    virtual void processUnmatchedRequest(const TRequest&, TResponse&) = 0;
    virtual void onRouteParametersError(const TRequest&, TResponse&, const RouteParameterError&){};
};

} // namespace whaleroute::detail

#endif // WHALEROUTE_IREQUESTROUTER_H