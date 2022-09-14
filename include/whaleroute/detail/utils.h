#pragma once
#include <whaleroute/types.h>
#include <whaleroute/stringconverter.h>
#include "external/sfun/string_utils.h"
#include <algorithm>
#include <string_view>
#include <string>
#include <regex>

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


}