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

inline std::string regexValue(const rx& regExp, RegexMode regexMode)
{
    if (regexMode != RegexMode::TildaEscape)
        return regExp.value;

    const auto rxVal = regExp.value;
    const auto startsWithTilda = sfun::startsWith(rxVal, "~~");
    const auto endsWithTilda = sfun::endsWith(rxVal, "~~");
    const auto splitRes = sfun::split(rxVal, "~~", false);
    const auto rxValParts = [&splitRes]
    {
        auto res = std::vector<std::string>{};
        std::transform(
                splitRes.begin(),
                splitRes.end(),
                std::back_inserter(res),
                [](const auto& rxValPartView)
                {
                    return sfun::replace(std::string{rxValPartView}, "~", "\\");
                });
        return res;
    }();
    return (startsWithTilda ? "~" : "") + sfun::join(rxValParts, "~") + (endsWithTilda ? "~" : "");
}

inline std::regex makeRegex(const rx& regExp, RegexMode regexMode, TrailingSlashMode mode)
{
    if (regexMode == RegexMode::Regular && mode == TrailingSlashMode::Strict)
        return std::regex{regExp.value};

    auto rxVal = regexValue(regExp, regexMode);
    if (mode == TrailingSlashMode::Optional) {
        if (sfun::endsWith(rxVal, "/")) {
            rxVal.pop_back();
            return std::regex{rxVal};
        }
        else if (sfun::endsWith(rxVal, "/)")) {
            rxVal[rxVal.size() - 1] = '?';
            rxVal += ')';
            return std::regex{rxVal};
        }
    }
    return std::regex{rxVal};
}

} // namespace whaleroute::detail

#endif // WHALEROUTE_UTILS_H