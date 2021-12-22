#pragma once
#include <algorithm>

namespace whaleroute::detail {

template<typename TDst, typename TSrc>
void concat(TDst& dst, const TSrc& src)
{
    std::copy(std::begin(src), std::end(src), std::inserter(dst, std::end(dst)));
}

}