#include "common.h"
#include <whaleroute/requestrouter.h>
#include <gtest/gtest.h>

namespace without_response_value {

class RouterWithoutResponseValue : public ::testing::Test,
                                   public whaleroute::RequestRouter<RequestProcessor, Request, RequestMethod, Response> {
public:
    void processRequest(RequestMethod method, const std::string& path, const std::string& name = {})
    {
        auto response = Response{};
        process(Request{method, path, name}, response);
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

    void callRequestProcessor(RequestProcessor& processor, const Request& request, Response& response) final
    {
        processor.process(request, response);
    }

protected:
    std::string responseData_;
};

class StatelessRouteProcessor : public RequestProcessor {
    void process(const Request& request, Response& response) override
    {
        if (!request.name.empty())
            response.data = "Hello " + request.name;
        else
            response.data = "/name-not-found";
    }
};

TEST_F(RouterWithoutResponseValue, StatelessRouteProcessor)
{
    route("/greet", RequestMethod::GET).process<StatelessRouteProcessor>();
    auto processor = StatelessRouteProcessor{};
    route("/greet2", RequestMethod::GET).process(processor);
    route("/", RequestMethod::GET).process([](auto&, auto& response){ response.data = "Hello world";});
    route(std::regex{"/greet/.*"}, RequestMethod::GET).process<StatelessRouteProcessor>();
    route().process([](auto&, auto& response){ response.data = "/404";});

    processRequest(RequestMethod::GET, "/");
    checkResponse("Hello world");

    processRequest(RequestMethod::GET, "/greet");
    checkResponse("/name-not-found");

    processRequest(RequestMethod::GET, "/greet2");
    checkResponse("/name-not-found");

    processRequest(RequestMethod::GET, "/greet/foo", "foo");
    checkResponse("Hello foo");

    processRequest(RequestMethod::POST, "/foo");
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

    void process(const Request& request, Response& response) override
    {
        if (request.requestPath == "/config" && !request.name.empty())
            state_.name = request.name;

        if (request.requestPath == "/" || request.requestPath == "/test") {
            if (state_.name.empty())
                response.data = "OK";
            else
                response.data = "Hello " + state_.name;
        } else
            response.data = "/";
    }

    NameState& state_;
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

TEST_F(RouterWithoutResponseValue, StatefulRouteProcessor)
{
    NameState state;
    auto processor = StatefulRouteProcessor{state};
    route(std::regex{R"(.*)"}, RequestMethod::GET).process(processor);

    processRequest(RequestMethod::GET, "/");
    checkResponse("OK");

    processRequest(RequestMethod::GET, "/config", "foo");
    checkResponse("/");

    processRequest(RequestMethod::GET, "/");
    checkResponse("Hello foo");
}

TEST_F(RouterWithoutResponseValue, ChainedRouteProcessor)
{
    NameState state;
    auto processor = StatefulRouteProcessor{state};
    auto nameProcessor = NameSetterRouteProcessor{state};
    route("/test", RequestMethod::GET).process(nameProcessor).process(processor);

    processRequest(RequestMethod::GET, "/test");
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
        response.data = "TEST";
    }

private:
    int counter = 0;
    int& state_;
};

TEST_F(RouterWithoutResponseValue, TemplateProcessorInMultipleRoutes)
{
    int state = 0;
    route("/test", RequestMethod::GET).process<CounterRouteProcessor>(state);
    route("/test2", RequestMethod::GET).process<CounterRouteProcessor>(state);

    processRequest(RequestMethod::GET, "/test");
    processRequest(RequestMethod::GET, "/test2");
    ASSERT_EQ(state, 2); //Which means that both route shares the same processor object
}

}