#pragma once

namespace whaleroute::detail {

template<typename TRequest, typename TResponse, typename TRequestType, typename TRequestProcessor, typename TResponseValue>
class IRequestRouter{
public:
    virtual void callRequestProcessor(TRequestProcessor&, const TRequest&, TResponse&) = 0;
    virtual std::string getRequestPath(const TRequest&) = 0;
    virtual TRequestType getRequestType(const TRequest&) = 0;
    virtual void processUnmatchedRequest(const TRequest&, TResponse&) = 0;
    virtual void setResponse(TResponse&, const TResponseValue&) = 0;

};

template<typename TRequest, typename TResponse, typename TRequestType, typename TRequestProcessor, typename TResponseValue>
class IRequestRouterWithoutRequestType : public IRequestRouter<TRequest, TResponse, TRequestType, TRequestProcessor, TResponseValue>{
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

template<typename TRequest, typename TResponse, typename TRequestType, typename TRequestProcessor, typename TResponseValue>
class IRequestRouterWithoutResponseSetter : public IRequestRouter<TRequest, TResponse, TRequestType, TRequestProcessor, TResponseValue> {
public:
    virtual void callRequestProcessor(TRequestProcessor&, const TRequest&, TResponse&) = 0;
    virtual std::string getRequestPath(const TRequest&) = 0;
    virtual TRequestType getRequestType(const TRequest&) = 0;
    virtual void processUnmatchedRequest(const TRequest&, TResponse&) = 0;
    virtual void setResponse(TResponse&, const TResponseValue&) final
    {}
};

template<typename TRequest, typename TResponse, typename TRequestType, typename TRequestProcessor, typename TResponseValue>
class IRequestRouterWithoutResponseSetterAndRequestType : public IRequestRouter<TRequest, TResponse, TRequestType, TRequestProcessor, TResponseValue> {
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

template<typename TRequest, typename TResponse, typename TRequestType, typename TRequestProcessor, typename TResponseValue>
class IRequestRouterWithoutRequestProcessor : public IRequestRouter<TRequest, TResponse, TRequestType, TRequestProcessor, TResponseValue> {
public:
    virtual void callRequestProcessor(TRequestProcessor&, const TRequest&, TResponse&) final
    {}
    virtual std::string getRequestPath(const TRequest&) = 0;
    virtual TRequestType getRequestType(const TRequest&) = 0;
    virtual void processUnmatchedRequest(const TRequest&, TResponse&) = 0;
    virtual void setResponse(TResponse&, const TResponseValue&) = 0;
};

template<typename TRequest, typename TResponse, typename TRequestType, typename TRequestProcessor, typename TResponseValue>
class IRequestRouterWithoutRequestProcessorAndRequestType : public IRequestRouter<TRequest, TResponse, TRequestType, TRequestProcessor, TResponseValue> {
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

template<typename TRequest, typename TResponse, typename TRequestType, typename TRequestProcessor, typename TResponseValue>
class IRequestRouterWithoutRequestProcessorAndResponseSetter : public IRequestRouter<TRequest, TResponse, TRequestType, TRequestProcessor, TResponseValue> {
public:
    virtual void callRequestProcessor(TRequestProcessor&, const TRequest&, TResponse&) final
    {}
    virtual std::string getRequestPath(const TRequest&) = 0;
    virtual TRequestType getRequestType(const TRequest&) = 0;
    virtual void processUnmatchedRequest(const TRequest&, TResponse&) = 0;
    virtual void setResponse(TResponse&, const TResponseValue&) final
    {}
};

template<typename TRequest, typename TResponse, typename TRequestType, typename TRequestProcessor, typename TResponseValue>
class IRequestRouterWithoutRequestProcessorAndResponseSetterAndRequestType : public IRequestRouter<TRequest, TResponse, TRequestType, TRequestProcessor, TResponseValue> {
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
