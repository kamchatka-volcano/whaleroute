#pragma once
#include <whaleroute/types.h>
#include <memory>
#include <string>
#include <variant>
#include <vector>

struct TestState {
    bool activated = false;
};

enum class RequestType {
    GET,
    POST
};
struct Request {
    RequestType type;
    std::string requestPath;
    std::string name;
};
struct Response {
    void init()
    {
        state = std::make_shared<State>();
    }
    void send(const std::string& data)
    {
        state->data = data;
        state->wasSent = true;
    }
    struct State {
        std::string data;
        bool wasSent = false;
        std::string context;
    };
    std::shared_ptr<State> state;
    std::vector<std::string> routeParams;
};
struct ResponseValue {
    std::string data;
};

inline std::string getRouteParamErrorInfo(const whaleroute::RouteParameterError& error)
{
    if (std::holds_alternative<whaleroute::RouteParameterCountMismatch>(error)) {
        const auto& errorInfo = std::get<whaleroute::RouteParameterCountMismatch>(error);
        return "ROUTE_PARAM_ERROR: PARAM COUNT MISMATCH,"
               " EXPECTED:"
                + std::to_string(errorInfo.expectedNumber) + " ACTUAL:" + std::to_string(errorInfo.actualNumber);
    }
    else if (std::holds_alternative<whaleroute::RouteParameterReadError>(error)) {
        const auto& errorInfo = std::get<whaleroute::RouteParameterReadError>(error);
        return "ROUTE_PARAM_ERROR: COULDN'T READ ROUTE PARAM,"
               " INDEX:"
                + std::to_string(errorInfo.index) + " VALUE:" + errorInfo.value;
    }
    return {};
}
