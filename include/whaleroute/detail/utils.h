#pragma once
#include <whaleroute/types.h>
#include <whaleroute/stringconverter.h>
#include <algorithm>

namespace whaleroute::detail {

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

}