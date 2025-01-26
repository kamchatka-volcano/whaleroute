#include "common.h"
#include <whaleroute/requestrouter.h>
#include <whaleroute/routeparam.h>
#include <gtest/gtest.h>
#include <sstream>
#include <unordered_map>

namespace whaleroute::config{
template<>
struct RouteParam<routeParamId("int")> {
    using type = int;
    inline static std::string_view name = "int";
    inline static std::string_view regex = R"(\d+)";
};

template<>
struct RouteParam<routeParamId("str")> {
    using type = std::string;
    inline static std::string_view name = "str";
    inline static std::string_view regex = R"([\w\$-\.\+!*'\(\)]+)";
};

template<>
struct RouteParam<routeParamId("path")> {
    using type = std::string;
    inline static std::string_view name = "path";
    inline static std::string_view regex = R"([\w\$-\.\+!*'\(\)/]+)";
};
}

struct ChapterString {
    std::string value;
};

struct Context {
    int counter = 0;
};

namespace whaleroute::config {
template<>
struct RouteMatcher<RequestType, Context> {
    bool operator()(const RequestType& value, const Request& request, const Context&) const
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

struct ResponseSender {
    void operator()(Response& response, const std::string& data)
    {
        response.send(data);
    }
};

class Router : public ::testing::Test,
               public whaleroute::RequestRouter<Request, Response, ResponseSender, Context> {
public:
    void processRequest(const std::string& path, RequestType requestType = RequestType::GET, std::string name = {})
    {
        auto response = Response{};
        response.init();
        const auto request = Request{requestType, path, std::move(name)};
        process(request, response);
        responseData_ = response.state->data;
    }

    void checkResponse(const std::string& expectedResponseData)
    {
        EXPECT_EQ(responseData_, expectedResponseData);
    }

    void onRouteParametersError(const Request&, Response& response, const whaleroute::RouteParameterError& error)
            override
    {
        response.send(getRouteParamErrorInfo(error));
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

class RouterWithRouteParams : public ::testing::Test,
               public whaleroute::RequestRouter<Request, Response, ResponseSender, Context> {
public:
    void processRequest(const std::string& path, RequestType requestType = RequestType::GET, std::string name = {})
    {
        auto response = Response{};
        response.init();
        const auto request = Request{requestType, path, std::move(name)};
        process(request, response);
        responseData_ = response.state->data;
    }

    void checkResponse(const std::string& expectedResponseData)
    {
        EXPECT_EQ(responseData_, expectedResponseData);
    }

    void onRouteParametersError(const Request&, Response& response, const whaleroute::RouteParameterError& error)
            override
    {
        response.send(getRouteParamErrorInfo(error));
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

    std::optional<std::string_view> routeParamRegex(std::string_view paramName) const final
    {
        auto it = routeParamRegex_.find(std::string{paramName});
        if (it == routeParamRegex_.end())
            return std::nullopt;
        return it->second;
    }

protected:
    std::string responseData_;
    std::unordered_map<std::string, std::string> routeParamRegex_ = {
            {"int", R"(\d+)"},
            {"str", R"([\w\$-\.\+!*'\(\)]+)"},
            {"path", R"([\w\$-\.\+!*'\(\)/]+)"},
            {"chapter_str", R"(\w+)"}};
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

class OptionalChapterNamePageIndexProcessor {
public:
    OptionalChapterNamePageIndexProcessor(std::string title = {})
        : title_{std::move(title)}
    {
    }

    void operator()(const std::optional<std::string>& chapterName, std::optional<int> pageIndex, const Request&, Response& response)
    {
        response.send(
                title_ + (title_.empty() ? "" : " ") + "Chapter: " + chapterName.value_or("no-chapter") + ", page[" +
                std::to_string(pageIndex.value_or(0)) + "]");
    }

private:
    std::string title_;
};

struct BookProcessor {
    void operator()(const whaleroute::RouteParameters<3>& params, const Request&, Response& response)
    {
        response.send(
                "Book: " + params.value.at(0) + ", Chapter: " + params.value.at(1) + ", page[" + params.value.at(2) +
                "]");
    }
};

struct BookProcessorForAnyParams {
    void operator()(const whaleroute::RouteParameters<>& params, const Request&, Response& response)
    {
        auto id = std::string{};
        for (const auto& param : params.value)
            id += param + "#";
        if (!id.empty())
            id.pop_back();
        response.send("Book: " + id);
    }
};

class ChapterNameProcessor {
public:
    void operator()(const ChapterString& chapterName, const Request&, Response& response)
    {
        response.send("Chapter: " + chapterName.value);
    }
};

struct IncrementContext {
    void operator()(const Request&, Response&, Context& context)
    {
        context.counter++;
    }
};

struct NoResponseRequestProcessor{
    void operator()(const Request&)
    {
        testNumber++;
    }
    int testNumber = 0;
};

struct ParametrizedNoResponseRequestProcessor{
    void operator()(int param, const Request&)
    {
        testNumber += param;
    }
    int testNumber = 0;
};

struct AltIncrementContext {
    void operator()(const Request&, Context& context)
    {
        context.counter++;
    }
};

struct ParametrizedAltIncrementContext {
    void operator()(int param, const Request&, Context& context)
    {
        context.counter += param;
    }
};


struct SendContext {
    void operator()(const Request&, Response& response, const Context& context)
    {
        response.send(std::to_string(context.counter));
    }
};

} // namespace


TEST_F(Router, Matching)
{
    routeRegex({".+"}).process<IncrementContext>();
    route("/", RequestType::GET).process<GreeterPageProcessor>();
    route("/moon", RequestType::GET).process<GreeterPageProcessor>("Moon");
    route("/page0", RequestType::GET)
            .process(
                    [](const Request&, Response& response)
                    {
                        response.send("Default page");
                    });
    routeRegex({R"(/page\d*)"}, RequestType::GET)
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
    routeRegex({R"(/chapter/(.+)/page(\d+)/)"}, RequestType::GET).process<ChapterNamePageIndexProcessor>();
    routeRegex({R"(/chapter_(.+)/page_(\d+)/)"}, RequestType::GET)
            .process<ChapterNamePageIndexProcessor>("TestBook");
    routeRegex(R"(/optional-chapter/(.+)?/page(\d+)?/)", RequestType::GET).process<OptionalChapterNamePageIndexProcessor>();
    routeRegex(R"(/optional-chapter2(?:/(\w+))?(?:/(\d+))?/)", RequestType::GET).process<OptionalChapterNamePageIndexProcessor>();
    routeRegex({R"(/book-(.+)/chapter/(.+)/page/(\d+)/)"}, RequestType::GET).process<BookProcessor>();
    routeRegex({R"(/book-(.+)/chapter/(.+)/)"}, RequestType::GET).process<BookProcessor>();
    routeRegex({R"(/book/(\w+))"}, RequestType::GET).process<BookProcessorForAnyParams>();
    routeRegex({R"(/book/(\w+)/(\w+)/)"}, RequestType::GET).process<BookProcessorForAnyParams>();
    routeRegex({R"(/no_capture_groups)"}, RequestType::GET).process<BookProcessorForAnyParams>();
    route("/no_capture_groups2", RequestType::GET).process<BookProcessorForAnyParams>();
    auto parametrizedProcessor = ChapterNameProcessor{};
    routeRegex({R"(/chapter_(.+)/)"}, RequestType::GET).process(parametrizedProcessor);
    route("/param_error").process(parametrizedProcessor);
    routeRegex({R"(/files/(.*\.xml))"}, RequestType::GET)
            .process(
                    [](const std::string& fileName, const Request&, Response& response)
                    {
                        auto fileContent = std::string{"XML file: " + fileName};
                        response.send(fileContent);
                    });
    routeRegex(R"(/files-optional/(.*\.xml)?)", RequestType::GET)
        .process(
                [](const std::optional<std::string>& fileName, const Request&, Response& response)
                {
                    auto fileContent = std::string{"file: " + fileName.value_or("empty")};
                    response.send(fileContent);
                });
    route("/context", RequestType::GET)
            .process(
                    [](const Request&, Response& response, Context& context)
                    {
                        response.send(std::to_string(context.counter));
                    });
    route("/context2", RequestType::GET).process<SendContext>();

    routeRegex({"/(.+)/context"}, RequestType::GET)
            .process(
                    [](const std::string& title, const Request&, Response& response, Context& context)
                    {
                        response.send(title + ": " + std::to_string(context.counter));
                    });
    routeRegex("(?:/(.+))?/optional-context", RequestType::GET)
        .process(
                [](const std::optional<std::string>& title, const Request&, Response& response, Context& context)
                {
                    response.send(title.value_or("no-title") + ": " + std::to_string(context.counter));
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

    processRequest("/book-Hello_world/chapter/test/page/123");
    checkResponse("Book: Hello_world, Chapter: test, page[123]");

    processRequest("/book-Hello_world/chapter/test/");
    checkResponse("ROUTE_PARAM_ERROR: PARAM COUNT MISMATCH, EXPECTED:3 ACTUAL:2");

    processRequest("/book/Hello/");
    checkResponse("Book: Hello");

    processRequest("/book/Hello/world/");
    checkResponse("Book: Hello#world");

    processRequest("/no_capture_groups");
    checkResponse("Book: ");
    processRequest("/no_capture_groups2");
    checkResponse("Book: ");

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

    processRequest("/files-optional/test.xml");
    checkResponse("file: test.xml");

    processRequest("/files-optional/");
    checkResponse("file: empty");

    processRequest("/foo");
    checkResponse("404");

    processRequest("/context");
    checkResponse("1");

    processRequest("/context2");
    checkResponse("1");

    processRequest("/test/context");
    checkResponse("test: 1");

    processRequest("/test/optional-context");
    checkResponse("test: 1");

    processRequest("/optional-context");
    checkResponse("no-title: 1");

    processRequest("/optional-chapter/test/page123");
    checkResponse("Chapter: test, page[123]");

    processRequest("/optional-chapter/test/page");
    checkResponse("Chapter: test, page[0]");

    processRequest("/optional-chapter2/test/123/");
    checkResponse("Chapter: test, page[123]");

    processRequest("/optional-chapter2/123/");
    checkResponse("Chapter: 123, page[0]");

    processRequest("/optional-chapter2/test/");
    checkResponse("Chapter: test, page[0]");

    processRequest("/optional-chapter2/");
    checkResponse("Chapter: no-chapter, page[0]");
}

TEST_F(RouterWithRouteParams, MatchingWithRouteParams)
{
    routeRegex({".+"}).process<IncrementContext>();
    route("/", RequestType::GET).process<GreeterPageProcessor>();
    route("/moon", RequestType::GET).process<GreeterPageProcessor>("Moon");
    route("/page0", RequestType::GET)
            .process(
                    [](const Request&, Response& response)
                    {
                        response.send("Default page");
                    });
    routeRegex({R"(/page\d*)"}, RequestType::GET)
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
    route("/chapter/{str}/page{int}/", RequestType::GET).process<ChapterNamePageIndexProcessor>();
    route("/chapter_{str}/page_{int}/", RequestType::GET).process<ChapterNamePageIndexProcessor>("TestBook");
    route("/optional-chapter/{str}?/page{int}?/", RequestType::GET).process<OptionalChapterNamePageIndexProcessor>();
    route("/optional-chapter2/{str}?/{int}?/", RequestType::GET).process<OptionalChapterNamePageIndexProcessor>();
    route("/book-{str}/chapter/{str}/page/{int}", RequestType::GET).process<BookProcessor>();
    route("/book-{str}/chapter/{str}/", RequestType::GET).process<BookProcessor>();
    route("/book/{str}/", RequestType::GET).process<BookProcessorForAnyParams>();
    route("/book/{str}/{str}/", RequestType::GET).process<BookProcessorForAnyParams>();
    route("/no_capture_groups", RequestType::GET).process<BookProcessorForAnyParams>();
    route("/no_capture_groups2", RequestType::GET).process<BookProcessorForAnyParams>();
    auto parametrizedProcessor = ChapterNameProcessor{};
    route("/chapter_{chapter_str}/", RequestType::GET).process(parametrizedProcessor);
    route("/files/{str}.xml", RequestType::GET)
            .process(
                    [](const std::string& fileName, const Request&, Response& response)
                    {
                        auto fileContent = std::string{"XML file: " + fileName};
                        response.send(fileContent);
                    });
    route("/files-optional/{str}?", RequestType::GET)
            .process(
                    [](const std::optional<std::string>& fileName, const Request&, Response& response)
                    {
                        auto fileContent = std::string{"file: " + fileName.value_or("empty")};
                        response.send(fileContent);
                    });
    route("/context", RequestType::GET)
            .process(
                    [](const Request&, Response& response, Context& context)
                    {
                        response.send(std::to_string(context.counter));
                    });
    route("/context2", RequestType::GET).process<SendContext>();

    route("/{str}/context", RequestType::GET)
            .process(
                    [](const std::string& title, const Request&, Response& response, Context& context)
                    {
                        response.send(title + ": " + std::to_string(context.counter));
                    });

    route("/{str}?/optional-context", RequestType::GET)
        .process(
                [](const std::optional<std::string>& title, const Request&, Response& response, Context& context)
                {
                    response.send(title.value_or("no-title") + ": " + std::to_string(context.counter));
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

    processRequest("/optional-chapter/test/page123");
    checkResponse("Chapter: test, page[123]");

    processRequest("/optional-chapter/page123");
    checkResponse("Chapter: no-chapter, page[123]");

    processRequest("/optional-chapter/test/page");
    checkResponse("Chapter: test, page[0]");

    processRequest("/optional-chapter/page");
    checkResponse("Chapter: no-chapter, page[0]");

    processRequest("/optional-chapter2/test/123/");
    checkResponse("Chapter: test, page[123]");

    processRequest("/optional-chapter2/123/");
    checkResponse("Chapter: 123, page[0]");

    processRequest("/optional-chapter2/test/");
    checkResponse("Chapter: test, page[0]");

    processRequest("/optional-chapter2/");
    checkResponse("Chapter: no-chapter, page[0]");

    processRequest("/book-Hello_world/chapter/test/page/123");
    checkResponse("Book: Hello_world, Chapter: test, page[123]");

    processRequest("/book-Hello_world/chapter/test/");
    checkResponse("ROUTE_PARAM_ERROR: PARAM COUNT MISMATCH, EXPECTED:3 ACTUAL:2");

    processRequest("/book/Hello/");
    checkResponse("Book: Hello");

    processRequest("/book/Hello/world/");
    checkResponse("Book: Hello#world");

    processRequest("/no_capture_groups");
    checkResponse("Book: ");
    processRequest("/no_capture_groups2");
    checkResponse("Book: ");

    processRequest("/chapter_test");
    checkResponse("Chapter: test");

    processRequest("/chapter_test/");
    checkResponse("Chapter: test");

    processRequest("/files/test.xml");
    checkResponse("XML file: test");

    processRequest("/files/test.xml1");
    checkResponse("404");

    processRequest("/files-optional/test.xml");
    checkResponse("file: test.xml");

    processRequest("/files-optional/");
    checkResponse("file: empty");

    processRequest("/foo");
    checkResponse("404");

    processRequest("/context");
    checkResponse("1");

    processRequest("/context2");
    checkResponse("1");

    processRequest("/test/context");
    checkResponse("test: 1");

    processRequest("/test/optional-context");
    checkResponse("test: 1");

    processRequest("/optional-context");
    checkResponse("no-title: 1");
}

#if (defined(_MSVC_LANG) && _MSVC_LANG >= 202002L) || (!defined(_MSVC_LANG) && __cplusplus >= 202002L)

TEST_F(Router, MatchingRegex_CPP20)
{
    routeRegex<".+">().process<IncrementContext>();
    route("/", RequestType::GET).process<GreeterPageProcessor>();
    route("/moon", RequestType::GET).process<GreeterPageProcessor>("Moon");
    route("/page0", RequestType::GET)
            .process(
                    [](const Request&, Response& response)
                    {
                        response.send("Default page");
                    });
    routeRegex<R"(/page\d*)">(RequestType::GET)
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
    routeRegex<R"(/chapter/(.+)/page(\d+)/)">(RequestType::GET).process<ChapterNamePageIndexProcessor>();
    routeRegex<R"(/chapter_(.+)/page_(\d+)/)">(RequestType::GET)
            .process<ChapterNamePageIndexProcessor>("TestBook");
    routeRegex<R"(/optional-chapter/(.+)?/page(\d+)?/)">(RequestType::GET).process<OptionalChapterNamePageIndexProcessor>();
    routeRegex<R"(/optional-chapter2(?:/(\w+))?(?:/(\d+))?/)">(RequestType::GET).process<OptionalChapterNamePageIndexProcessor>();
    routeRegex<R"(/book-(.+)/chapter/(.+)/page/(\d+)/)">(RequestType::GET).process<BookProcessor>();
    routeRegex<R"(/book-(.+)/chapter/(.+)/)">(RequestType::GET).process<BookProcessor>();
    routeRegex<R"(/book/(\w+))">(RequestType::GET).process<BookProcessorForAnyParams>();
    routeRegex<R"(/book/(\w+)/(\w+)/)">(RequestType::GET).process<BookProcessorForAnyParams>();
    routeRegex<R"(/no_capture_groups)">(RequestType::GET).process<BookProcessorForAnyParams>();
    route("/no_capture_groups2", RequestType::GET).process<BookProcessorForAnyParams>();
    auto parametrizedProcessor = ChapterNameProcessor{};
    routeRegex<R"(/chapter_(.+)/)">(RequestType::GET).process(parametrizedProcessor);
    route("/param_error").process(parametrizedProcessor);
    routeRegex<R"(/files/(.*\.xml))">(RequestType::GET)
            .process(
                    [](const std::string& fileName, const Request&, Response& response)
                    {
                        auto fileContent = std::string{"XML file: " + fileName};
                        response.send(fileContent);
                    });
    routeRegex<R"(/files-optional/(.*\.xml)?)">(RequestType::GET)
    .process(
            [](const std::optional<std::string>& fileName, const Request&, Response& response)
            {
                auto fileContent = std::string{"file: " + fileName.value_or("empty")};
                response.send(fileContent);
            });
    route("/context", RequestType::GET)
            .process(
                    [](const Request&, Response& response, Context& context)
                    {
                        response.send(std::to_string(context.counter));
                    });
    route("/context2", RequestType::GET).process<SendContext>();

    routeRegex<"/(.+)/context">(RequestType::GET)
            .process(
                    [](const std::string& title, const Request&, Response& response, Context& context)
                    {
                        response.send(title + ": " + std::to_string(context.counter));
                    });
    routeRegex("(?:/(.+))?/optional-context", RequestType::GET)
        .process(
                [](const std::optional<std::string>& title, const Request&, Response& response, Context& context)
                {
                    response.send(title.value_or("no-title") + ": " + std::to_string(context.counter));
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

    processRequest("/book-Hello_world/chapter/test/page/123");
    checkResponse("Book: Hello_world, Chapter: test, page[123]");

    processRequest("/book-Hello_world/chapter/test/");
    checkResponse("ROUTE_PARAM_ERROR: PARAM COUNT MISMATCH, EXPECTED:3 ACTUAL:2");

    processRequest("/book/Hello/");
    checkResponse("Book: Hello");

    processRequest("/book/Hello/world/");
    checkResponse("Book: Hello#world");

    processRequest("/no_capture_groups");
    checkResponse("Book: ");
    processRequest("/no_capture_groups2");
    checkResponse("Book: ");

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

    processRequest("/files-optional/test.xml");
    checkResponse("file: test.xml");

    processRequest("/files-optional/");
    checkResponse("file: empty");

    processRequest("/foo");
    checkResponse("404");

    processRequest("/context");
    checkResponse("1");

    processRequest("/context2");
    checkResponse("1");

    processRequest("/test/context");
    checkResponse("test: 1");

    processRequest("/test/optional-context");
    checkResponse("test: 1");

    processRequest("/optional-context");
    checkResponse("no-title: 1");

    processRequest("/optional-chapter/test/page123");
    checkResponse("Chapter: test, page[123]");

    processRequest("/optional-chapter/test/page");
    checkResponse("Chapter: test, page[0]");

    processRequest("/optional-chapter2/test/123/");
    checkResponse("Chapter: test, page[123]");

    processRequest("/optional-chapter2/123/");
    checkResponse("Chapter: 123, page[0]");

    processRequest("/optional-chapter2/test/");
    checkResponse("Chapter: test, page[0]");

    processRequest("/optional-chapter2/");
    checkResponse("Chapter: no-chapter, page[0]");
}

namespace whaleroute::config {
template<>
struct RouteParam<routeParamId("chapter_str")>
{
    using type = ChapterString;
    inline static std::string_view name = "chapter_str";
    inline static std::string_view regex = R"(\w+)";
};
}


TEST_F(Router, Matching_Path_CPP20)
{
    route<"{path}">().process<IncrementContext>();
    route<"/">(RequestType::GET).process<GreeterPageProcessor>();
    route<"/moon">(RequestType::GET).process<GreeterPageProcessor>("Moon");
    route<"/page0">(RequestType::GET)
            .process(
                    [](const Request&, Response& response)
                    {
                        response.send("Default page");
                    });
    routeRegex({R"(/page\d*)"}, RequestType::GET)
            .process(
                    [](const Request&, Response& response)
                    {
                        response.send("Some page");
                    });
    route<"/upload">(RequestType::POST)
            .process(
                    [](const Request&, Response& response)
                    {
                        response.send("OK");
                    });
    route<"/chapter/{str}/page{int}/">(RequestType::GET).process<ChapterNamePageIndexProcessor>();
    route<"/optional-chapter/{str}?/page{int}?/">(RequestType::GET).process<OptionalChapterNamePageIndexProcessor>();
    route<"/optional-chapter2/{str}?/{int}?/">(RequestType::GET).process<OptionalChapterNamePageIndexProcessor>();
    route<"/chapter_{str}/page_{int}/">(RequestType::GET).process<ChapterNamePageIndexProcessor>("TestBook");
    route<"/book-{str}/chapter/{str}/page/{int}">(RequestType::GET).process<BookProcessor>();
    route<"/book-{str}/chapter/{str}/">(RequestType::GET).process<BookProcessor>();
    route<"/book/{str}/">(RequestType::GET).process<BookProcessorForAnyParams>();
    route<"/book/{str}/{str}/">(RequestType::GET).process<BookProcessorForAnyParams>();
    route<"/no_capture_groups">(RequestType::GET).process<BookProcessorForAnyParams>();
    route<"/no_capture_groups2">(RequestType::GET).process<BookProcessorForAnyParams>();
    auto parametrizedProcessor = ChapterNameProcessor{};
    route<"/chapter_{chapter_str}/">(RequestType::GET).process(parametrizedProcessor);
    route<"/files/{str}.xml">(RequestType::GET)
            .process(
                    [](const std::string& fileName, const Request&, Response& response)
                    {
                        auto fileContent = std::string{"XML file: " + fileName};
                        response.send(fileContent);
                    });
    route<"/files-optional/{str}?">(RequestType::GET)
            .process(
                    [](const std::optional<std::string>& fileName, const Request&, Response& response)
                    {
                        auto fileContent = std::string{"file: " + fileName.value_or("empty")};
                        response.send(fileContent);
                    });

    route<"/context">(RequestType::GET)
            .process(
                    [](const Request&, Response& response, Context& context)
                    {
                        response.send(std::to_string(context.counter));
                    });
    route<"/context2">(RequestType::GET).process<SendContext>();

    route<"/{str}/context">(RequestType::GET)
            .process(
                    [](const std::string& title, const Request&, Response& response, Context& context)
                    {
                        response.send(title + ": " + std::to_string(context.counter));
                    });
    route<"/{str}?/optional-context">(RequestType::GET)
            .process(
                    [](const std::optional<std::string>& title, const Request&, Response& response, Context& context)
                    {
                        response.send(title.value_or("no-title") + ": " + std::to_string(context.counter));
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

    processRequest("/optional-chapter/test/page123");
    checkResponse("Chapter: test, page[123]");

    processRequest("/optional-chapter/page123");
    checkResponse("Chapter: no-chapter, page[123]");

    processRequest("/optional-chapter/test/page");
    checkResponse("Chapter: test, page[0]");

    processRequest("/optional-chapter/page");
    checkResponse("Chapter: no-chapter, page[0]");

    processRequest("/optional-chapter2/test/123/");
    checkResponse("Chapter: test, page[123]");

    processRequest("/optional-chapter2/123/");
    checkResponse("Chapter: 123, page[0]");

    processRequest("/optional-chapter2/test/");
    checkResponse("Chapter: test, page[0]");

    processRequest("/optional-chapter2/");
    checkResponse("Chapter: no-chapter, page[0]");

    processRequest("/chapter_test/page_123");
    checkResponse("TestBook Chapter: test, page[123]");

    processRequest("/book-Hello_world/chapter/test/page/123");
    checkResponse("Book: Hello_world, Chapter: test, page[123]");

    processRequest("/book-Hello_world/chapter/test/");
    checkResponse("ROUTE_PARAM_ERROR: PARAM COUNT MISMATCH, EXPECTED:3 ACTUAL:2");

    processRequest("/book/Hello/");
    checkResponse("Book: Hello");

    processRequest("/book/Hello/world/");
    checkResponse("Book: Hello#world");

    processRequest("/no_capture_groups");
    checkResponse("Book: ");
    processRequest("/no_capture_groups2");
    checkResponse("Book: ");

    processRequest("/chapter_test");
    checkResponse("Chapter: test");

    processRequest("/chapter_test/");
    checkResponse("Chapter: test");

    processRequest("/files/test.xml");
    checkResponse("XML file: test");

    processRequest("/files-optional/test.xml");
    checkResponse("file: test.xml");

    processRequest("/files-optional/");
    checkResponse("file: empty");

    processRequest("/files/test.xml1");
    checkResponse("404");

    processRequest("/foo");
    checkResponse("404");

    processRequest("/context");
    checkResponse("1");

    processRequest("/context2");
    checkResponse("1");

    processRequest("/test/context");
    checkResponse("test: 1");

    processRequest("/test/optional-context");
    checkResponse("test: 1");

    processRequest("/optional-context");
    checkResponse("no-title: 1");
}

#endif

TEST_F(Router, RequestProcessorWithoutResponse)
{
    auto noResponseRequestProcessor = NoResponseRequestProcessor{};
    auto parametrizedNoResponseRequestProcessor = ParametrizedNoResponseRequestProcessor{};
    route("/context/").process<AltIncrementContext>();
    auto paramRequestProcessor = ParametrizedAltIncrementContext{};
    routeRegex("/context/param/(\\d+)").process(paramRequestProcessor);
    route("/test").process(noResponseRequestProcessor);
    routeRegex(R"(/test/(\d+)/)").process(parametrizedNoResponseRequestProcessor);
    routeRegex(R"(/context/.*)", RequestType::GET)
            .process(
                    [](const Request&, Response& response, Context& context)
                    {
                        response.send(std::to_string(context.counter));
                    });
    processRequest("/context/");
    checkResponse("1");
    processRequest("/test");
    EXPECT_EQ(noResponseRequestProcessor.testNumber, 1);
    processRequest("/test/3");
    EXPECT_EQ(parametrizedNoResponseRequestProcessor.testNumber, 3);
    processRequest("/context/param/2");
    checkResponse("2");
}

TEST_F(Router, DefaultUnmatchedRequestHandler)
{
    route("/", RequestType::GET).set("Hello world");
    processRequest("/foo");
    checkResponse("NO_MATCH");
    processRequest("/foo", RequestType::POST);
    checkResponse("NO_MATCH");
}

TEST_F(Router, MultipleRoutesMatching)
{
    routeRegex({"/greet/.*"}, RequestType::GET)
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
    routeRegex({"/thank/.*"}, RequestType::GET)
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

    route("/", RequestType::GET).set("Hello Bill");
    route().set("404");

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

    void operator()(const Request&, Response& response, Context&)
    {
        state_ = ++counter;
        response.send("TEST");
    }

private:
    int counter = 0;
    int& state_;
};

TEST_F(Router, SameProcessorObjectUsedInMultipleRoutes)
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

TEST_F(Router, SameProcessorTypeCreatedInMultipleRoutes)
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

    void operator()(const std::string& param, const Request&, Response& response, Context&)
    {
        state_ = ++counter;
        response.send("TEST " + param);
    }

private:
    int counter = 0;
    int& state_;
};

TEST_F(Router, SameParametrizedProcessorObjectUsedInMultipleRoutes)
{
    int state = 0;
    auto counterProcessor = ParametrizedCounterRouteProcessor{state};
    routeRegex({"/test/(.+)"}, RequestType::GET).process(counterProcessor);
    routeRegex({"/test2/(.+)"}, RequestType::GET).process(counterProcessor);

    processRequest("/test/foo");
    checkResponse("TEST foo");
    processRequest("/test2/bar");
    checkResponse("TEST bar");
    ASSERT_EQ(state, 2); // Which means that routes contain the same processor object
}

TEST_F(Router, SameParametrizedProcessorTypeCreatedInMultipleRoutes)
{
    int state = 0;
    routeRegex({"/test/(.+)"}, RequestType::GET).process<ParametrizedCounterRouteProcessor>(state);
    routeRegex({"/test2/(.+)"}, RequestType::GET).process<ParametrizedCounterRouteProcessor>(state);

    processRequest("/test/foo");
    checkResponse("TEST foo");
    processRequest("/test2/bar");
    checkResponse("TEST bar");
    ASSERT_EQ(state, 1); // Which means that routes contain different processor objects
}