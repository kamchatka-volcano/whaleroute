#ifndef WHALEROUTE_ROUTESPECIFICATION_H
#define WHALEROUTE_ROUTESPECIFICATION_H

namespace whaleroute::config {
template <typename T, typename TRequest, typename TResponse>
struct RouteSpecification {
    bool operator()(const T& value, const TRequest& request, TResponse& response) const;
};

} // namespace whaleroute::config

#endif // WHALEROUTE_ROUTESPECIFICATION_H