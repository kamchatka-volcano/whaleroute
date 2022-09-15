#ifndef WHALEROUTE_IREQUESTPROCESSOR_H
#define WHALEROUTE_IREQUESTPROCESSOR_H

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

#endif //WHALEROUTE_IREQUESTPROCESSOR_H