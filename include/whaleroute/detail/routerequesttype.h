#pragma once
#include "emptytype.h"

namespace whaleroute::detail {
template<typename TRequestType>
class RouteRequestType {
public:
    template<typename T = TRequestType, typename = std::enable_if_t<!std::is_same_v<T, _>>>
    RouteRequestType(TRequestType type)
            : requestType_(type), isAny_(false)
    {}

    RouteRequestType(_)
            : isAny_(true)
    {}

    operator TRequestType() const
    {
        return requestType_;
    }

    bool isAny() const
    {
        return isAny_;
    }


private:
    TRequestType requestType_;
    bool isAny_;
};

}