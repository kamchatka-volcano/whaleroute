#include "common.h"
#include <whaleroute/requestrouter.h>
#include <gtest/gtest.h>

namespace without_processor {

class RouterWithoutProcessor : public ::testing::Test,
                               public whaleroute::RequestRouter<whaleroute::_, Request, RequestMethod, Response, std::string> {
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

    void setResponse(Response& response, const std::string& value) final
    {
        response.data = value;
    }

protected:
    std::string responseData_;
};

TEST_F(RouterWithoutProcessor, NoMatchRouteText)
{
    route("/", RequestMethod::GET).set("Hello world");
    route().set("Not found");
    processRequest(RequestMethod::GET, "/foo");

    checkResponse("Not found");
}

TEST_F(RouterWithoutProcessor, MultipleRoutes)
{
    route("/", RequestMethod::GET).set("Hello world");
    route("/page0", RequestMethod::GET).set("Default page");
    route(std::regex{R"(/page\d*)"}, RequestMethod::GET).set("Some page");
    route("/upload", RequestMethod::POST).set("OK");
    route(std::regex{R"(/files/.*\.xml)"}, RequestMethod::GET).process(
            [](const Request&, Response& response) {
                auto fileContent = std::string{"testXML"};
                response.data = fileContent;
            });
    route().set("404");

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