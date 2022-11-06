//#include "common.h"
//#include <whaleroute/requestrouter.h>
//#include <gtest/gtest.h>
//#include <algorithm>
//
//namespace whaleroute::config {
//template<typename TRequest, typename TResponse>
//struct RouteSpecification<RequestType, TRequest, TResponse> {
//    bool operator()(RequestType value, const TRequest& request, TResponse&) const
//    {
//        return value == request.type;
//    }
//};
//}
//
//namespace response_value_arguments {
//
//class TestString{
//public:
//    enum class CaseMode{Original, UpperCase};
//    TestString(std::string value, CaseMode caseMode = CaseMode::Original)
//    : value_{std::move(value)}
//    {
//        if (caseMode == CaseMode::UpperCase)
//            std::transform(value_.cbegin(), value_.cend(), value_.begin(), [](unsigned char ch){
//                return static_cast<char>(std::toupper(ch));
//            });
//    }
//    TestString(const char* value, CaseMode caseMode = CaseMode::Original)
//    : TestString{std::string{value}, caseMode}
//    {
//
//    }
//    std::string value() const
//    {
//        return value_;
//    }
//
//private:
//    std::string value_;
//};
//
//class ResponseValueFromArgs : public ::testing::Test,
//                               public whaleroute::RequestRouter<Request, Response, TestString> {
//public:
//    void processRequest(RequestType type, const std::string& path)
//    {
//        auto response = Response{};
//        response.init();
//        process(Request{type, path, {}}, response);
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
//        response.state->data = "NO_MATCH";
//    }
//
//    void setResponseValue(Response& response, const TestString& value) final
//    {
//        response.state->data = value.value();
//    }
//
//protected:
//    std::string responseData_;
//};
//
//TEST_F(ResponseValueFromArgs, Default)
//{
//    route("/", RequestType::GET).set("Hello world");
//    route().set("Not found", TestString::CaseMode::UpperCase);
//
//    processRequest(RequestType::GET, "/");
//    checkResponse("Hello world");
//
//    processRequest(RequestType::GET, "/foo");
//    checkResponse("NOT FOUND");
//}
//
//}