#ifndef WHALEROUTE_UTILS_H
#define WHALEROUTE_UTILS_H

#include "stringconverter.h"
#include "types.h"
#include "external/sfun/string_utils.h"
#include <algorithm>
#include <functional>
#include <regex>
#include <string>
#include <string_view>
#include <type_traits>

namespace whaleroute::detail {

template<typename, typename = void>
struct IsCompleteType : std::false_type {};

template<typename T>
struct IsCompleteType<T, std::void_t<decltype(sizeof(T))>> : std::true_type {};

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
    catch (...) {
        return std::nullopt;
    }
}

inline std::string makePath(const std::string& path, TrailingSlashMode mode)
{
    if (mode == TrailingSlashMode::Optional && path != "/" && !path.empty() && path.back() == '/')
        return {path.begin(), path.end() - 1};
    return path;
}

inline std::regex makeRegex(const rx& regExp, TrailingSlashMode)
{
    return std::regex{regExp.value};
}

inline std::tuple<bool, std::vector<std::string>> matchRegex(const std::string& path, const std::regex& regExp)
{
    auto matchList = std::smatch{};
    if (!std::regex_match(path, matchList, regExp))
        return {false, {}};

    auto routeParams = std::vector<std::string>{};
    for (auto i = 1u; i < matchList.size(); ++i)
        routeParams.push_back(matchList[i].str());
    return {true, std::move(routeParams)};
};

} // namespace whaleroute::detail

#endif // WHALEROUTE_UTILS_H