#include "common.h"
#include <whaleroute/requestrouter.h>
#include <gtest/gtest.h>
#include <sstream>

struct ChapterString {
    std::string value;
};

namespace {
class AltRegexSpecifying;
}

namespace whaleroute::config {
template <>
struct RouteMatcher<AltRegexSpecifying, RequestType> {
    bool operator()(RequestType value, const Request& request, Response&) const
    {
        return value == request.type;
    }
};

template <>
struct StringConverter<ChapterString> {
    static std::optional<ChapterString> fromString(const std::string& data)
    {
        return ChapterString{data};
    }
};
} // namespace whaleroute::config

namespace {

class AltRegexSpecifying : public ::testing::Test,
                           public whaleroute::RequestRouter<AltRegexSpecifying, Request, Response, std::string> {
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

    void setResponseValue(Response& response, const std::string& value) final
    {
        response.send(value);
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
                title_ + (title_.empty() ? "" : " ") + "Chapter: " + chapterName + ", page[" +
                std::to_string(pageIndex) + "]");
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

TEST_F(AltRegexSpecifying, StringLiterals)
{
    using namespace whaleroute::string_literals;

    route("/", RequestType::GET).process<GreeterPageProcessor>();
    route("/moon", RequestType::GET).process<GreeterPageProcessor>("Moon");
    route("/page0", RequestType::GET)
            .process(
                    [](const Request&, Response& response)
                    {
                        response.send("Default page");
                    });
    route(R"(/page\d*)"_rx, RequestType::GET)
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
    route(R"(/chapter/(.+)/page(\d+))"_rx, RequestType::GET).process<ChapterNamePageIndexProcessor>();
    route(R"(/chapter_(.+)/page_(\d+))"_rx, RequestType::GET).process<ChapterNamePageIndexProcessor>("TestBook");
    auto parametrizedProcessor = ChapterNameProcessor{};
    route(R"(/chapter_(.+))"_rx, RequestType::GET).process(parametrizedProcessor);
    route("/param_error").process(parametrizedProcessor);
    route(R"(/files/(.*\.xml))"_rx, RequestType::GET)
            .process(
                    [](const std::string& fileName, const Request&, Response& response)
                    {
                        auto fileContent = std::string{"XML file: " + fileName};
                        response.send(fileContent);
                    });
    route().set("404");

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

    processRequest("/chapter_test");
    checkResponse("Chapter: test");

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

TEST_F(AltRegexSpecifying, TildaEscape)
{
    setRegexMode(whaleroute::RegexMode::TildaEscape);
    route("/", RequestType::GET).process<GreeterPageProcessor>();
    route("/moon", RequestType::GET).process<GreeterPageProcessor>("Moon");
    route("/page0", RequestType::GET)
            .process(
                    [](const Request&, Response& response)
                    {
                        response.send("Default page");
                    });
    route(whaleroute::rx{"/page~d*"}, RequestType::GET)
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
    route(whaleroute::rx{"/chapter/(.+)/page(~d+)"}, RequestType::GET).process<ChapterNamePageIndexProcessor>();
    route(whaleroute::rx{"/chapter_(.+)/page_(~d+)"}, RequestType::GET)
            .process<ChapterNamePageIndexProcessor>("TestBook");
    auto parametrizedProcessor = ChapterNameProcessor{};
    route(whaleroute::rx{"/chapter_(.+)"}, RequestType::GET).process(parametrizedProcessor);
    route("/param_error").process(parametrizedProcessor);
    route(whaleroute::rx{"/files/(.*~.xml)"}, RequestType::GET)
            .process(
                    [](const std::string& fileName, const Request&, Response& response)
                    {
                        auto fileContent = std::string{"XML file: " + fileName};
                        response.send(fileContent);
                    });
    route().set("404");

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

    processRequest("/chapter_test");
    checkResponse("Chapter: test");

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

TEST_F(AltRegexSpecifying, StringLiteralsAndTildaEscape)
{
    using namespace whaleroute::string_literals;
    setRegexMode(whaleroute::RegexMode::TildaEscape);

    route("/", RequestType::GET).process<GreeterPageProcessor>();
    route("/moon", RequestType::GET).process<GreeterPageProcessor>("Moon");
    route("/moon~~"_rx, RequestType::GET).process<GreeterPageProcessor>("Moon~");
    route("/~~moon"_rx, RequestType::GET).process<GreeterPageProcessor>("~Moon");
    route("/m~~n"_rx, RequestType::GET).process<GreeterPageProcessor>("M~n");
    route("/~~m~~n~~"_rx, RequestType::GET).process<GreeterPageProcessor>("~M~n~");
    route("/page0", RequestType::GET)
            .process(
                    [](const Request&, Response& response)
                    {
                        response.send("Default page");
                    });
    route("/page~d*"_rx, RequestType::GET)
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
    route("/chapter/(.+)/page(~d+)"_rx, RequestType::GET).process<ChapterNamePageIndexProcessor>();
    route("/chapter_(.+)/page_(~d+)"_rx, RequestType::GET).process<ChapterNamePageIndexProcessor>("TestBook");
    auto parametrizedProcessor = ChapterNameProcessor{};
    route("/chapter_(.+)"_rx, RequestType::GET).process(parametrizedProcessor);
    route("/param_error").process(parametrizedProcessor);
    route("/files/(.*~.xml)"_rx, RequestType::GET)
            .process(
                    [](const std::string& fileName, const Request&, Response& response)
                    {
                        auto fileContent = std::string{"XML file: " + fileName};
                        response.send(fileContent);
                    });
    route().set("404");

    processRequest("/");
    checkResponse("Hello world");
    processRequest("/", RequestType::POST);
    checkResponse("404");

    processRequest("/moon");
    checkResponse("Hello Moon");

    processRequest("/moon~");
    checkResponse("Hello Moon~");

    processRequest("/~moon");
    checkResponse("Hello ~Moon");

    processRequest("/m~n");
    checkResponse("Hello M~n");

    processRequest("/~m~n~");
    checkResponse("Hello ~M~n~");

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

    processRequest("/chapter_test");
    checkResponse("Chapter: test");

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
