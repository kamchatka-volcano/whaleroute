#ifndef WHALEROUTE_REQUESTPROCESSORQUEUE_H
#define WHALEROUTE_REQUESTPROCESSORQUEUE_H

#include "detail/requestprocessorqueue_impl.h"
#include <functional>
#include <memory>
#include <vector>

namespace whaleroute {

class RequestProcessorQueue {
public:
    template <typename TRouteContext>
    explicit RequestProcessorQueue(std::vector<std::function<bool(TRouteContext&)>> requestProcessorInvokers)
        : impl_{std::make_shared<detail::RequestProcessorQueueImpl<TRouteContext>>(requestProcessorInvokers)}
    {
    }
    RequestProcessorQueue() = default;

    void launch()
    {
        if (impl_)
            impl_->launch();
    }

    void stop()
    {
        if (impl_)
            impl_->stop();
    }

private:
    std::shared_ptr<detail::IRequestProcessorQueueImpl> impl_;
};

} // namespace whaleroute

#endif // WHALEROUTE_REQUESTPROCESSORQUEUE_H