#ifndef WHALEROUTE_ROUTEPARAMPARSER_H
#define WHALEROUTE_ROUTEPARAMPARSER_H
#include "routeparam.h"
#include "external/sfun/type_list.h"
#include "external/sfun/utility.h"
#include <array>
#include <cstdint>
#include <utility>
#include <vector>

namespace whaleroute::detail{

inline std::vector<std::string> readPathParams(std::string_view str)
{
    auto params = std::vector<std::string>{};
    auto i = 0;
    int openBraceCount = 0;
    int paramPos = 0;
    while (i < sfun::ssize(str)) {
        if (str[i] == '{') {
            openBraceCount++;
            paramPos = i + 1;
        }
        if (str[i] == '}') {
            openBraceCount--;
            params.emplace_back(std::next(str.begin(), paramPos), std::next(str.begin(), i));
        }
        if (openBraceCount > 1)
            return {};
        i++;
    }
    if (openBraceCount != 0)
        return {};

    return params;
}

#if (defined(_MSVC_LANG) && _MSVC_LANG >= 202002L) || (!defined(_MSVC_LANG) && __cplusplus >= 202002L)

template<auto idArray, std::size_t... Is>
constexpr auto paramIdListToParamTraitTupleImpl(std::index_sequence<Is...>) {
    return std::tuple<sfun::type_identity<config::RouteParam<idArray[Is].id>>...>{};
}

template<auto idArray>
constexpr auto paramIdListToParamTraitTuple() {
    return paramIdListToParamTraitTupleImpl<idArray>(std::make_index_sequence<idArray.size()>{});
}

template<auto idArray, std::size_t... Is>
constexpr auto paramIdListToParamTypeTupleImpl(std::index_sequence<Is...>) {
    return std::tuple<sfun::type_identity<std::conditional_t<
            idArray[Is].optional,
            std::optional<RouteParamType<idArray[Is].id>>,
            RouteParamType<idArray[Is].id>>>...>{};
}

template<auto idArray>
constexpr auto paramIdListToParamTypeTuple() {
    return paramIdListToParamTypeTupleImpl<idArray>(std::make_index_sequence<idArray.size()>{});
}

template<auto idArray, std::size_t... Is, typename TFunc>
constexpr auto forEachIdImpl(std::index_sequence<Is...>, TFunc&& func)
{
    (func(idArray[Is]), ...);
}

template<auto idArray, typename TFunc>
constexpr auto forEachId(TFunc&& func)
{
    forEachIdImpl<idArray>(std::make_index_sequence<idArray.size()>{}, func);
}

template<typename T, std::size_t capacity = 1024>
struct static_vector {
    using type = T;

    constexpr auto operator[](std::size_t i) -> T& {
        return raw[i];
    }

    constexpr auto operator[](std::size_t i) const -> T const& {
        return raw[i];
    }

    constexpr void emplace_back(auto&& expr) {
        raw[count] = static_cast<decltype(expr)>(expr);
        count += 1;
    }

    constexpr auto size() const -> std::size_t {
        return count;
    }

    constexpr auto data() const -> const T* {
        return raw.data();
    }

    std::array<T, capacity> raw{};
    std::size_t count = 0;
};

template<auto staticVector>
consteval auto makeArrayFromStaticVector()
{
    auto result = std::array<typename decltype(staticVector)::type, staticVector.size()>{};
    for (auto i = 0; i < sfun::ssize(result); ++i)
        result[i] = staticVector.raw[i];
    return result;
}

constexpr int countRegexCaptureGroups(std::string_view str)
{
    auto i = 0;
    int openBraceCount = 0;
    int groupCount = 0;
    while (i < sfun::ssize(str)) {
        if (str[i] == '(') {
            if (i == 0 || str[i - 1] != '\\')
                openBraceCount++;
            if (i >= sfun::ssize(str) - 1 || str[i + 1] != '?')
                groupCount++;
        }
        if (str[i] == ')') {
            if (i == 0 || str[i - 1] != '\\')
                openBraceCount--;
        }
        if (openBraceCount < 0)
            return -1;
        i++;
    }
    if (openBraceCount != 0)
        return -1;


    return groupCount;
};

struct ParamId {
    uint32_t id;
    bool optional;
};

constexpr static_vector<ParamId> readPathParamIds(std::string_view str)
{
    auto params = static_vector<ParamId>{};
    auto i = 0;
    int openBraceCount = 0;
    int paramPos = 0;
    while (i < sfun::ssize(str)) {
        if (str[i] == '{') {
            openBraceCount++;
            paramPos = i + 1;
        }
        if (str[i] == '}') {
            openBraceCount--;
            params.emplace_back(
                    ParamId{.id = fnv1a(std::string_view{std::next(str.begin(), paramPos), std::next(str.begin(), i)}),
                            .optional = (i < sfun::ssize(str) - 1 && str[i + 1] == '?')});
        }
        if (openBraceCount > 1)
            return {};
        i++;
    }
    if (openBraceCount != 0)
        return {};

    return params;
}

template<detail::StaticString path>
constexpr auto readPathParamIds()
{
    return makeArrayFromStaticVector<readPathParamIds(path.str())>();
}

template<detail::StaticString path>
constexpr auto countRegexCaptureGroups()
{
    return countRegexCaptureGroups(path.str());
}

template<auto paramIdArray>
constexpr auto paramIdsToParamTraitList()
{
    return sfun::type_list{paramIdListToParamTraitTuple<paramIdArray>()};
}

template<auto paramIdArray>
constexpr auto paramIdsToParamTypeList()
{
    return sfun::type_list{paramIdListToParamTypeTuple<paramIdArray>()};
}

#endif

}

#endif //WHALEROUTE_ROUTEPARAMPARSER_H
