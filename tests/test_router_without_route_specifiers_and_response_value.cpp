#include "common.h"
#include <whaleroute/requestrouter.h>
#include <gtest/gtest.h>

struct ChapterString{
    std::string value;
};

namespace whaleroute::config
{
template<>
struct StringConverter<ChapterString>{
    static std::optional<ChapterString> fromString(const std::string& data)
    {
        return ChapterString{data};
    }
};
}


namespace without_route_specifiers_and_response_value{
class RouterWithoutRouteSpecifiersAndResponseValue : public ::testing::Test,
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

protected:
    std::string responseData_;
};

class GreeterPageProcessor : public whaleroute::RequestProcessor<Request, Response>
{
public:
    GreeterPageProcessor(std::string name = {})
        : name_(std::move(name))
    {}

private:
    void process(const Request&, Response& response) override
    {
        response.send("Hello " + (name_.empty() ? "world" : name_));
    }
    std::string name_;
};

class ChapterNamePageIndexProcessor : public whaleroute::RequestProcessor<Request, Response, std::string, int>
{
public:
    ChapterNamePageIndexProcessor(std::string title = {})
        : title_{std::move(title)}
    {}

private:
    void process(const std::string& chapterName, const int& pageIndex, const Request&, Response& response) override
    {
        response.send(title_ + (title_.empty() ? "" : " ") + "Chapter: " + chapterName + ", page[" + std::to_string(pageIndex) + "]");
    }
    void onRouteParametersError(const Request&, Response& response, const whaleroute::RouteParameterError&) override
    {
        response.send("ROUTE_PARAM_ERROR");
    }
    std::string title_;
};

class ChapterNameProcessor : public whaleroute::RequestProcessor<Request, Response, ChapterString>
{
    void process(const ChapterString& chapterName, const Request&, Response& response) override
    {
        response.send("Chapter: " + chapterName.value);
    }
    void onRouteParametersError(const Request&, Response& response, const whaleroute::RouteParameterError&) override
    {
        response.send("ROUTE_PARAM_ERROR");
    }
};

TEST_F(RouterWithoutRouteSpecifiersAndResponseValue, Matching)
{
    route("/").process<GreeterPageProcessor>();
    route("/moon").process<GreeterPageProcessor>("Moon");
    route("/page0").process([](auto&, auto& response){ response.send("Default page");});
    route(std::regex{R"(/page\d*)"}).process([](auto&, auto& response){ response.send("Some page");});
    route("/upload").process([](auto&, auto& response){ response.send("OK");});
    route(std::regex{R"(/chapter/(.+)/page(\d+))"}).process<ChapterNamePageIndexProcessor>();
    route(std::regex{R"(/chapter_(.+)/page_(\d+))"}).process<ChapterNamePageIndexProcessor>("TestBook");
    auto parametrizedProcessor = ChapterNameProcessor{};
    route(std::regex{R"(/chapter_(.+)/)"}).process(parametrizedProcessor);
    route("/param_error").process(parametrizedProcessor);
    route(std::regex{R"(/files/(.*\.xml))"}).process<std::string>(
            [](const std::string& fileName, const Request&, Response& response) {
                auto fileContent = std::string{"XML file: " + fileName};
                response.send(fileContent);
            });
    route().process([](auto&, auto& response){ response.send("404");});

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
    checkResponse("ROUTE_PARAM_ERROR");

    processRequest("/files/test.xml");
    checkResponse("XML file: test.xml");

    processRequest("/files/test.xml1");
    checkResponse("404");

    processRequest("/foo");
    checkResponse("404");
}

TEST_F(RouterWithoutRouteSpecifiersAndResponseValue, DefaultUnmatchedRequestHandler)
{
     route("/").process([](const auto&, auto& response){
         response.send("Hello world");
     });
     processRequest("/foo");
     checkResponse("NO_MATCH");
}

TEST_F(RouterWithoutRouteSpecifiersAndResponseValue, MultipleRoutesMatching)
{
    route(std::regex{"/greet/.*"}).process([](const auto&, auto& response) {
        response.state->data = "Hello";
    });
    route("/greet/world").process([](const auto& request, auto& response) {
        response.state->data += " world" + (request.name.empty() ? std::string{} : " " + request.name);
        response.state->wasSent = true;
    });
    auto testState = std::string{};
    route(std::regex{"/thank/.*"}).process([](const auto&, auto& response) {
                                                         response.state->data = "Thanks";});
    route("/thank/world").process([](const auto&, auto& response) { //Chained processors
                                                response.state->data += " world";
                                                response.state->wasSent = true;})
                                           .process([&testState](const auto&, auto&) { //should be invoked even when processing is finished by sending the response
                                                testState = "TEST";});

    route("/").process([](auto&, auto& response){ response.send("Hello Bill");});
    route().process([](auto&, auto& response){ response.send("404");});

    processRequest("/greet/world", RequestType::GET, "Request#1"); //Sending request with name, to test that the same request object is used in both processors
    checkResponse("Hello world Request#1");
    processRequest("/greet/moon");
    checkResponse("404");
    processRequest("/thank/world");
    checkResponse("Thanks world");
    EXPECT_EQ(testState, "TEST"); //Ensures that chained processor was called after response had been sent
    processRequest("/thank/moon");
    checkResponse("404");
    processRequest("/");
    checkResponse("Hello Bill");
}

class CounterRouteProcessor : public whaleroute::RequestProcessor<Request, Response>
{
public:
    CounterRouteProcessor(int& state)
        : state_(state)
    {}

    void process(const Request&, Response& response) override
    {
        state_ = ++counter;
        response.send("TEST");
    }

private:
    int counter = 0;
    int& state_;
};

TEST_F(RouterWithoutRouteSpecifiersAndResponseValue, SameProcessorObjectUsedInMultipleRoutes){
    int state = 0;
    auto counterProcessor = CounterRouteProcessor{state};
    route("/test").process(counterProcessor);
    route("/test2").process(counterProcessor);

    processRequest("/test");
    checkResponse("TEST");
    processRequest("/test2");
    checkResponse("TEST");
    ASSERT_EQ(state, 2); //Which means that routes contain the same processor object
}

TEST_F(RouterWithoutRouteSpecifiersAndResponseValue, SameProcessorTypeCreatedInMultipleRoutes){
    int state = 0;
    route("/test").process<CounterRouteProcessor>(state);
    route("/test2").process<CounterRouteProcessor>(state);

    processRequest("/test");
    checkResponse("TEST");
    processRequest("/test2");
    checkResponse("TEST");
    ASSERT_EQ(state, 1); //Which means that routes contain different processor objects
}

class ParametrizedCounterRouteProcessor : public whaleroute::RequestProcessor<Request, Response, std::string>
{
public:
    ParametrizedCounterRouteProcessor(int& state)
        : state_(state)
    {}

    void process(const std::string& param, const Request&, Response& response) override
    {
        state_ = ++counter;
        response.send("TEST " + param);
    }

private:
    int counter = 0;
    int& state_;
};

TEST_F(RouterWithoutRouteSpecifiersAndResponseValue, SameParametrizedProcessorObjectUsedInMultipleRoutes){
    int state = 0;
    auto counterProcessor = ParametrizedCounterRouteProcessor{state};
    route(std::regex{"/test/(.+)"}).process(counterProcessor);
    route(std::regex{"/test2/(.+)"}).process(counterProcessor);

    processRequest("/test/foo");
    checkResponse("TEST foo");
    processRequest("/test2/bar");
    checkResponse("TEST bar");
    ASSERT_EQ(state, 2); //Which means that routes contain the same processor object
}

TEST_F(RouterWithoutRouteSpecifiersAndResponseValue, SameParametrizedProcessorTypeCreatedInMultipleRoutes){
    int state = 0;
    route(std::regex{"/test/(.+)"}).process<ParametrizedCounterRouteProcessor>(state);
    route(std::regex{"/test2/(.+)"}).process<ParametrizedCounterRouteProcessor>(state);

    processRequest("/test/foo");
    checkResponse("TEST foo");
    processRequest("/test2/bar");
    checkResponse("TEST bar");
    ASSERT_EQ(state, 1); //Which means that routes contain different processor objects
}


}