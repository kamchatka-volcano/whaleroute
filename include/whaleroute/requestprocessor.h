#ifndef WHALEROUTE_REQUESTPROCESSOR_H
#define WHALEROUTE_REQUESTPROCESSOR_H

#include "utils.h"
#include "external/sfun/functional.h"
#include "external/sfun/type_traits.h"
#include <variant>

namespace whaleroute::detail {

template<typename TRequestProcessor, typename TRequest, typename TResponse, typename TRouteContext>
constexpr void checkRequestProcessorSignature()
{
    constexpr auto args = sfun::callable_args<TRequestProcessor>{};
    using returnType = sfun::callable_return_type<TRequestProcessor>;
    if constexpr (std::is_same_v<returnType, void>) {
        static_assert(args.size() >= 2);
        if constexpr (std::is_same_v<TRouteContext, _>) {
            static_assert(std::is_same_v<const TRequest&, typename decltype(sfun::get<args.size() - 2>(args))::type>);
            static_assert(std::is_same_v<TResponse&, typename decltype(sfun::get<args.size() - 1>(args))::type>);
        }
        else {
            if constexpr (args.size() == 2) {
                static_assert(
                        std::is_same_v<const TRequest&, typename decltype(sfun::get<args.size() - 2>(args))::type>);
                static_assert(std::is_same_v<TResponse&, typename decltype(sfun::get<args.size() - 1>(args))::type>);
            }
            else if constexpr (
                    std::is_lvalue_reference_v<typename decltype(sfun::get<args.size() - 1>(args))::type> &&
                    std::is_same_v<
                            TRouteContext,
                            std::decay_t<typename decltype(sfun::get<args.size() - 1>(args))::type>>) {
                static_assert(
                        std::is_same_v<const TRequest&, typename decltype(sfun::get<args.size() - 3>(args))::type>);
                static_assert(std::is_same_v<TResponse&, typename decltype(sfun::get<args.size() - 2>(args))::type>);
            }
            else {
                static_assert(
                        std::is_same_v<const TRequest&, typename decltype(sfun::get<args.size() - 2>(args))::type>);
                static_assert(std::is_same_v<TResponse&, typename decltype(sfun::get<args.size() - 1>(args))::type>);
            }
        }
    }
    else {
        static_assert(args.size() >= 1);
        if constexpr (std::is_same_v<TRouteContext, _>) {
            static_assert(std::is_same_v<const TRequest&, typename decltype(sfun::get<args.size() - 1>(args))::type>);
        }
        else {
            if constexpr (args.size() == 1) {
                static_assert(
                        std::is_same_v<const TRequest&, typename decltype(sfun::get<args.size() - 1>(args))::type>);
            }
            else if constexpr (
                    std::is_lvalue_reference_v<typename decltype(sfun::get<args.size() - 1>(args))::type> &&
                    std::is_same_v<
                            TRouteContext,
                            std::decay_t<typename decltype(sfun::get<args.size() - 1>(args))::type>>) {
                static_assert(
                        std::is_same_v<const TRequest&, typename decltype(sfun::get<args.size() - 2>(args))::type>);
            }
            else {
                static_assert(
                        std::is_same_v<const TRequest&, typename decltype(sfun::get<args.size() - 1>(args))::type>);
            }
        }
    }
}

template<typename TArgsTypeList, std::size_t paramsSize>
using requestProcessorArgsRouteParams =
        sfun::decay_tuple_t<sfun::to_tuple_t<decltype(TArgsTypeList::template slice<0, paramsSize>())>>;

template<typename TParamsTuple>
auto makeParams(const std::vector<std::string>& routeParams) -> std::variant<TParamsTuple, RouteParameterError>
{
    auto i = 0;
    auto error = std::optional<RouteParameterError>{};
    auto readParam = [&](auto& val)
    {
        static_assert(
                !std::is_base_of_v<detail::RouteParameters, std::decay_t<decltype(val)>>,
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
    std::apply(
            [readParam](auto&... paramValues)
            {
                ((readParam(paramValues)), ...);
            },
            params);

    if (error)
        return error.value();
    return params;
}

template<typename TArgsTypeList, int paramsSize>
auto readRouteParams(const std::vector<std::string>& routeParams)
        -> std::variant<requestProcessorArgsRouteParams<TArgsTypeList, paramsSize>, RouteParameterError>
{
    static_assert(paramsSize > 0);
    using ParamsTuple = requestProcessorArgsRouteParams<TArgsTypeList, paramsSize>;
    if constexpr (
            std::tuple_size_v<ParamsTuple> == 1 &&
            std::is_base_of_v<detail::RouteParameters, std::tuple_element_t<0, ParamsTuple>>) {
        using RouteParams = std::tuple_element_t<0, ParamsTuple>;
        auto params = ParamsTuple{};
        auto& paramList = std::get<0>(params);
        paramList.value = routeParams;
        if (RouteParams::MinSize::value > static_cast<int>(routeParams.size()))
            return RouteParameterCountMismatch{RouteParams::MinSize::value, static_cast<int>(routeParams.size())};
        return params;
    }
    else {
        if (paramsSize > routeParams.size())
            return RouteParameterCountMismatch{paramsSize, static_cast<int>(routeParams.size())};

        return makeParams<ParamsTuple>(routeParams);
    }
}

template<typename TArgs, typename TRouteContext, typename TReturnType>
constexpr int getParamsCount()
{
    constexpr auto args = TArgs{};
    using LastArg = typename decltype(sfun::get<TArgs::size() - 1>(args))::type;
    if constexpr (std::is_same_v<TReturnType, void>) {
        if constexpr (
                args.size() > 2 && std::is_reference_v<LastArg> && std::is_same_v<std::decay_t<LastArg>, TRouteContext>)
            return args.size() - 3;
        else
            return args.size() - 2;
    }
    else {
        if constexpr (
                args.size() > 1 && std::is_reference_v<LastArg> && std::is_same_v<std::decay_t<LastArg>, TRouteContext>)
            return args.size() - 2;
        else
            return args.size() - 1;
    };
}

template<
        typename TResponseConverter,
        typename TRequestProcessor,
        typename TRequest,
        typename TResponse,
        typename TRouteContext>
void invokeRequestProcessor(
        TRequestProcessor& requestProcessor,
        const TRequest& request,
        TResponse& response,
        const std::vector<std::string>& routeParams,
        TRouteContext& routeContext,
        std::function<void(const TRequest&, TResponse&, const RouteParameterError&)> routeParamErrorHandler)
{
    checkRequestProcessorSignature<TRequestProcessor, TRequest, TResponse, TRouteContext>();

    constexpr auto args = sfun::callable_args<TRequestProcessor>{};
    using ReturnType = sfun::callable_return_type<TRequestProcessor>;
    constexpr auto paramsCount = getParamsCount<decltype(args), TRouteContext, ReturnType>();
    if constexpr (!paramsCount) {
        if constexpr (std::is_same_v<ReturnType, void>) {
            if constexpr (args.size() == 2)
                requestProcessor(request, response);
            else
                requestProcessor(request, response, routeContext);
        }
        else {
            if constexpr (args.size() == 1)
                TResponseConverter{}(response, requestProcessor(request));
            else
                TResponseConverter{}(response, requestProcessor(request, routeContext));
        }
    }
    else {
        auto paramsResult = readRouteParams<decltype(args), paramsCount>(routeParams);
        auto paramsResultVisitor = sfun::overloaded{
                [&](const RouteParameterError& error)
                {
                    if (routeParamErrorHandler)
                        routeParamErrorHandler(request, response, error);
                },
                [&](const auto& params)
                {
                    auto callProcess = [&](const auto&... param)
                    {
                        constexpr auto paramsCount = getParamsCount<decltype(args), TRouteContext, ReturnType>();
                        if constexpr (std::is_same_v<ReturnType, void>) {
                            if constexpr (args.size() - paramsCount == 2)
                                requestProcessor(param..., request, response);
                            else
                                requestProcessor(param..., request, response, routeContext);
                        }
                        else {
                            if constexpr (args.size() - paramsCount == 1)
                                TResponseConverter{}(response, requestProcessor(param..., request));
                            else
                                TResponseConverter{}(response, requestProcessor(param..., request, routeContext));
                        }
                    };
                    std::apply(callProcess, params);
                }};

        std::visit(paramsResultVisitor, paramsResult);
    }
}
} // namespace whaleroute::detail

#endif // WHALEROUTE_REQUESTPROCESSOR_H