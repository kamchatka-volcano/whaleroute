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

template<typename TParamsTuple>
auto makeParams(const std::vector<std::string>& routeParams) -> std::variant<TParamsTuple, RouteParameterError>
{
    auto i = 0;
    auto error = std::optional<RouteParameterError>{};
    auto readParam = [&](auto& val){
        static_assert(!std::is_base_of_v<detail::RouteParameters, std::decay_t<decltype(val)>>,
                "If you use RouteParameters, it must be the only parameter argument of the request processor callable");
        if (error.has_value())
            return;

        const auto& param = routeParams.at(i++);
        auto paramValue = detail::convertFromString<std::decay_t<decltype(val)>>(param);
        if (paramValue)
            val = *paramValue;
        else
            error = RouteParameterReadError{i, param};
    };
    auto params = TParamsTuple{};
    std::apply([readParam](auto& ... paramValues){
        ((readParam(paramValues)), ...);
    }, params);

    if (error)
        return error.value();
    return params;
}

template<typename TArgsTypeList>
auto readRouteParams(const std::vector<std::string>& routeParams) -> std::variant<requestProcessorArgsRouteParams<TArgsTypeList>, RouteParameterError>
{
    constexpr auto argsSize = std::tuple_size_v<TArgsTypeList>;
    constexpr auto paramsSize = argsSize - 2;
    static_assert(paramsSize > 0);

    if (paramsSize > routeParams.size())
        return RouteParameterCountMismatch{paramsSize, static_cast<int>(routeParams.size())};

    using ParamsTuple = requestProcessorArgsRouteParams<TArgsTypeList>;
    if constexpr (std::tuple_size_v<ParamsTuple> == 1 && std::is_base_of_v<detail::RouteParameters, std::tuple_element_t<0, ParamsTuple>>){
        auto params = ParamsTuple{};
        auto& paramList = std::get<0>(params);
        paramList.value = routeParams;
        if (paramList.numOfElements && *paramList.numOfElements > static_cast<int>(routeParams.size()))
            return RouteParameterCountMismatch{*paramList.numOfElements, static_cast<int>(routeParams.size())};
        return params;
    }
    else
        return makeParams<ParamsTuple>(routeParams);
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