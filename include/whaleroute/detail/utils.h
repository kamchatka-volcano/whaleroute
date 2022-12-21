#ifndef WHALEROUTE_UTILS_H
#define WHALEROUTE_UTILS_H

#include "external/sfun/string_utils.h"
#include <whaleroute/stringconverter.h>
#include <whaleroute/types.h>
#include <algorithm>
#include <functional>
#include <regex>
#include <string>
#include <string_view>
#include <type_traits>

namespace whaleroute::detail {

namespace str = sfun::string_utils;

template <typename TDst, typename TSrc>
void concat(TDst& dst, const TSrc& src)
{
    std::copy(std::begin(src), std::end(src), std::inserter(dst, std::end(dst)));
}

template <typename T>
std::optional<T> convertFromString(const std::string& data)
{
    try {
        return config::StringConverter<T>::fromString(data);
    } catch (...) {
        return std::nullopt;
    }
}

inline std::string makePath(const std::string& path, TrailingSlashMode mode)
{
    if (mode == TrailingSlashMode::Optional && path != "/" && !path.empty() && path.back() == '/')
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
    const auto rxValParts = [&splitRes]
    {
        auto res = std::vector<std::string>{};
        std::transform(
                splitRes.begin(),
                splitRes.end(),
                std::back_inserter(res),
                [](const auto& rxValPartView)
                {
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
    if (mode == TrailingSlashMode::Optional) {
        if (str::endsWith(rxVal, "/")) {
            rxVal.pop_back();
            return std::regex{rxVal};
        }
        else if (str::endsWith(rxVal, "/)")) {
            rxVal[rxVal.size() - 1] = '?';
            rxVal += ')';
            return std::regex{rxVal};
        }
    }
    return std::regex{rxVal};
}

template <typename T>
struct type_wrapper {
    using type = T;
};
template <typename TWrapper>
using unwrap_type = typename TWrapper::type;

template <typename... T>
using type_list = std::tuple<type_wrapper<T>...>;

template <std::size_t index, typename TList>
using type_list_element = unwrap_type<std::remove_reference_t<decltype(std::get<index>(TList{}))>>;

template <typename TList>
constexpr auto type_list_size = std::tuple_size_v<TList>;

template <std::size_t index, typename TList>
constexpr auto typeListElement()
{
    auto list = TList{};
    return type_wrapper<unwrap_type<std::remove_reference_t<decltype(std::get<index>(list))>>>{};
}
template <typename TList, std::size_t... I>
constexpr auto makeTypeListElementsTuple(std::index_sequence<I...>) -> std::tuple<type_list_element<I, TList>...>;

template <typename TList, std::size_t Size = std::tuple_size_v<TList>>
using type_list_elements_tuple = decltype(makeTypeListElementsTuple<TList>(std::make_index_sequence<Size>()));

template <typename... T>
constexpr auto makeDecayTuple(std::tuple<T...> const&) -> std::tuple<std::decay_t<T>...>;

template <typename T>
using decay_tuple = decltype(makeDecayTuple(std::declval<T>()));

template <typename T>
struct callable_signature;

template <typename R, typename... Args>
struct callable_signature<std::function<R(Args...)>> {
    using return_type = R;
    using args = type_list<Args...>;
};

template <typename TCallable>
using callable_args = typename callable_signature<decltype(std::function{std::declval<TCallable>()})>::args;

template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

template <typename T>
inline constexpr auto dependent_false = false;

} // namespace whaleroute::detail

#endif // WHALEROUTE_UTILS_H