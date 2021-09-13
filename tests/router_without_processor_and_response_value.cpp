#include "common.h"
#include <whaleroute/requestrouter.h>
#include <gtest/gtest.h>

namespace without_processor_and_response_value {

class RouterWithoutProcessorAndResponseValue : public ::testing::Test,
                                               public whaleroute::RequestRouter<whaleroute::_, Request, RequestMethod, Response> {
public:
    void processRequest(RequestMethod method, const std::string& path)
    {
        auto response = Response{};
        process(Request{method, path}, response);
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

    RequestMethod getRequestMethod(const Request& request) final
    {
        return request.method;
    }

    void processUnmatchedRequest(const Request&, Response& response) final
    {
        response.data = "NO_MATCH";
    }

protected:
    std::string responseData_;
};

TEST_F(RouterWithoutProcessorAndResponseValue, NoMatchRouteText)
{
    route("/", RequestMethod::GET).process([](auto&, auto& response){ response.data = "Hello world"; });
    route().process([](auto&, auto& response){ response.data = "Not found";});
    processRequest(RequestMethod::GET, "/foo");

    checkResponse("Not found");
}

TEST_F(RouterWithoutProcessorAndResponseValue, MultipleRoutes)
{
    route("/", RequestMethod::GET).process([](auto&, auto& response){ response.data = "Hello world";});
    route("/page0", RequestMethod::GET).process([](auto&, auto& response){ response.data = "Default page";});
    route(std::regex{R"(/page\d*)"}, RequestMethod::GET).process([](auto&, auto& response){ response.data = "Some page";});
    route("/upload", RequestMethod::POST).process([](auto&, auto& response){ response.data = "OK";});
    route(std::regex{R"(/files/.*\.xml)"}, RequestMethod::GET).process(
            [](const Request&, Response& response) {
                auto fileContent = std::string{"testXML"};
                response.data = fileContent;
            });
    route().process([](auto&, auto& response){ response.data = "404";});

    processRequest(RequestMethod::GET, "/");
    checkResponse("Hello world");

    processRequest(RequestMethod::POST, "/upload");
    checkResponse("OK");

    processRequest(RequestMethod::GET, "/page123");
    checkResponse("Some page");

    processRequest(RequestMethod::GET, "/page0");
    checkResponse("Default page");

    processRequest(RequestMethod::GET, "/files/test.xml");
    checkResponse("testXML");

    processRequest(RequestMethod::GET, "/files/test.xml1");
    checkResponse("404");

    processRequest(RequestMethod::POST, "/foo");
    checkResponse("404");
}

}