#include "common.h"
#include <whaleroute/requestrouter.h>
#include <gtest/gtest.h>

namespace without_processor_and_response_value {

class RouterWithoutProcessorAndRequestTypeAndResponseValue : public ::testing::Test,
                                                             public whaleroute::RequestRouter<Request, Response> {
public:
    void processRequest(RequestType type, const std::string& path)
    {
        auto response = Response{};
        process(Request{type, path}, response);
        responseData_ = response.data;
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
        response.data = "NO_MATCH";
    }

protected:
    std::string responseData_;
};

TEST_F(RouterWithoutProcessorAndRequestTypeAndResponseValue, NoMatchRouteText)
{
    route("/").process([](auto&, auto& response){ response.data = "Hello world"; });
    route().process([](auto&, auto& response){ response.data = "Not found";});
    processRequest(RequestType::GET, "/foo");

    checkResponse("Not found");
}

TEST_F(RouterWithoutProcessorAndRequestTypeAndResponseValue, MultipleRoutes)
{
    route("/").process([](auto&, auto& response){ response.data = "Hello world";});
    route("/page0").process([](auto&, auto& response){ response.data = "Default page";});
    route(std::regex{R"(/page\d*)"}).process([](auto&, auto& response){ response.data = "Some page";});
    route("/upload").process([](auto&, auto& response){ response.data = "OK";});
    route(std::regex{R"(/files/.*\.xml)"}).process(
            [](const Request&, Response& response) {
                auto fileContent = std::string{"testXML"};
                response.data = fileContent;
            });
    route().process([](auto&, auto& response){ response.data = "404";});

    processRequest(RequestType::POST, "/");
    checkResponse("Hello world");

    processRequest(RequestType::POST, "/upload");
    checkResponse("OK");

    processRequest(RequestType::POST, "/page123");
    checkResponse("Some page");

    processRequest(RequestType::POST, "/page0");
    checkResponse("Default page");

    processRequest(RequestType::POST, "/files/test.xml");
    checkResponse("testXML");

    processRequest(RequestType::POST, "/files/test.xml1");
    checkResponse("404");

    processRequest(RequestType::POST, "/foo");
    checkResponse("404");
}

}