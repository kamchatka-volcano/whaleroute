#include "common.h"
#include <whaleroute/requestrouter.h>
#include <gtest/gtest.h>

struct ChapterString {
    std::string value;
};

namespace whaleroute::config {
template<>
struct RouteMatcher<RequestType> {
    bool operator()(RequestType value, const Request& request) const
    {
        return value == request.type;
    }
};

template<>
struct StringConverter<ChapterString> {
    static std::optional<ChapterString> fromString(const std::string& data)
    {
        return ChapterString{data};
    }
};

} // namespace whaleroute::config

namespace {
class RouterWithoutResponseValue : public ::testing::Test,
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

struct ChapterNameProcessor {
    void operator()(const ChapterString& chapterName, const Request&, Response& response)
    {
        response.send("Chapter: " + chapterName.value);
    }
};

} // namespace

TEST_F(RouterWithoutResponseValue, Matching)
{
    route("/", RequestType::GET).process<GreeterPageProcessor>();
    route("/moon", RequestType::GET).process<GreeterPageProcessor>("Moon");
    route("/page0", RequestType::GET)
            .process(
                    [](const Request&, Response& response)
                    {
                        response.send("Default page");
                    });
    route(whaleroute::rx{R"(/page\d*)"}, RequestType::GET)
            .process(
                    [](const Request&, Response& response)
                    {
                        response.send("Some page");
                    });
    route("/upload", RequestType::POST)
            .process(
                    [](const Request&, Response& response)
                    {
                        response.send("OK");
                    });
    route(whaleroute::rx{R"(/chapter/(.+)/page(\d+))"}, RequestType::GET).process<ChapterNamePageIndexProcessor>();
    route(whaleroute::rx{R"(/chapter_(.+)/page_(\d+))"}, RequestType::GET)
            .process<ChapterNamePageIndexProcessor>("TestBook");
    auto parametrizedProcessor = ChapterNameProcessor{};
    route(whaleroute::rx{R"(/chapter_(.+)/)"}, RequestType::GET).process(parametrizedProcessor);
    route("/param_error").process(parametrizedProcessor);
    route(whaleroute::rx{R"(/files/(.*\.xml))"}, RequestType::GET)
            .process(
                    [](const std::string& fileName, const Request&, Response& response)
                    {
                        auto fileContent = std::string{"XML file: " + fileName};
                        response.send(fileContent);
                    });
    route().process(
            [](const Request&, Response& response)
            {
                response.state->data = "404";
            });

    processRequest("/");
    checkResponse("Hello world");
    processRequest("/", RequestType::POST);
    checkResponse("404");

    processRequest("/moon");
    checkResponse("Hello Moon");

    processRequest("/upload", RequestType::POST);
    checkResponse("OK");
    processRequest("/upload", RequestType::GET);
    checkResponse("404");

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

TEST_F(RouterWithoutResponseValue, DefaultUnmatchedRequestHandler)
{
    route("/", RequestType::GET)
            .process(
                    [](const Request&, Response& response)
                    {
                        response.send("Hello world");
                    });
    processRequest("/foo");
    checkResponse("NO_MATCH");
    processRequest("/foo", RequestType::POST);
    checkResponse("NO_MATCH");
}

TEST_F(RouterWithoutResponseValue, MultipleRoutesMatching)
{
    route(whaleroute::rx{"/greet/.*"}, RequestType::GET)
            .process(
                    [](const Request&, Response& response)
                    {
                        response.state->data = "Hello";
                    });
    route("/greet/world", RequestType::GET)
            .process(
                    [](const Request& request, Response& response)
                    {
                        response.state->data += " world" + (request.name.empty() ? std::string{} : " " + request.name);
                        response.state->wasSent = true;
                    });
    auto testState = std::string{};
    route(whaleroute::rx{"/thank/.*"}, RequestType::GET)
            .process(
                    [](const Request&, Response& response)
                    {
                        response.state->data = "Thanks";
                    });
    route("/thank/world", RequestType::GET)
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

    route("/", RequestType::GET)
            .process(
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

TEST_F(RouterWithoutResponseValue, SameProcessorObjectUsedInMultipleRoutes)
{
    int state = 0;
    auto counterProcessor = CounterRouteProcessor{state};
    route("/test", RequestType::GET).process(counterProcessor);
    route("/test2", RequestType::GET).process(counterProcessor);

    processRequest("/test");
    checkResponse("TEST");
    processRequest("/test2");
    checkResponse("TEST");
    ASSERT_EQ(state, 2); // Which means that routes contain the same processor object
}

TEST_F(RouterWithoutResponseValue, SameProcessorTypeCreatedInMultipleRoutes)
{
    int state = 0;
    route("/test", RequestType::GET).process<CounterRouteProcessor>(state);
    route("/test2", RequestType::GET).process<CounterRouteProcessor>(state);

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

TEST_F(RouterWithoutResponseValue, SameParametrizedProcessorObjectUsedInMultipleRoutes)
{
    int state = 0;
    auto counterProcessor = ParametrizedCounterRouteProcessor{state};
    route(whaleroute::rx{"/test/(.+)"}, RequestType::GET).process(counterProcessor);
    route(whaleroute::rx{"/test2/(.+)"}, RequestType::GET).process(counterProcessor);

    processRequest("/test/foo");
    checkResponse("TEST foo");
    processRequest("/test2/bar");
    checkResponse("TEST bar");
    ASSERT_EQ(state, 2); // Which means that routes contain the same processor object
}

TEST_F(RouterWithoutResponseValue, SameParametrizedProcessorTypeCreatedInMultipleRoutes)
{
    int state = 0;
    route(whaleroute::rx{"/test/(.+)"}, RequestType::GET).process<ParametrizedCounterRouteProcessor>(state);
    route(whaleroute::rx{"/test2/(.+)"}, RequestType::GET).process<ParametrizedCounterRouteProcessor>(state);

    processRequest("/test/foo");
    checkResponse("TEST foo");
    processRequest("/test2/bar");
    checkResponse("TEST bar");
    ASSERT_EQ(state, 1); // Which means that routes contain different processor objects
}
