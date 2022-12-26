#ifndef WHALEROUTE_REQUESTPROCESSORQUEUE_H
#define WHALEROUTE_REQUESTPROCESSORQUEUE_H

#include <whaleroute/irequestprocessorqueue.h>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace whaleroute::detail {

template <typename TRouteContext>
class RequestProcessorQueue : public IRequestProcessorQueue {
public:
    explicit RequestProcessorQueue(std::vector<std::function<bool(TRouteContext&)>> requestProcessorInvokers)
        : requestProcessorInvokers_{std::move(requestProcessorInvokers)}
        , routeContext_{std::make_shared<TRouteContext>()}
    {
    }
    RequestProcessorQueue() = default;

    void launch() override
    {
        isStopped_ = false;
        for (; currentIndex_ < requestProcessorInvokers_.size(); ++currentIndex_) {
            if (isStopped_)
                break;
            auto result = requestProcessorInvokers_.at(currentIndex_)(*routeContext_);
            if (!result) {
                currentIndex_ = requestProcessorInvokers_.size() + 1;
                break;
            }
        }
    }

    void stop() override
    {
        isStopped_ = true;
    }

private:
    std::size_t currentIndex_ = 0;
    bool isStopped_ = false;
    std::vector<std::function<bool(TRouteContext&)>> requestProcessorInvokers_;
    std::shared_ptr<TRouteContext> routeContext_;
};

} // namespace whaleroute

#endif // WHALEROUTE_REQUESTPROCESSORQUEUE_H