#ifndef WHALEROUTE_REQUESTPROCESSORQUEUE_H
#define WHALEROUTE_REQUESTPROCESSORQUEUE_H

#include "external/sfun/interface.h"
#include <functional>
#include <memory>
#include <vector>

namespace whaleroute::detail {

class IRequestProcessorQueueImpl : private sfun::Interface<IRequestProcessorQueueImpl> {
public:
    virtual void launch() = 0;
    virtual void stop() = 0;
};

template<typename TRouteContext>
class RequestProcessorQueueImpl : public IRequestProcessorQueueImpl {
public:
    explicit RequestProcessorQueueImpl(std::vector<std::function<bool(TRouteContext&)>> requestProcessorInvokers)
        : requestProcessorInvokers_{std::move(requestProcessorInvokers)}
    {
    }
    RequestProcessorQueueImpl() = default;

    void launch() override
    {
        isStopped_ = false;
        for (; currentIndex_ < requestProcessorInvokers_.size(); ++currentIndex_) {
            if (isStopped_)
                break;
            auto result = requestProcessorInvokers_.at(currentIndex_)(routeContext_);
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
    TRouteContext routeContext_;
};

} //namespace whaleroute::detail

namespace whaleroute {

class RequestProcessorQueue {
public:
    template<typename TRouteContext>
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