#include "common.h"
#include <whaleroute/requestrouter.h>
#include <gtest/gtest.h>

namespace without_request_type {

class RouterWithoutRequestType : public ::testing::Test,
                                 public whaleroute::RequestRouter<Request, Response, RequestProcessor, std::string> {
public:
    void processRequest(const std::string& path, const std::string& name = {})
    {
        auto response = Response{};
        response.init();
        process(Request{RequestType::GET, path, name}, response);
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

    void callRequestProcessor(RequestProcessor& processor, const Request& request, Response& response) final
    {
        processor.process(request, response);
    }

    void setResponseValue(Response& response, const std::string& value) final
    {
        response.state->data = value;
    }

    void setRouteParameters(const std::vector<std::string>& params, const Request&, Response& response) final
    {
        response.routeParams = params;
    }

protected:
    std::string responseData_;
};

class StatelessRouteProcessor : public RequestProcessor {
    void process(const Request& request, Response& response) override
    {
        if (!request.name.empty())
            response.state->data = "Hello " + request.name;
        else
            response.state->data = "/name-not-found";
    }
};

TEST_F(RouterWithoutRequestType, StatelessRouteProcessor)
{
    route("/greet").process<StatelessRouteProcessor>();
    auto processor = StatelessRouteProcessor{};
    route("/greet2").process(processor);
    route("/").process([](auto&, auto& response){ response.state->data = "Hello world";});
    route(std::regex{"/greet/.*"}).process<StatelessRouteProcessor>();
    route().process([](auto&, auto& response){ response.state->data = "/404";});

    processRequest("/");
    checkResponse("Hello world");

    processRequest("/greet");
    checkResponse("/name-not-found");

    processRequest("/greet2");
    checkResponse("/name-not-found");

    processRequest("/greet/foo", "foo");
    checkResponse("Hello foo");

    processRequest("/foo");
    checkResponse("/404");
}

struct NameState {
    std::string name;
};

class StatefulRouteProcessor : public RequestProcessor {
public:
    StatefulRouteProcessor(NameState& state)
            : state_(state)
    {}

    const std::vector<std::string>& routeParams() const
    {
        return routeParams_;
    }

    void process(const Request& request, Response& response) override
    {
        routeParams_ = response.routeParams;
        if (request.requestPath == "/config" && !request.name.empty())
            state_.name = request.name;

        if (request.requestPath == "/" || request.requestPath == "/test") {
            if (state_.name.empty())
                response.state->data = "OK";
            else
                response.state->data = "Hello " + state_.name;
        } else
            response.state->data = "/";
    }

    NameState& state_;
    std::vector<std::string> routeParams_;
};

class NameSetterRouteProcessor : public RequestProcessor {
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

TEST_F(RouterWithoutRequestType, StatefulRouteProcessor)
{
    NameState state;
    auto processor = StatefulRouteProcessor{state};
    route(std::regex{R"(/testparams/(.*))"}).process(processor);
    route(std::regex{R"(/testparams2/(.+)-(.+))"}).process(processor);
    route(std::regex{R"(.*)"}).process(processor);

    processRequest("/");
    checkResponse("OK");

    processRequest("/config", "foo");
    checkResponse("/");

    processRequest("/");
    checkResponse("Hello foo");

    processRequest("/testparams/123");
    EXPECT_EQ(processor.routeParams(), std::vector<std::string>{"123"});

    processRequest("/");
    EXPECT_EQ(processor.routeParams(), std::vector<std::string>{});

    processRequest("/testparams2/hello-world");
    EXPECT_EQ(processor.routeParams(), (std::vector<std::string>{"hello", "world"}));
}

TEST_F(RouterWithoutRequestType, ChainedRouteProcessor)
{
    NameState state;
    auto processor = StatefulRouteProcessor{state};
    auto nameProcessor = NameSetterRouteProcessor{state};
    route("/test").process(nameProcessor).process(processor);

    processRequest("/test");
    checkResponse("Hello TEST");
}

class CounterRouteProcessor : public RequestProcessor {
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

TEST_F(RouterWithoutRequestType, TemplateProcessorInMultipleRoutes)
{
    int state = 0;
    route("/test").process<CounterRouteProcessor>(state);
    route("/test2").process<CounterRouteProcessor>(state);

    processRequest("/test");
    processRequest("/test2");
    ASSERT_EQ(state, 1); //Which means that routes contain different processor objects
}

}