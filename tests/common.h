#pragma once
#include <string>

struct TestState{
    bool activated = false;
};

enum class RequestMethod{
    GET,
    POST
};
struct Request{
    RequestMethod method;
    std::string requestPath;
    std::string name;
};
struct Response{
    std::string data;
};
struct ResponseValue{
    std::string data;
};

class RequestProcessor{
public:
    virtual void process(const Request&, Response&){}
};
