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
//
//template<typename TRequest, typename TResponse>
//struct RouteSpecification<std::string, TRequest, TResponse> {
//    bool operator()(const std::string& value, const TRequest& request, TResponse&) const
//    {
//        return value == request.name;
//    }
//};
//
//}
//
//namespace multiple_route_specifiers {
//
//class MultipleRouteSpecifiers : public ::testing::Test,
//                                public whaleroute::RequestRouter<Request, Response, std::string> {
//public:
//    void processRequest(const std::string& path, RequestType type, std::string name = {})
//    {
//        auto response = Response{};
//        response.init();
//        process(Request{type, path, std::move(name)}, response);
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
//    void setResponseValue(Response& response, const std::string& value) final
//    {
//        response.state->data = value;
//    }
//
//protected:
//    std::string responseData_;
//};
//
//using namespace std::string_literals;
//
//TEST_F(MultipleRouteSpecifiers, Default)
//{
//    route("/", RequestType::GET).set("Hello world");
//    route("/moon", RequestType::GET, "Sender"s).set("Hello moon from sender");
//    route().set("404");
//
//    processRequest("/", RequestType::GET);
//    checkResponse("Hello world");
//
//    processRequest("/moon", RequestType::GET);
//    checkResponse("404");
//
//    processRequest("/moon", RequestType::GET, "Sender");
//    checkResponse("Hello moon from sender");
//
//    processRequest("/foo", RequestType::GET);
//    checkResponse("404");
//}
//
//}