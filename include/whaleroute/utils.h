#ifndef WHALEROUTE_UTILS_H
#define WHALEROUTE_UTILS_H

#include "stringconverter.h"
#include "types.h"
#include "external/sfun/string_utils.h"
#include <algorithm>
#include <cstdint>
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
        if constexpr(sfun::is_optional_v<T>) {
            if (data.empty())
                return T{};

            return config::StringConverter<sfun::remove_optional_t<T>>::fromString(data);
        }
        else
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

inline std::regex makeRegex(std::string_view regExp, TrailingSlashMode)
{
    return std::regex{std::string{regExp}};
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

constexpr std::uint32_t fnv1a(std::string_view data)
{
    const auto fnvPrime = std::uint32_t{0x01000193};
    auto hash = std::uint32_t{0x811c9dc5};

    for (auto ch : data) {
        hash ^= static_cast<uint32_t>(ch);
        hash *= fnvPrime;
    }
    return hash;
}

inline std::string prepareRegexString(std::string_view input)
{
    static const auto specialChars = std::string{R"(\.^$+()[]{}|?*)"};
    auto result = std::string{};
    result.reserve(input.size());
    for (auto ch : input) {
        if (specialChars.find(ch) != std::string::npos)
            result.push_back('\\');
        result.push_back(ch);
    }
    return result;
}

} // namespace whaleroute::detail

#endif // WHALEROUTE_UTILS_H