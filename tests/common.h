#pragma once
#include <string>

struct TestState{
    bool activated = false;
};

enum class RequestType{
    GET,
    POST
};
struct Request{
    RequestType type;
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
