#ifndef WHALEROUTE_REQUESTPROCESSORQUEUE_H
#define WHALEROUTE_REQUESTPROCESSORQUEUE_H

#include <functional>
#include <optional>
#include <vector>

namespace whaleroute {

class RequestProcessorQueue {
public:
    explicit RequestProcessorQueue(std::vector<std::function<bool()>> requestProcessorInvokers)
        : requestProcessorInvokers_{std::move(requestProcessorInvokers)}
    {
    }
    RequestProcessorQueue() = default;

    void launch()
    {
        isStopped_ = false;
        for (; currentIndex_ < requestProcessorInvokers_.size(); ++currentIndex_) {
            if (isStopped_)
                break;
            auto result = requestProcessorInvokers_.at(currentIndex_)();
            if (!result) {
                currentIndex_ = requestProcessorInvokers_.size() + 1;
                break;
            }
        }
    }

    void stop()
    {
        isStopped_ = true;
    }

private:
    std::size_t currentIndex_ = 0;
    bool isStopped_ = false;
    std::vector<std::function<bool()>> requestProcessorInvokers_;
};

} // namespace whaleroute

#endif // WHALEROUTE_REQUESTPROCESSORQUEUE_H