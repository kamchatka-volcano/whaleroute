#include "common.h"
#include <whaleroute/types.h>
#include <whaleroute/requestrouter.h>
#include <gtest/gtest.h>

namespace whaleroute::traits{
template<typename TRequest, typename TResponse>
struct RouteSpecification<RequestType, TRequest, TResponse> {
    bool operator()(RequestType value, const TRequest& request, TResponse&) const
    {
        return value == request.type;
    }
};

}

class Router : public ::testing::Test,
               public whaleroute::RequestRouter<Request, Response, RequestProcessor, std::string> {
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

    void processUnmatchedRequest(const Request&, Response& response) final
    {
        response.state->data = "NO_MATCH";
    }
    void setResponseValue(Response& response, const std::string& value) final
    {
        response.state->data = value;
    }

    void callRequestProcessor(RequestProcessor& processor, const Request& request, Response& response) final
    {
        processor.process(request, response);
    }

protected:
    std::string responseData_;
};

class StatelessRouteProcessor : public RequestProcessor
{
    void process(const Request& request, Response& response) override
    {
        if (!request.name.empty())
            response.state->data = "Hello " + request.name;
        else
            response.state->data = "/name-not-found";
    }
};

TEST_F(Router, StatelessRouteProcessor){
    route("/greet", RequestType::GET).process<StatelessRouteProcessor>();
    auto processor = StatelessRouteProcessor{};
    route("/greet2", RequestType::GET).process(processor);
    route("/", RequestType::GET).set("Hello world");
    route("/any").process([](const Request&, Response& response){ response.state->data = "Any!";});
    route(std::regex{"/greet/.*"}, RequestType::GET).process<StatelessRouteProcessor>();
    route(std::regex{"/greet2/.*"}).process<StatelessRouteProcessor>();
    route().set("/404");

    processRequest(RequestType::GET, "/");
    checkResponse("Hello world");

    processRequest(RequestType::GET, "/greet");
    checkResponse("/name-not-found");

    processRequest(RequestType::GET, "/greet2");
    checkResponse("/name-not-found");

    processRequest(RequestType::GET, "/greet/foo", "foo");
    checkResponse("Hello foo");

    processRequest(RequestType::POST, "/greet2/foo", "foo");
    checkResponse("Hello foo");

    processRequest(RequestType::POST, "/foo");
    checkResponse("/404");

    processRequest(RequestType::POST, "/any");
    checkResponse("Any!");

    processRequest(RequestType::POST, "/any/");
    checkResponse("Any!");

    setTrailingSlashMode(whaleroute::TrailingSlashMode::Strict);
    processRequest(RequestType::POST, "/any/");
    checkResponse("/404");
}

struct NameState{
    std::string name;
};

class StatefulRouteProcessor : public RequestProcessor
{
public:
    StatefulRouteProcessor(NameState& state)
        : state_(state)
    {}

    void process(const Request& request, Response& response) override
    {
        if (request.requestPath == "/config" && !request.name.empty())
            state_.name = request.name;

        if (request.requestPath == "/" || request.requestPath == "/test"){
            if (state_.name.empty())
                response.state->data  = "OK";
            else
                response.state->data = "Hello " + state_.name;
        }
        else
            response.state->data =  "/";
    }

    NameState& state_;
};

class NameSetterRouteProcessor : public RequestProcessor
{
public:
    NameSetterRouteProcessor(NameState& state)
        : state_(state)
    {}

    void process(const Request&, Response&) override
    {
        state_.name = "TEST";
    }

private:
    NameState& state_;
};

TEST_F(Router, StatefulRouteProcessor){
    NameState state;
    auto processor = StatefulRouteProcessor{state};
    route(std::regex{R"(.*)"}, RequestType::GET).process(processor);

    processRequest(RequestType::GET, "/");
    checkResponse("OK");

    processRequest(RequestType::GET, "/config", "foo");
    checkResponse("/");

    processRequest(RequestType::GET, "/");
    checkResponse("Hello foo");
}

TEST_F(Router, ChainedRouteProcessor){
    NameState state;
    auto processor = StatefulRouteProcessor{state};
    auto nameProcessor = NameSetterRouteProcessor{state};
    route("/test", RequestType::GET).process(nameProcessor).process(processor);

    processRequest(RequestType::GET, "/test");
    checkResponse("Hello TEST");
}

class CounterRouteProcessor : public RequestProcessor
{
public:
    CounterRouteProcessor(int& state)
        : state_(state)
    {}

    void process(const Request&, Response& response) override
    {
        state_ = ++counter;
        response.state->data = "TEST";
    }

private:
    int counter = 0;
    int& state_;
};

TEST_F(Router, TemplateProcessorInMultipleRoutes){
    int state = 0;
    route("/test", RequestType::GET).process<CounterRouteProcessor>(state);
    route("/test2", RequestType::GET).process<CounterRouteProcessor>(state);

    processRequest(RequestType::GET, "/test");
    processRequest(RequestType::GET, "/test2");
    ASSERT_EQ(state, 1); //Which means that routes contain different processor objects
}

