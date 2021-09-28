#pragma once
#include <map>
#include <memory>
#include <typeinfo>
#include <typeindex>

namespace whaleroute::detail{

template <typename TRequestProcessor>
class RequestProcessorInstancer{
public:
    template <typename TProcessor>
    TRequestProcessor& get()
    {
        static_assert(std::is_base_of_v<TRequestProcessor, TProcessor>, "TProcessor must be a subclass of TRequestProcessor");

        auto processorType = std::type_index{typeid(TProcessor)};
        auto processorIt = storage_.find(processorType);
        if (processorIt == storage_.end()){
            auto processor = std::make_unique<TProcessor>();
            processorIt = storage_.emplace(processorType, std::move(processor)).first;
        }
        return *processorIt->second;
    }

    template <typename TProcessor, typename... TArgs>
    TRequestProcessor& get(TArgs&&... args)
    {
        static_assert(std::is_base_of_v<TRequestProcessor, TProcessor>, "TProcessor must be a subclass of TRequestProcessor");

        auto processorType = std::type_index{typeid(TProcessor)};
        auto processorIt = storage_.find(processorType);
        if (processorIt == storage_.end()){
            auto processor = std::make_unique<TProcessor>(std::forward<TArgs>(args)...);
            processorIt = storage_.emplace(processorType, std::move(processor)).first;
        }
        return *processorIt->second;
    }

private:
    std::map<std::type_index, std::unique_ptr<TRequestProcessor>> storage_;

};

}

