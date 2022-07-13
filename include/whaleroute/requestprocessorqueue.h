#pragma once
#include <vector>
#include <functional>
#include <optional>

namespace whaleroute{

class RequestProcessorQueue{
public:
    explicit RequestProcessorQueue(std::vector<std::function<std::optional<bool>()>> requestProcessorInvokers,
                                   std::function<void()> unmatchedRequestHandler)
        : requestProcessorInvokers_(std::move(requestProcessorInvokers))
        , unmatchedRequestHandler_(std::move(unmatchedRequestHandler))
    {}
    RequestProcessorQueue() = default;

    void launch()
    {
        isStopped_ = false;
        for (; currentIndex_ < requestProcessorInvokers_.size(); ++currentIndex_){
            if (isStopped_)
                break;
            auto result = requestProcessorInvokers_.at(currentIndex_)();
            if (!result)
                unmatchedRequestHandler_();
            else if (!*result){
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
    std::vector<std::function<std::optional<bool>()>> requestProcessorInvokers_;
    std::function<void()> unmatchedRequestHandler_;
};

}