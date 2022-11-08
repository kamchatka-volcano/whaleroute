#ifndef WHALEROUTE_REQUESTPROCESSOR_H
#define WHALEROUTE_REQUESTPROCESSOR_H

#include "utils.h"
#include <variant>

namespace whaleroute::detail{

template<typename TRequestProcessor, typename TRequest, typename TResponse>
constexpr void checkRequestProcessorSignature()
{
    using args = callable_args<TRequestProcessor>;
    constexpr auto argsSize = std::tuple_size_v<args>;
    static_assert(argsSize >= 2);
    static_assert(std::is_same_v<const TRequest&, typename decltype(typeListElement<argsSize - 2, args>())::type>);
    static_assert(std::is_same_v<TResponse&, typename decltype(typeListElement<argsSize - 1, args>())::type>);
}

template<typename TArgsTypeList>
using requestProcessorArgsRouteParams = decay_tuple<type_list_elements_tuple<TArgsTypeList, type_list_size<TArgsTypeList> - 2>>;

template<typename TArgsTypeList>
auto readRouteParams(const std::vector<std::string>& routeParams) -> std::variant<requestProcessorArgsRouteParams<TArgsTypeList>, RouteParameterError>
{
    constexpr auto argsSize = std::tuple_size_v<TArgsTypeList>;
    constexpr auto paramsSize = argsSize - 2;
    static_assert(paramsSize > 0);

    if (routeParams.size() < paramsSize)
        return RouteParameterCountMismatch{paramsSize, static_cast<int>(routeParams.size())};

    auto params = requestProcessorArgsRouteParams<TArgsTypeList>{};
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

    using argsTypeList = callable_args<TRequestProcessor>;
    constexpr auto argsSize = std::tuple_size_v<argsTypeList>;
    constexpr auto paramsSize = argsSize - 2;
    if constexpr (!paramsSize) {
        requestProcessor(request, response);
        return;
    }
    else {
        auto paramsResult = readRouteParams<argsTypeList>(routeParams);
        std::visit(overloaded{
                [&](const RouteParameterError& error) {
                    if (routeParamErrorHandler)
                        routeParamErrorHandler(request, response, error);
                },
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

#endif //WHALEROUTE_REQUESTPROCESSOR_H