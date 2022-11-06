#ifndef WHALEROUTE_UTILS_H
#define WHALEROUTE_UTILS_H

#include <whaleroute/types.h>
#include <whaleroute/stringconverter.h>
#include "external/sfun/string_utils.h"
#include <algorithm>
#include <string_view>
#include <string>
#include <regex>
#include <type_traits>
#include <functional>

namespace whaleroute::detail {

namespace str = sfun::string_utils;

template<typename TDst, typename TSrc>
void concat(TDst& dst, const TSrc& src)
{
    std::copy(std::begin(src), std::end(src), std::inserter(dst, std::end(dst)));
}

template<typename T>
std::optional<T> convertFromString(const std::string& data)
{
    try {
        return config::StringConverter<T>::fromString(data);
    }
    catch(...){
        return {};
    }
}

inline std::string makePath(const std::string& path, TrailingSlashMode mode)
{
    if (mode == TrailingSlashMode::Optional &&
        path != "/" &&
        !path.empty() &&
        path.back() == '/')
        return {path.begin(), path.end() - 1};
    return path;
}

inline std::string regexValue(const rx& regExp, RegexMode regexMode)
{
    if (regexMode != RegexMode::TildaEscape)
        return regExp.value;

    const auto rxVal = regExp.value;
    const auto startsWithTilda = str::startsWith(rxVal, "~~");
    const auto endsWithTilda = str::endsWith(rxVal, "~~");
    const auto splitRes = str::split(rxVal, "~~", false);
    const auto rxValParts = [&splitRes]{
        auto res = std::vector<std::string>{};
        std::transform(splitRes.begin(), splitRes.end(), std::back_inserter(res), [](const auto& rxValPartView) {
            return str::replace(std::string{rxValPartView}, "~", "\\");
        });
        return res;
    }();
    return (startsWithTilda ? "~" : "") + str::join(rxValParts, "~") + (endsWithTilda ? "~" : "");
}


inline std::regex makeRegex(const rx& regExp, RegexMode regexMode, TrailingSlashMode mode)
{
    if (regexMode == RegexMode::Regular && mode == TrailingSlashMode::Strict)
        return std::regex{regExp.value};

    auto rxVal = regexValue(regExp, regexMode);
    if (mode == TrailingSlashMode::Optional){
        if (str::endsWith(rxVal, "/")){
            rxVal.pop_back();
            return std::regex{rxVal};
        }
        else if (str::endsWith(rxVal, "/)")){
            rxVal[rxVal.size() - 1] = '?';
            rxVal += ')';
            return std::regex{rxVal};
        }
    }
    return std::regex{rxVal};
}

template <typename T>
struct get_signature;

template <typename R, typename... Args>
struct get_signature<std::function<R(Args...)>> {
    using return_type = R;
    using args = std::tuple<Args...>;
};
//
//template<typename TCallable>
//using callable_return_type = typename get_signature<decltype(std::function{std::declval<TCallable>()})>::return_type;

template<typename TCallable>
using callable_args = typename get_signature<decltype(std::function{std::declval<TCallable>()})>::args;

template<typename TRequestProcessor, typename TRequest, typename TResponse>
constexpr void checkRequestProcessorSignature()
{
    using args = callable_args<TRequestProcessor>;
    constexpr auto argsSize = std::tuple_size_v<args>;
    static_assert(argsSize >= 2);
    static_assert(std::is_same_v<const TRequest&, std::tuple_element_t<argsSize - 2, args>>);
    static_assert(std::is_same_v<TResponse&, std::tuple_element_t<argsSize - 1, args>>);
}


template <typename... TParam, typename TRequest, typename TResponse>
constexpr std::tuple<TParam...> getRouteParametersFromArgs(const std::tuple<TParam..., TRequest, TResponse>& argTuple)
{
    return std::apply([](auto... param, auto, auto) {
        return std::make_tuple(param...);
    }, argTuple);
}

//template <typename... T, std::size_t... I>
//auto subtuple_(const std::tuple<T...>& t, std::index_sequence<I...>) {
//  return std::make_tuple(std::get<I>(t)...);
//}
//
//template <int Trim, typename... T>
//auto subtuple(const std::tuple<T...>& t) {
//  return subtuple_(t, std::make_index_sequence<sizeof...(T) - Trim>());
//}

template<typename T, std::size_t... I>
struct DecaySubTuple{
    using type = std::tuple<std::decay_t<std::tuple_element_t<I, T>>...>;
};

template<typename T, std::size_t... I>
constexpr auto makeDecaySubtuple(std::index_sequence<I...>)
{
    return DecaySubTuple<T, I...>{};
}

template<typename TArgsTuple>
using requestProcessorRouteParamsType = typename decltype(makeDecaySubtuple<TArgsTuple>(std::make_index_sequence<std::tuple_size_v<TArgsTuple> - 2>()))::type;

template<typename TRequestProcessor, typename TRequest, typename TResponse>
void invokeRequestProcessor(TRequestProcessor& requestProcessor, const TRequest& request, TResponse& response, const std::vector<std::string>& routeParams)
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
        if (routeParams.size() < paramsSize)
            throw std::runtime_error{"Route parameter count mismatch"};
        using params_t = requestProcessorRouteParamsType<args_t>;
        auto params = params_t{};
        auto i = 0u;
        auto error = std::optional<RouteParameterError>{};
        auto readParam = [&](auto& val) {
            if (error)
                return;
            //        if (i >= routeParams.size()){
            //            error = RouteParameterCountMismatch{std::tuple_size_v<decltype(param)>, static_cast<int>(routeParams.size())};
            //            return;
            //        }
            const auto& param = routeParams.at(i++);
            auto paramValue = detail::convertFromString<std::decay_t<decltype(val)>>(param);
            if (paramValue)
                val = *paramValue;
            else
                throw std::runtime_error{"Route parameter read error"};
            //error = RouteParameterReadError{static_cast<int>(i), param};
        };

        std::apply([readParam](auto& ... paramValues) {
            ((readParam(paramValues)), ...);
        }, params);

        //    if (error) {
        //        onRouteParametersError(request, response, *error);
        //        return;
        //    }

        auto callProcess = [&](const auto& ... param) {
            requestProcessor(param..., request, response);
        };
        std::apply(callProcess, params);
    }
}

}

#endif //WHALEROUTE_UTILS_H