#include "common.h"
#include <whaleroute/requestrouter.h>
#include <gtest/gtest.h>

namespace without_processor_and_response_value {

class RouterWithoutProcessorAndResponseValue : public ::testing::Test,
                                               public whaleroute::RequestRouter<Request, Response, RequestType> {
public:
    void processRequest(RequestType type, const std::string& path)
    {
        auto response = Response{};
        response.init();
        process(Request{type, path}, response);
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

protected:
    std::string responseData_;
};

TEST_F(RouterWithoutProcessorAndResponseValue, NoMatchRouteText)
{
    route("/", RequestType::GET).process([](auto&, auto& response){ response.state->data = "Hello world"; });
    route().process([](auto&, auto& response){ response.state->data = "Not found";});
    processRequest(RequestType::GET, "/foo");

    checkResponse("Not found");
}

TEST_F(RouterWithoutProcessorAndResponseValue, MultipleRoutes)
{
    route("/", RequestType::GET).process([](auto&, auto& response){ response.state->data = "Hello world";});
    route("/page0", RequestType::GET).process([](auto&, auto& response){ response.state->data = "Default page";});
    route(std::regex{R"(/page\d*)"}, RequestType::GET).process([](auto&, auto& response){ response.state->data = "Some page";});
    route("/upload", RequestType::POST).process([](auto&, auto& response){ response.state->data = "OK";});
    route("/any", whaleroute::_{}).process([](const Request& request, Response& response){ response.state->data = "Any!";});
    route(std::regex{R"(/files/.*\.xml)"}, RequestType::GET).process(
            [](const Request&, Response& response) {
                auto fileContent = std::string{"testXML"};
                response.state->data = fileContent;
            });
    route().process([](auto&, auto& response){ response.state->data = "404";});

    processRequest(RequestType::GET, "/");
    checkResponse("Hello world");

    processRequest(RequestType::POST, "/upload");
    checkResponse("OK");

    processRequest(RequestType::GET, "/page123");
    checkResponse("Some page");

    processRequest(RequestType::GET, "/page0");
    checkResponse("Default page");

    processRequest(RequestType::GET, "/files/test.xml");
    checkResponse("testXML");

    processRequest(RequestType::GET, "/files/test.xml1");
    checkResponse("404");

    processRequest(RequestType::POST, "/foo");
    checkResponse("404");

    processRequest(RequestType::POST, "/any");
    checkResponse("Any!");
}

}