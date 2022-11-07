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

template<typename TCallable>
using callable_args = typename get_signature<decltype(std::function{std::declval<TCallable>()})>::args;

template<typename T, std::size_t... I>
struct DecaySubTuple{
    using type = std::tuple<std::decay_t<std::tuple_element_t<I, T>>...>;
};

template<typename T, std::size_t... I>
constexpr auto makeDecaySubtuple(std::index_sequence<I...>)
{
    return DecaySubTuple<T, I...>{};
}

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;
}

#endif //WHALEROUTE_UTILS_H