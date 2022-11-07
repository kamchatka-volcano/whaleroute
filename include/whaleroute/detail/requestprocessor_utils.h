#pragma once
#include "utils.h"
#include <variant>

namespace whaleroute::detail{

template<typename TRequestProcessor, typename TRequest, typename TResponse>
constexpr void checkRequestProcessorSignature()
{
    using args = callable_args<TRequestProcessor>;
    constexpr auto argsSize = std::tuple_size_v<args>;
    static_assert(argsSize >= 2);
    static_assert(std::is_same_v<const TRequest&, std::tuple_element_t<argsSize - 2, args>>);
    static_assert(std::is_same_v<TResponse&, std::tuple_element_t<argsSize - 1, args>>);
}

template<typename TArgsTuple>
using requestProcessorArgsRouteParams = typename decltype(makeDecaySubtuple<TArgsTuple>(std::make_index_sequence<std::tuple_size_v<TArgsTuple> - 2>()))::type;

template<typename TArgsTuple>
auto readRouteParams(const std::vector<std::string>& routeParams) -> std::variant<requestProcessorArgsRouteParams<TArgsTuple>, RouteParameterError>
{
    constexpr auto argsSize = std::tuple_size_v<TArgsTuple>;
    constexpr auto paramsSize = argsSize - 2;
    static_assert(paramsSize > 0);

    if (routeParams.size() < paramsSize)
        return RouteParameterCountMismatch{paramsSize, static_cast<int>(routeParams.size())};

    auto params = requestProcessorArgsRouteParams<TArgsTuple>{};
    auto i = std::size_t{};
    auto error = std::optional<RouteParameterError>{};
    auto readParam = [&](auto& val) {
        if (error)
            return;

        const auto& param = routeParams.at(i++);
        auto paramValue = detail::convertFromString<std::decay_t<decltype(val)>>(param);
        if (paramValue)
            val = *paramValue;
        else
            error = RouteParameterReadError{static_cast<int>(i), param};
    };

    std::apply([readParam](auto& ... paramValues) {
        ((readParam(paramValues)), ...);
    }, params);
    if (error)
        return *error;

    return params;
}

template<typename TRequestProcessor, typename TRequest, typename TResponse>
void invokeRequestProcessor(
        TRequestProcessor& requestProcessor,
        const TRequest& request,
        TResponse& response,
        const std::vector<std::string>& routeParams,
        std::function<void(const TRequest&, TResponse&, const RouteParameterError&)> routeParamErrorHandler)
{
    checkRequestProcessorSignature<TRequestProcessor, TRequest, TResponse>();

    using args_t = callable_args<TRequestProcessor>;
    constexpr auto argsSize = std::tuple_size_v<args_t>;
    constexpr auto paramsSize = argsSize - 2;
    if constexpr (!paramsSize) {
        requestProcessor(request, response);
        return;
    }
    else {
        auto paramsResult = readRouteParams<args_t>(routeParams);
        std::visit(overloaded{
                [&](const RouteParameterError& error) { routeParamErrorHandler(request, response, error); },
                [&](const auto& params) {
                    auto callProcess = [&](const auto& ... param) {
                        requestProcessor(param..., request, response);
                    };
                    std::apply(callProcess, params);
                }
        }, paramsResult);
    }
}

}