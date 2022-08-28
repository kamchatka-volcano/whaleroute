#include "common.h"
#include <whaleroute/requestrouter.h>
#include <gtest/gtest.h>
#include <algorithm>

namespace whaleroute::traits {
template<typename TRequest, typename TResponse>
struct RouteSpecification<RequestType, TRequest, TResponse> {
    bool operator()(RequestType value, const TRequest& request, TResponse&) const
    {
        return value == request.type;
    }
};
}

class TestString{
public:
    enum class CaseMode{Original, UpperCase};
    TestString(std::string value, CaseMode caseMode = CaseMode::Original)
    : value_{std::move(value)}
    {
        if (caseMode == CaseMode::UpperCase)
            std::transform(value_.cbegin(), value_.cend(), value_.begin(), [](unsigned char ch){
                return static_cast<char>(std::toupper(ch));
            });
    }
    TestString(const char* value, CaseMode caseMode = CaseMode::Original)
    : TestString{std::string{value}, caseMode}
    {

    }
    std::string value() const
    {
        return value_;
    }

private:
    std::string value_;
};

namespace without_processor {

class RouterWithoutProcessor : public ::testing::Test,
                               public whaleroute::RequestRouter<Request, Response, whaleroute::_, TestString> {
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

    void setResponseValue(Response& response, const TestString& value) final
    {
        response.state->data = value.value();
    }

protected:
    std::string responseData_;
};

TEST_F(RouterWithoutProcessor, NoMatchRouteText)
{
    route("/", RequestType::GET).set("Hello world");
    route().set("Not found", TestString::CaseMode::UpperCase);
    processRequest(RequestType::GET, "/foo");

    checkResponse("NOT FOUND");
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