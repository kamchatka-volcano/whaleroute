#ifndef WHALEROUTE_IREQUESTPROCESSORQUEUE_H
#define WHALEROUTE_IREQUESTPROCESSORQUEUE_H

namespace whaleroute {

class IRequestProcessorQueue {
public:
    virtual ~IRequestProcessorQueue() = default;
    virtual void launch() = 0;
    virtual void stop() = 0;
};

} // namespace whaleroute

#endif // WHALEROUTE_IREQUESTPROCESSORQUEUE_H
