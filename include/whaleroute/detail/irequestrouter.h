#pragma once

namespace whaleroute::detail {

template<typename TRequestProcessor, typename TRequest, typename TRequestType, typename TResponse, typename TResponseValue>
class IRequestRouter{
public:
    virtual void callRequestProcessor(TRequestProcessor&, const TRequest&, TResponse&) = 0;
    virtual std::string getRequestPath(const TRequest&) = 0;
    virtual TRequestType getRequestType(const TRequest&) = 0;
    virtual void processUnmatchedRequest(const TRequest&, TResponse&) = 0;
    virtual void setResponse(TResponse&, const TResponseValue&) = 0;

};

template<typename TRequestProcessor, typename TRequest, typename TRequestType, typename TResponse, typename TResponseValue>
class IRequestRouterWithoutRequestType{
public:
    virtual void callRequestProcessor(TRequestProcessor&, const TRequest&, TResponse&) = 0;
    virtual std::string getRequestPath(const TRequest&) = 0;
    virtual TRequestType getRequestType(const TRequest&) final
    {
        return TRequestType{};
    };
    virtual void processUnmatchedRequest(const TRequest&, TResponse&) = 0;
    virtual void setResponse(TResponse&, const TResponseValue&) = 0;

};

template<typename TRequestProcessor, typename TRequest, typename TRequestType, typename TResponse, typename TResponseValue>
class IRequestRouterWithoutResponseSetter : public IRequestRouter<TRequestProcessor, TRequest, TRequestType, TResponse, TResponseValue> {
public:
    virtual void callRequestProcessor(TRequestProcessor&, const TRequest&, TResponse&) = 0;
    virtual std::string getRequestPath(const TRequest&) = 0;
    virtual TRequestType getRequestType(const TRequest&) = 0;
    virtual void processUnmatchedRequest(const TRequest&, TResponse&) = 0;
    virtual void setResponse(TResponse&, const TResponseValue&) final
    {}
};

template<typename TRequestProcessor, typename TRequest, typename TRequestType, typename TResponse, typename TResponseValue>
class IRequestRouterWithoutResponseSetterAndRequestType : public IRequestRouter<TRequestProcessor, TRequest, TRequestType, TResponse, TResponseValue> {
public:
    virtual void callRequestProcessor(TRequestProcessor&, const TRequest&, TResponse&) = 0;
    virtual std::string getRequestPath(const TRequest&) = 0;
    virtual TRequestType getRequestType(const TRequest&) final
    {
        return TRequestType{};
    }
    virtual void processUnmatchedRequest(const TRequest&, TResponse&) = 0;
    virtual void setResponse(TResponse&, const TResponseValue&) final
    {}
};

template<typename TRequestProcessor, typename TRequest, typename TRequestType, typename TResponse, typename TResponseValue>
class IRequestRouterWithoutRequestProcessor : public IRequestRouter<TRequestProcessor, TRequest, TRequestType, TResponse, TResponseValue> {
public:
    virtual void callRequestProcessor(TRequestProcessor&, const TRequest&, TResponse&) final
    {}
    virtual std::string getRequestPath(const TRequest&) = 0;
    virtual TRequestType getRequestType(const TRequest&) = 0;
    virtual void processUnmatchedRequest(const TRequest&, TResponse&) = 0;
    virtual void setResponse(TResponse&, const TResponseValue&) = 0;
};

template<typename TRequestProcessor, typename TRequest, typename TRequestType, typename TResponse, typename TResponseValue>
class IRequestRouterWithoutRequestProcessorAndRequestType : public IRequestRouter<TRequestProcessor, TRequest, TRequestType, TResponse, TResponseValue> {
public:
    virtual void callRequestProcessor(TRequestProcessor&, const TRequest&, TResponse&) final
    {}
    virtual std::string getRequestPath(const TRequest&) = 0;
    virtual TRequestType getRequestType(const TRequest&) final
    {
        return TRequestType{};
    }
    virtual void processUnmatchedRequest(const TRequest&, TResponse&) = 0;
    virtual void setResponse(TResponse&, const TResponseValue&) = 0;
};

template<typename TRequestProcessor, typename TRequest, typename TRequestType, typename TResponse, typename TResponseValue>
class IRequestRouterWithoutRequestProcessorAndResponseSetter : public IRequestRouter<TRequestProcessor, TRequest, TRequestType, TResponse, TResponseValue> {
public:
    virtual void callRequestProcessor(TRequestProcessor&, const TRequest&, TResponse&) final
    {}
    virtual std::string getRequestPath(const TRequest&) = 0;
    virtual TRequestType getRequestType(const TRequest&) = 0;
    virtual void processUnmatchedRequest(const TRequest&, TResponse&) = 0;
    virtual void setResponse(TResponse&, const TResponseValue&) final
    {}
};

template<typename TRequestProcessor, typename TRequest, typename TRequestType, typename TResponse, typename TResponseValue>
class IRequestRouterWithoutRequestProcessorAndResponseSetterAndRequestType : public IRequestRouter<TRequestProcessor, TRequest, TRequestType, TResponse, TResponseValue> {
public:
    virtual void callRequestProcessor(TRequestProcessor&, const TRequest&, TResponse&) final
    {}
    virtual std::string getRequestPath(const TRequest&) = 0;
    virtual TRequestType getRequestType(const TRequest&) final
    {
        return TRequestType{};
    }
    virtual void processUnmatchedRequest(const TRequest&, TResponse&) = 0;
    virtual void setResponse(TResponse&, const TResponseValue&) final
    {}
};


}
