#pragma once
#include <string>
#include <memory>
#include <vector>

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
    void init()
    {
        state = std::make_shared<State>();
    }
    struct State {
        std::string data;
        bool wasSent = false;
    };
    std::shared_ptr<State> state;
    std::vector<std::string> routeParams;
};
struct ResponseValue{
    std::string data;
};

class RequestProcessor{
public:
    virtual ~RequestProcessor() = default;
    virtual void process(const Request&, Response&){}
};
