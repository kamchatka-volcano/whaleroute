//#include "common.h"
//#include <whaleroute/requestrouter.h>
//#include <gtest/gtest.h>
//#include <sstream>
//
//struct ChapterString{
//    std::string value;
//};
//
//namespace whaleroute::config{
//template<typename TRequest, typename TResponse>
//struct RouteSpecification<RequestType, TRequest, TResponse> {
//    bool operator()(RequestType value, const TRequest& request, TResponse&) const
//    {
//        return value == request.type;
//    }
//};
//
//template<>
//struct StringConverter<ChapterString>{
//    static std::optional<ChapterString> fromString(const std::string& data)
//    {
//        return ChapterString{data};
//    }
//};
//}
//
//namespace alt_regex_specifying{
//
//class AltRegexSpecifying : public ::testing::Test,
//                           public whaleroute::RequestRouter<Request, Response, std::string> {
//public:
//    void processRequest(const std::string& path, RequestType requestType = RequestType::GET, std::string name = {})
//    {
//        auto response = Response{};
//        response.init();
//        process(Request{requestType, path, std::move(name)}, response);
//        responseData_ = response.state->data;
//    }
//
//    void checkResponse(const std::string& expectedResponseData)
//    {
//        EXPECT_EQ(responseData_, expectedResponseData);
//    }
//
//protected:
//    std::string getRequestPath(const Request& request) final
//    {
//        return request.requestPath;
//    }
//
//    void processUnmatchedRequest(const Request&, Response& response) final
//    {
//        response.send("NO_MATCH");
//    }
//
//    void setResponseValue(Response& response, const std::string& value) final
//    {
//        response.send(value);
//    }
//
//    bool isRouteProcessingFinished(const Request&, Response& response) const final
//    {
//        return response.state->wasSent;
//    }
//
//protected:
//    std::string responseData_;
//};
//
//class GreeterPageProcessor : public whaleroute::RequestProcessor<Request, Response>
//{
//public:
//    GreeterPageProcessor(std::string name = {})
//        : name_(std::move(name))
//    {}
//
//private:
//    void process(const Request&, Response& response) override
//    {
//        response.send("Hello " + (name_.empty() ? "world" : name_));
//    }
//    std::string name_;
//};
//
//class ChapterNamePageIndexProcessor : public whaleroute::RequestProcessor<Request, Response, std::string, int>
//{
//public:
//    ChapterNamePageIndexProcessor(std::string title = {})
//        : title_{std::move(title)}
//    {}
//
//private:
//    void process(const std::string& chapterName, const int& pageIndex, const Request&, Response& response) override
//    {
//        response.send(title_ + (title_.empty() ? "" : " ") + "Chapter: " + chapterName + ", page[" + std::to_string(pageIndex) + "]");
//    }
//    void onRouteParametersError(const Request&, Response& response, const whaleroute::RouteParameterError& error) override
//    {
//        response.send(getRouteParamErrorInfo(error));
//    }
//    std::string title_;
//};
//
//class ChapterNameProcessor : public whaleroute::RequestProcessor<Request, Response, ChapterString>
//{
//    void process(const ChapterString& chapterName, const Request&, Response& response) override
//    {
//        response.send("Chapter: " + chapterName.value);
//    }
//    void onRouteParametersError(const Request&, Response& response, const whaleroute::RouteParameterError& error) override
//    {
//        response.send(getRouteParamErrorInfo(error));
//    }
//};
//
//
//TEST_F(AltRegexSpecifying, StringLiterals)
//{
//    using namespace whaleroute::string_literals;
//
//    route("/", RequestType::GET).process<GreeterPageProcessor>();
//    route("/moon", RequestType::GET).process<GreeterPageProcessor>("Moon");
//    route("/page0", RequestType::GET).process([](auto&, auto& response){ response.send("Default page");});
//    route(R"(/page\d*)"_rx, RequestType::GET).process([](auto&, auto& response){ response.send("Some page");});
//    route("/upload", RequestType::POST).process([](auto&, auto& response){ response.send("OK");});
//    route(R"(/chapter/(.+)/page(\d+))"_rx, RequestType::GET).process<ChapterNamePageIndexProcessor>();
//    route(R"(/chapter_(.+)/page_(\d+))"_rx, RequestType::GET).process<ChapterNamePageIndexProcessor>("TestBook");
//    auto parametrizedProcessor = ChapterNameProcessor{};
//    route(R"(/chapter_(.+))"_rx, RequestType::GET).process(parametrizedProcessor);
//    route("/param_error").process(parametrizedProcessor);
//    route(R"(/files/(.*\.xml))"_rx, RequestType::GET).process<std::string>(
//            [](const std::string& fileName, const Request&, Response& response) {
//                auto fileContent = std::string{"XML file: " + fileName};
//                response.send(fileContent);
//            });
//    route().set("404");
//
//    processRequest("/");
//    checkResponse("Hello world");
//    processRequest("/", RequestType::POST);
//    checkResponse("404");
//
//    processRequest("/moon");
//    checkResponse("Hello Moon");
//
//    processRequest("/upload", RequestType::POST);
//    checkResponse("OK");
//    processRequest("/upload", RequestType::GET);
//    checkResponse("404");
//
//    processRequest("/page123");
//    checkResponse("Some page");
//
//    processRequest("/page0");
//    checkResponse("Default page");
//
//    processRequest("/chapter/test/page123");
//    checkResponse("Chapter: test, page[123]");
//
//    processRequest("/chapter_test/page_123");
//    checkResponse("TestBook Chapter: test, page[123]");
//
//    processRequest("/chapter_test");
//    checkResponse("Chapter: test");
//
//    processRequest("/chapter_test/");
//    checkResponse("Chapter: test");
//
//    processRequest("/param_error/");
//    checkResponse("ROUTE_PARAM_ERROR: PARAM COUNT MISMATCH, EXPECTED:1 ACTUAL:0");
//
//    processRequest("/files/test.xml");
//    checkResponse("XML file: test.xml");
//
//    processRequest("/files/test.xml1");
//    checkResponse("404");
//
//    processRequest("/foo");
//    checkResponse("404");
//}
//
//TEST_F(AltRegexSpecifying, TildaEscape)
//{
//    setRegexMode(whaleroute::RegexMode::TildaEscape);
//    route("/", RequestType::GET).process<GreeterPageProcessor>();
//    route("/moon", RequestType::GET).process<GreeterPageProcessor>("Moon");
//    route("/page0", RequestType::GET).process([](auto&, auto& response){ response.send("Default page");});
//    route(whaleroute::rx{"/page~d*"}, RequestType::GET).process([](auto&, auto& response){ response.send("Some page");});
//    route("/upload", RequestType::POST).process([](auto&, auto& response){ response.send("OK");});
//    route(whaleroute::rx{"/chapter/(.+)/page(~d+)"}, RequestType::GET).process<ChapterNamePageIndexProcessor>();
//    route(whaleroute::rx{"/chapter_(.+)/page_(~d+)"}, RequestType::GET).process<ChapterNamePageIndexProcessor>("TestBook");
//    auto parametrizedProcessor = ChapterNameProcessor{};
//    route(whaleroute::rx{"/chapter_(.+)"}, RequestType::GET).process(parametrizedProcessor);
//    route("/param_error").process(parametrizedProcessor);
//    route(whaleroute::rx{"/files/(.*~.xml)"}, RequestType::GET).process<std::string>(
//            [](const std::string& fileName, const Request&, Response& response) {
//                auto fileContent = std::string{"XML file: " + fileName};
//                response.send(fileContent);
//            });
//    route().set("404");
//
//    processRequest("/");
//    checkResponse("Hello world");
//    processRequest("/", RequestType::POST);
//    checkResponse("404");
//
//    processRequest("/moon");
//    checkResponse("Hello Moon");
//
//    processRequest("/upload", RequestType::POST);
//    checkResponse("OK");
//    processRequest("/upload", RequestType::GET);
//    checkResponse("404");
//
//    processRequest("/page123");
//    checkResponse("Some page");
//
//    processRequest("/page0");
//    checkResponse("Default page");
//
//    processRequest("/chapter/test/page123");
//    checkResponse("Chapter: test, page[123]");
//
//    processRequest("/chapter_test/page_123");
//    checkResponse("TestBook Chapter: test, page[123]");
//
//    processRequest("/chapter_test");
//    checkResponse("Chapter: test");
//
//    processRequest("/chapter_test/");
//    checkResponse("Chapter: test");
//
//    processRequest("/param_error/");
//    checkResponse("ROUTE_PARAM_ERROR: PARAM COUNT MISMATCH, EXPECTED:1 ACTUAL:0");
//
//    processRequest("/files/test.xml");
//    checkResponse("XML file: test.xml");
//
//    processRequest("/files/test.xml1");
//    checkResponse("404");
//
//    processRequest("/foo");
//    checkResponse("404");
//}
//
//TEST_F(AltRegexSpecifying, StringLiteralsAndTildaEscape)
//{
//    using namespace whaleroute::string_literals;
//    setRegexMode(whaleroute::RegexMode::TildaEscape);
//
//    route("/", RequestType::GET).process<GreeterPageProcessor>();
//    route("/moon", RequestType::GET).process<GreeterPageProcessor>("Moon");
//    route("/moon~~"_rx, RequestType::GET).process<GreeterPageProcessor>("Moon~");
//    route("/~~moon"_rx, RequestType::GET).process<GreeterPageProcessor>("~Moon");
//    route("/m~~n"_rx, RequestType::GET).process<GreeterPageProcessor>("M~n");
//    route("/~~m~~n~~"_rx, RequestType::GET).process<GreeterPageProcessor>("~M~n~");
//    route("/page0", RequestType::GET).process([](auto&, auto& response){ response.send("Default page");});
//    route("/page~d*"_rx, RequestType::GET).process([](auto&, auto& response){ response.send("Some page");});
//    route("/upload", RequestType::POST).process([](auto&, auto& response){ response.send("OK");});
//    route("/chapter/(.+)/page(~d+)"_rx, RequestType::GET).process<ChapterNamePageIndexProcessor>();
//    route("/chapter_(.+)/page_(~d+)"_rx, RequestType::GET).process<ChapterNamePageIndexProcessor>("TestBook");
//    auto parametrizedProcessor = ChapterNameProcessor{};
//    route("/chapter_(.+)"_rx, RequestType::GET).process(parametrizedProcessor);
//    route("/param_error").process(parametrizedProcessor);
//    route("/files/(.*~.xml)"_rx, RequestType::GET).process<std::string>(
//            [](const std::string& fileName, const Request&, Response& response) {
//                auto fileContent = std::string{"XML file: " + fileName};
//                response.send(fileContent);
//            });
//    route().set("404");
//
//    processRequest("/");
//    checkResponse("Hello world");
//    processRequest("/", RequestType::POST);
//    checkResponse("404");
//
//    processRequest("/moon");
//    checkResponse("Hello Moon");
//
//    processRequest("/moon~");
//    checkResponse("Hello Moon~");
//
//    processRequest("/~moon");
//    checkResponse("Hello ~Moon");
//
//    processRequest("/m~n");
//    checkResponse("Hello M~n");
//
//    processRequest("/~m~n~");
//    checkResponse("Hello ~M~n~");
//
//    processRequest("/upload", RequestType::POST);
//    checkResponse("OK");
//    processRequest("/upload", RequestType::GET);
//    checkResponse("404");
//
//    processRequest("/page123");
//    checkResponse("Some page");
//
//    processRequest("/page0");
//    checkResponse("Default page");
//
//    processRequest("/chapter/test/page123");
//    checkResponse("Chapter: test, page[123]");
//
//    processRequest("/chapter_test/page_123");
//    checkResponse("TestBook Chapter: test, page[123]");
//
//    processRequest("/chapter_test");
//    checkResponse("Chapter: test");
//
//    processRequest("/chapter_test/");
//    checkResponse("Chapter: test");
//
//    processRequest("/param_error/");
//    checkResponse("ROUTE_PARAM_ERROR: PARAM COUNT MISMATCH, EXPECTED:1 ACTUAL:0");
//
//    processRequest("/files/test.xml");
//    checkResponse("XML file: test.xml");
//
//    processRequest("/files/test.xml1");
//    checkResponse("404");
//
//    processRequest("/foo");
//    checkResponse("404");
//}
//
//
//
//
//}