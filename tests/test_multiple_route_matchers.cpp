#include "common.h"
#include <whaleroute/requestrouter.h>
#include <gtest/gtest.h>
#include <algorithm>

namespace whaleroute::config {
template<>
struct RouteMatcher<RequestType> {
    bool operator()(RequestType value, const Request& request) const
    {
        return value == request.type;
    }
};

template<>
struct RouteMatcher<std::string> {
    bool operator()(const std::string& value, const Request& request) const
    {
        return value == request.name;
    }
};
} // namespace whaleroute::config

namespace {

struct ResponseSender {
    void operator()(Response& response, const std::string& data)
    {
        response.send(data);
    }
};

class MultipleRouteMatchers : public ::testing::Test,
                              public whaleroute::RequestRouter<Request, Response, ResponseSender> {
public:
    void processRequest(const std::string& path, RequestType type, std::string name = {})
    {
        auto response = Response{};
        response.init();
        process(Request{type, path, std::move(name)}, response);
        responseData_ = response.state->data;
    }

    void checkResponse(const std::string& expectedResponseData)
    {
        EXPECT_EQ(responseData_, expectedResponseData);
    }

protected:
    std::string getRequestPath(const Request& request) final
    {
        return request.requestPath;
    }

    void processUnmatchedRequest(const Request&, Response& response) final
    {
        response.state->data = "NO_MATCH";
    }

    bool isRouteProcessingFinished(const Request&, Response& response) const final
    {
        return response.state->wasSent;
    }

protected:
    std::string responseData_;
};

struct Context {
    std::string state;
};

} //namespace

namespace whaleroute::config {

template<>
struct RouteMatcher<std::string, Context> {
    bool operator()(const std::string& value, const Request&, const Context& ctx) const
    {
        return value == ctx.state;
    }
};

} // namespace whaleroute::config

namespace {

class MultipleRouteMatchersWithContext : public ::testing::Test,
                                         public whaleroute::RequestRouter<Request, Response, ResponseSender, Context> {
public:
    void processRequest(const std::string& path, std::string name = {})
    {
        auto response = Response{};
        response.init();
        process(Request{RequestType::GET, path, std::move(name)}, response);
        responseData_ = response.state->data;
    }

    void checkResponse(const std::string& expectedResponseData)
    {
        EXPECT_EQ(responseData_, expectedResponseData);
    }

protected:
    std::string getRequestPath(const Request& request) final
    {
        return request.requestPath;
    }

    void processUnmatchedRequest(const Request&, Response& response) final
    {
        response.state->data = "NO_MATCH";
    }

    bool isRouteProcessingFinished(const Request&, Response& response) const final
    {
        return response.state->wasSent;
    }

protected:
    std::string responseData_;
};

} // namespace

using namespace std::string_literals;

TEST_F(MultipleRouteMatchers, Default)
{
    route("/", RequestType::GET).set("Hello world");
    route("/moon", RequestType::GET, "Sender"s).set("Hello moon from sender");
    route().set("404");

    processRequest("/", RequestType::GET);
    checkResponse("Hello world");

    processRequest("/moon", RequestType::GET);
    checkResponse("404");

    processRequest("/moon", RequestType::GET, "Sender");
    checkResponse("Hello moon from sender");

    processRequest("/foo", RequestType::GET);
    checkResponse("404");
}

TEST_F(MultipleRouteMatchersWithContext, Default)
{
    route("/").set("Hello world");
    route("/moon").set("Hello moon from sender");
    route().set("404");

    processRequest("/");
    checkResponse("Hello world");

    processRequest("/moon");
    checkResponse("Hello moon from sender");

    processRequest("/foo");
    checkResponse("404");
}

TEST_F(MultipleRouteMatchersWithContext, ContextMatching)
{
    route(whaleroute::rx{".+"})
            .process(
                    [](const Request&, Response&, Context& ctx)
                    {
                        ctx.state = "42";
                    });
    route("/", "9"s).set("Hello moon");
    route("/", "42"s).set("Hello world");
    route().set("404");

    processRequest("/moon");
    checkResponse("404");

    processRequest("/");
    checkResponse("Hello world");
}