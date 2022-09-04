#pragma once
#include "detail/irequestprocessor.h"
#include "detail/utils.h"
#include <type_traits>
#include <tuple>
#include <sstream>
#include <vector>

namespace whaleroute{

template <typename TRequest, typename TResponse, typename... TRouteParam>
class RequestProcessor : public detail::IRequestProcessor<TRequest, TResponse>
{
public:
    virtual void process(const TRouteParam&..., const TRequest&, TResponse&) = 0;
    virtual void onRouteParametersError(const TRequest&, TResponse&, const RouteParameterError&){};

protected:
    void processRouterRequest(const TRequest& request, TResponse& response, const std::vector<std::string>& routeParams) override
    {
        auto param = std::tuple<TRouteParam...>{};
        auto i = 0u;
        auto error = std::optional<RouteParameterError>{};
        auto readParam = [&](auto& val){
            if (error)
                return;

            if (i >= routeParams.size()){
                error = RouteParameterCountMismatch{std::tuple_size_v<decltype(param)>, static_cast<int>(routeParams.size())};
                return;
            }
            const auto& param = routeParams.at(i++);
            auto paramValue = detail::convertFromString<std::decay_t<decltype(val)>>(param);
            if (paramValue)
                val = *paramValue;
            else
                error = RouteParameterReadError{static_cast<int>(i), param};
        };

        std::apply([readParam](auto&... paramValues){
            ((readParam(paramValues)),...);
        }, param);

        if (error) {
            onRouteParametersError(request, response, *error);
            return;
        }

        auto callProcess = [&](const TRouteParam&... arg){
            process(arg..., request, response);
        };
        std::apply(callProcess, param);
    }
};

}