#pragma once
#include <vector>
#include <string>

namespace whaleroute::detail{

template<typename TRequest, typename TResponse>
class IRequestProcessor {
public:
    IRequestProcessor() = default;
    virtual ~IRequestProcessor() = default;
    IRequestProcessor(const IRequestProcessor&) = delete;
    IRequestProcessor& operator=(const IRequestProcessor&) = delete;
    IRequestProcessor(IRequestProcessor&&) = delete;
    IRequestProcessor&& operator=(IRequestProcessor&&) = delete;

    virtual void processRouterRequest(const TRequest&, TResponse&, const std::vector<std::string>&) = 0;
};

}