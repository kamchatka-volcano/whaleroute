#include "common.h"
#include <whaleroute/requestrouter.h>
#include <gtest/gtest.h>

struct ChapterString {
    std::string value;
};

namespace whaleroute::config {
template<>
struct StringConverter<ChapterString> {
    static std::optional<ChapterString> fromString(const std::string& data)
    {
        return ChapterString{data};
    }
};
} // namespace whaleroute::config

namespace {
class RouterWithoutRouteMatchersAndResponseValue : public ::testing::Test,
                                                   public whaleroute::RequestRouter<Request, Response> {
public:
    void processRequest(const std::string& path, RequestType requestType = RequestType::GET, std::string name = {})
    {
        auto response = Response{};
        response.init();
        process(Request{requestType, path, std::move(name)}, response);
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
        response.send("NO_MATCH");
    }

    bool isRouteProcessingFinished(const Request&, Response& response) const final
    {
        return response.state->wasSent;
    }

    void onRouteParametersError(const Request&, Response& response, const whaleroute::RouteParameterError& error)
            override
    {
        response.send(getRouteParamErrorInfo(error));
    }

protected:
    std::string responseData_;
};

class GreeterPageProcessor {
public:
    GreeterPageProcessor(std::string name = {})
        : name_(std::move(name))
    {
    }

    void operator()(const Request&, Response& response)
    {
        response.send("Hello " + (name_.empty() ? "world" : name_));
    }

private:
    std::string name_;
};

class ChapterNamePageIndexProcessor {
public:
    ChapterNamePageIndexProcessor(std::string title = {})
        : title_{std::move(title)}
    {
    }

    void operator()(const std::string& chapterName, const int& pageIndex, const Request&, Response& response)
    {
        response.send(
                title_ + (title_.empty() ? "" : " ") + "Chapter: " + chapterName + //
                ", page[" + std::to_string(pageIndex) + "]");
    }

private:
    std::string title_;
};

class ChapterNameProcessor {
public:
    void operator()(const ChapterString& chapterName, const Request&, Response& response)
    {
        response.send("Chapter: " + chapterName.value);
    }
};

} // namespace

TEST_F(RouterWithoutRouteMatchersAndResponseValue, Matching)
{
    route("/").process<GreeterPageProcessor>();
    route("/moon").process<GreeterPageProcessor>("Moon");
    route("/page0").process(
            [](const Request&, Response& response)
            {
                response.send("Default page");
            });
    route(whaleroute::rx{R"(/page\d*)"})
            .process(
                    [](const Request&, Response& response)
                    {
                        response.send("Some page");
                    });
    route("/upload").process(
            [](const Request&, Response& response)
            {
                response.send("OK");
            });
    route(whaleroute::rx{R"(/chapter/(.+)/page(\d+))"}).process<ChapterNamePageIndexProcessor>();
    route(whaleroute::rx{R"(/chapter_(.+)/page_(\d+))"}).process<ChapterNamePageIndexProcessor>("TestBook");
    auto parametrizedProcessor = ChapterNameProcessor{};
    route(whaleroute::rx{R"(/chapter_(.+)/)"}).process(parametrizedProcessor);
    route("/param_error").process(parametrizedProcessor);
    route(whaleroute::rx{R"(/files/(.*\.xml))"})
            .process(
                    [](const std::string& fileName, const Request&, Response& response)
                    {
                        auto fileContent = std::string{"XML file: " + fileName};
                        response.send(fileContent);
                    });
    route().process(
            [](const Request&, Response& response)
            {
                response.send("404");
            });

    processRequest("/");
    checkResponse("Hello world");

    processRequest("/moon");
    checkResponse("Hello Moon");

    processRequest("/upload");
    checkResponse("OK");

    processRequest("/page123");
    checkResponse("Some page");

    processRequest("/page0");
    checkResponse("Default page");

    processRequest("/chapter/test/page123");
    checkResponse("Chapter: test, page[123]");

    processRequest("/chapter_test/page_123");
    checkResponse("TestBook Chapter: test, page[123]");

    processRequest("/chapter_test/");
    checkResponse("Chapter: test");

    processRequest("/param_error/");
    checkResponse("ROUTE_PARAM_ERROR: PARAM COUNT MISMATCH, EXPECTED:1 ACTUAL:0");

    processRequest("/files/test.xml");
    checkResponse("XML file: test.xml");

    processRequest("/files/test.xml1");
    checkResponse("404");

    processRequest("/foo");
    checkResponse("404");
}

TEST_F(RouterWithoutRouteMatchersAndResponseValue, DefaultUnmatchedRequestHandler)
{
    route("/").process(
            [](const Request&, Response& response)
            {
                response.send("Hello world");
            });
    processRequest("/foo");
    checkResponse("NO_MATCH");
}

TEST_F(RouterWithoutRouteMatchersAndResponseValue, MultipleRoutesMatching)
{
    route(whaleroute::rx{"/greet/.*"})
            .process(
                    [](const Request&, Response& response)
                    {
                        response.state->data = "Hello";
                    });
    route("/greet/world")
            .process(
                    [](const Request& request, Response& response)
                    {
                        response.state->data += " world" + (request.name.empty() ? std::string{} : " " + request.name);
                        response.state->wasSent = true;
                    });
    auto testState = std::string{};
    route(whaleroute::rx{"/thank/.*"})
            .process(
                    [](const Request&, Response& response)
                    {
                        response.state->data = "Thanks";
                    });
    route("/thank/world")
            .process(
                    [](const Request&, Response& response) { // Chained processors
                        response.state->data += " world";
                        response.state->wasSent = true;
                    })
            .process([&testState](
                             const Request&,
                             Response&) { // should be invoked even when processing is finished by sending the response
                testState = "TEST";
            });

    route("/").process(
            [](const Request&, Response& response)
            {
                response.send("Hello Bill");
            });
    route().process(
            [](const Request&, Response& response)
            {
                response.send("404");
            });

    processRequest(
            "/greet/world",
            RequestType::GET,
            "Request#1"); // Sending request with name, to test that the same request object is used in both processors
    checkResponse("Hello world Request#1");
    processRequest("/greet/moon");
    checkResponse("404");
    processRequest("/thank/world");
    checkResponse("Thanks world");
    EXPECT_EQ(testState, "TEST"); // Ensures that chained processor was called after response had been sent
    processRequest("/thank/moon");
    checkResponse("404");
    processRequest("/");
    checkResponse("Hello Bill");
}

class CounterRouteProcessor {
public:
    CounterRouteProcessor(int& state)
        : state_(state)
    {
    }

    void operator()(const Request&, Response& response)
    {
        state_ = ++counter;
        response.send("TEST");
    }

private:
    int counter = 0;
    int& state_;
};

TEST_F(RouterWithoutRouteMatchersAndResponseValue, SameProcessorObjectUsedInMultipleRoutes)
{
    int state = 0;
    auto counterProcessor = CounterRouteProcessor{state};
    route("/test").process(counterProcessor);
    route("/test2").process(counterProcessor);

    processRequest("/test");
    checkResponse("TEST");
    processRequest("/test2");
    checkResponse("TEST");
    ASSERT_EQ(state, 2); // Which means that routes contain the same processor object
}

TEST_F(RouterWithoutRouteMatchersAndResponseValue, SameProcessorTypeCreatedInMultipleRoutes)
{
    int state = 0;
    route("/test").process<CounterRouteProcessor>(state);
    route("/test2").process<CounterRouteProcessor>(state);

    processRequest("/test");
    checkResponse("TEST");
    processRequest("/test2");
    checkResponse("TEST");
    ASSERT_EQ(state, 1); // Which means that routes contain different processor objects
}

class ParametrizedCounterRouteProcessor {
public:
    ParametrizedCounterRouteProcessor(int& state)
        : state_(state)
    {
    }

    void operator()(const std::string& param, const Request&, Response& response)
    {
        state_ = ++counter;
        response.send("TEST " + param);
    }

private:
    int counter = 0;
    int& state_;
};

TEST_F(RouterWithoutRouteMatchersAndResponseValue, SameParametrizedProcessorObjectUsedInMultipleRoutes)
{
    int state = 0;
    auto counterProcessor = ParametrizedCounterRouteProcessor{state};
    route(whaleroute::rx{"/test/(.+)"}).process(counterProcessor);
    route(whaleroute::rx{"/test2/(.+)"}).process(counterProcessor);

    processRequest("/test/foo");
    checkResponse("TEST foo");
    processRequest("/test2/bar");
    checkResponse("TEST bar");
    ASSERT_EQ(state, 2); // Which means that routes contain the same processor object
}

TEST_F(RouterWithoutRouteMatchersAndResponseValue, SameParametrizedProcessorTypeCreatedInMultipleRoutes)
{
    int state = 0;
    route(whaleroute::rx{"/test/(.+)"}).process<ParametrizedCounterRouteProcessor>(state);
    route(whaleroute::rx{"/test2/(.+)"}).process<ParametrizedCounterRouteProcessor>(state);

    processRequest("/test/foo");
    checkResponse("TEST foo");
    processRequest("/test2/bar");
    checkResponse("TEST bar");
    ASSERT_EQ(state, 1); // Which means that routes contain different processor objects
}
