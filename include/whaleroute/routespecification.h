#pragma once

namespace whaleroute::config{
template<typename T, typename TRequest, typename TResponse>
struct RouteSpecification{
    bool operator()(const T& value, const TRequest& request, TResponse& response) const;
};

}
