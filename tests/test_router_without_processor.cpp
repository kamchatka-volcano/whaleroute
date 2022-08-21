#include "common.h"
#include <whaleroute/requestrouter.h>
#include <gtest/gtest.h>

namespace whaleroute {
template<typename TRequest, typename TResponse>
struct RouteSpecificationPredicate<RequestType, TRequest, TResponse> {
    bool operator()(RequestType value, const TRequest& request, TResponse&) const
    {
        return value == request.type;
    }
};
}

namespace without_processor {

class RouterWithoutProcessor : public ::testing::Test,
                               public whaleroute::RequestRouter<Request, Response, whaleroute::_, std::string> {
public:
    void processRequest(RequestType type, const std::string& path)
    {
        auto response = Response{};
        response.init();
        process(Request{type, path, {}}, response);
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

    void setResponseValue(Response& response, const std::string& value) final
    {
        response.state->data = value;
    }

protected:
    std::string responseData_;
};

TEST_F(RouterWithoutProcessor, NoMatchRouteText)
{
    route("/", RequestType::GET).set("Hello world");
    route().set("Not found");
    processRequest(RequestType::GET, "/foo");

    checkResponse("Not found");
}

TEST_F(RouterWithoutProcessor, MultipleRoutes)
{
    route("/", RequestType::GET).set("Hello world");
    route("/page0", RequestType::GET).set("Default page");
    route("/any").process([](const Request&, Response& response){ response.state->data = "Any!";});
    route(std::regex{R"(/page\d*)"}, RequestType::GET).set("Some page");
    route("/upload", RequestType::POST).set("OK");
    route(std::regex{R"(/files/.*\.xml)"}, RequestType::GET).process(
            [](const Request&, Response& response) {
                auto fileContent = std::string{"testXML"};
                response.state->data = fileContent;
            });
    route().set("404");

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