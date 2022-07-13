#include "common.h"
#include <whaleroute/requestrouter.h>
#include <gtest/gtest.h>

namespace {

class MultiRouter : public ::testing::Test,
                    public whaleroute::RequestRouter<Request, Response, RequestType, RequestProcessor, std::string> {
public:
    void processRequest(RequestType type, const std::string& path, const std::string& name = {})
    {
        auto response = Response{};
        response.init();
        process(Request{type, path, name}, response);
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
    RequestType getRequestType(const Request& request) final
    {
        return request.type;
    }

    void processUnmatchedRequest(const Request&, Response& response) final
    {
        response.state->data = "NO_MATCH";
    }
    void setResponseValue(Response& response, const std::string& value) final
    {
        response.state->data = value;
        response.state->wasSent = true;
    }

    void callRequestProcessor(RequestProcessor& processor, const Request& request, Response& response) final
    {
        processor.process(request, response);
    }

    bool isRouteProcessingFinished(const Request&, Response& response) const final
    {
        return response.state->wasSent;
    }

protected:
    std::string responseData_;
};

TEST_F(MultiRouter, MultiRouteTest)
{
    route(std::regex{"/greet/.*"}, RequestType::GET).process([](const auto&, auto& response) {
        response.state->data = "Hello";
    });
    route("/greet/world", RequestType::GET).process([](const auto&, auto& response) {
        response.state->data += " world";
        response.state->wasSent = true;
    });
    route("/", RequestType::GET).set("Hello Bill");
    route().set("/404");

    processRequest(RequestType::GET, "/greet/world");
    checkResponse("Hello world");
    processRequest(RequestType::GET, "/greet/moon");
    checkResponse("/404");
    processRequest(RequestType::GET, "/");
    checkResponse("Hello Bill");
}

TEST_F(MultiRouter, MultiRouteTestWithChaining)
{
    auto testState = std::string{};
    route(std::regex{"/greet/.*"}, RequestType::GET).process([](const auto&, auto& response) {
        response.state->data = "Hello";
    });
    route("/greet/world", RequestType::GET)
    .process([](const auto&, auto& response) {
        response.state->data += " world";
        response.state->wasSent = true;
    }).process([&testState](const auto&, auto&) {
        testState = "TEST";
    });
    route("/", RequestType::GET).set("Hello Bill");
    route().set("/404");

    processRequest(RequestType::GET, "/greet/world");
    checkResponse("Hello world");
    EXPECT_EQ(testState, "TEST"); //Ensures that chained processor was called after response had been sent
    processRequest(RequestType::GET, "/greet/moon");
    checkResponse("/404");
    processRequest(RequestType::GET, "/");
    checkResponse("Hello Bill");
}

}
