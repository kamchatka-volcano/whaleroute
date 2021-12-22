#include "common.h"
#include <whaleroute/requestrouter.h>
#include <gtest/gtest.h>

class AccessTestingRouter : public ::testing::TestWithParam<std::tuple<whaleroute::RouteAccess, bool>>,
public whaleroute::RequestRouter<Request, Response, RequestType, whaleroute::_,ResponseValue> {
public:
AccessTestingRouter()
{
}

void SetUp() override
{
accessType_ = std::get<0>(GetParam());
state_.activated = false;
}

void processRequest(RequestType type, const std::string& path)
{
    auto response = Response{};
    response.init();
    process(Request{type, path}, response);
    responseData_ = response.state->data;
}

void expectNoMatch()
{
    EXPECT_EQ(responseData_, "NO_MATCH");
}

void checkState()
{
    switch (accessType_) {
        case whaleroute::RouteAccess::Open:
            EXPECT_TRUE(state_.activated);
            break;
        case whaleroute::RouteAccess::Authorized: {
            auto emptyMap = std::vector<std::pair<std::string, std::string>>{};
            auto request = Request{};
            if (isAccessAuthorized(request))
                EXPECT_TRUE(state_.activated);
            else
                EXPECT_FALSE(state_.activated);
            break;
        }
        case whaleroute::RouteAccess::Forbidden: {
            auto emptyMap = std::vector<std::pair<std::string, std::string>>{};
            auto request = Request{};
            if (isAccessAuthorized(request))
                EXPECT_FALSE(state_.activated);
            else
                EXPECT_TRUE(state_.activated);
            break;
        }
    }
}


void checkResponse(const std::string& expectedResponseData)
{
    checkState();
    switch (accessType_) {
        case whaleroute::RouteAccess::Open:
            EXPECT_EQ(responseData_, expectedResponseData);
            break;
        case whaleroute::RouteAccess::Authorized: {
            auto request = Request{};
            if (isAccessAuthorized(request))
                EXPECT_EQ(responseData_, expectedResponseData);
            else expectNoMatch();
            break;
        }
        case whaleroute::RouteAccess::Forbidden: {
            auto request = Request{};
            if (isAccessAuthorized(request))
                expectNoMatch();
            else
                EXPECT_EQ(responseData_, expectedResponseData);
            break;
        }
    }
}

protected:
bool isAccessAuthorized(const Request&) const override
{
return std::get<1>(GetParam());
}

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

void setResponseValue(Response& response, const ResponseValue& value) final
{
    state_.activated = true;
    response.state->data = value.data;
}

std::function<void(const Request&, Response& response)>
makeProcessor(std::function<void(const Request&, Response& response)> processor)
{
    return [this, processor](const Request& request, Response& response) {
        state_.activated = true;
        processor(request, response);
    };
}

protected:
whaleroute::RouteAccess accessType_;
std::string responseData_;
TestState state_;
};

TEST_P(AccessTestingRouter, SetResponseValue)
{
route("/", RequestType::GET, accessType_).set(ResponseValue{"Hello world"});

processRequest(RequestType::GET, "/");
checkResponse("Hello world");
}

TEST_P(AccessTestingRouter, SetResponseValueWithRegexp)
{
route(std::regex{R"(/page\d*)"}, RequestType::GET, accessType_).set(ResponseValue{"Hello world"});

processRequest(RequestType::GET, "/page123");
checkResponse("Hello world");
}

TEST_P(AccessTestingRouter, Process)
{
route("/", RequestType::GET, accessType_).process(makeProcessor([](const Request&, Response& response) {
    response.state->data = "Hello world";
}));

processRequest(RequestType::GET, "/");
checkResponse("Hello world");
}

TEST_P(AccessTestingRouter, ProcessAnyRequestType)
{
    route("/", whaleroute::_{}, accessType_).process(makeProcessor([](const Request&, Response& response) {
        response.state->data = "Hello world";
    }));

    processRequest(RequestType::POST, "/");
    checkResponse("Hello world");
}

TEST_P(AccessTestingRouter, ProcessWithRegexp)
{
route(std::regex{R"(/page\d*)"}, RequestType::GET, accessType_).process(
        makeProcessor([](const Request&, Response& response) {
    response.state->data = "Hello world";
}));

processRequest(RequestType::GET, "/page123");
checkResponse("Hello world");
}

INSTANTIATE_TEST_SUITE_P(WithAccessCheck, AccessTestingRouter, ::testing::Values(
        std::make_tuple(whaleroute::RouteAccess::Open, true),
        std::make_tuple(whaleroute::RouteAccess::Open, false),
        std::make_tuple(whaleroute::RouteAccess::Authorized, true),
        std::make_tuple(whaleroute::RouteAccess::Authorized, false),
        std::make_tuple(whaleroute::RouteAccess::Forbidden, true),
        std::make_tuple(whaleroute::RouteAccess::Forbidden, false)
));