# whaleroute


**whaleroute** - is a C++17 header-only library for request routing. It's designed for binding handlers to HTTP requests, but it can easily be used with other protocols, as the library is implemented as a generic class template.
If your incoming data processing function has a signature like `void(const Request&, Response&)` and you need to perform different actions based on a string value from the request object, **whaleroute** can be a big help.  

### Usage
#### Implementing the router  
Let's say that our Request and Response classes look like this:
```c++
namespace demo{
struct Request{
    std::string uri;
    enum class Method{
        GET,
        POST
    } method;
};
struct Response{
    void send(const std::string& data)
    {
        std::cout << data;
    }
};
```

To create the simplest router we must use Request and Response types as template arguments and implement two virtual functions:
* `virtual std::string getRequestPath(const TRequest&) = 0;`
* `virtual void processUnmatchedRequest(const TRequest&, TResponse&) = 0;`

```c++
#include <whaleroute/requestrouter.h>

namespace demo{
class Router : public whaleroute::RequestRouter<Request, Response>{
    std::string getRequestPath(const TRequest& request) final
    {
        return request.uri;
    }
    
    void processUnmatchedRequest(const TRequest&, TResponse& response) final
    {
        response.send("HTTP/1.1 418 I'm a teapot\r\n\r\n");
    }
};
}
```

Now our router can be used like this:
```c++
    auto router = demo::Router{};
    router.route("/").process([](const demo::Request& request, demo::Response& response){
        response.send("HTTP/1.1 200 OK\r\n\r\n");
    });
    //...
    router.process(request, response);
```
#### Registering the request type

To use the request type in routes we need to register it by passing it as a template argument and implement the following virtual function:
* `virtual TRequestType getRequestType(const TRequest&) = 0;`
```c++
#include <whaleroute/requestrouter.h>

namespace demo{
class Router : public whaleroute::RequestRouter<Request, Response, Request::Method>{
    std::string getRequestPath(const Request& request) final
    {
        return request.uri;
    }
    
    Request::Method getRequestType(const Request& request) final
    {
        return request.method;
    }
    
    void processUnmatchedRequest(const Request&, Response& response)
    {
        response.send("HTTP/1.1 418 I'm a teapot\r\n\r\n");
    }
};
}

```
Now request type can be specified in the routes:
```c++
    auto router = demo::Router{};
    router.route("/", demo::Request::Method::GET).process([](const demo::Request& request, demo::Response& response)){
        response.send("HTTP/1.1 200 OK\r\n\r\n");
    }); 
```

To specify that a route should match requests with any request type, pass an object of an empty type `whaleroute::_{}`, or just  don't specify the request type.
```c++
    auto router = demo::Router{};
    router.route("/", whaleroute::_{}).process([](const demo::Request& request, demo::Response& response)){
        response.send("HTTP/1.1 200 OK\r\n\r\n");
    });
    router.route("/any").process([](const demo::Request& request, demo::Response& response)){
        response.send("HTTP/1.1 200 OK\r\n\r\n");
    }); 
```

#### Registering the request processing class
Instead of passing a lambda to the route's `process` method, you can use a custom class for request processing. To do this you must register the polymorphic base class by passing it as a template argument and use the child object in the `process` method. The request processing calling signature can be arbitrary, to support this you must to implement the following virtual function:
* `virtual void callRequestProcessor(TRequestProcessor&, const TRequest&, TResponse&) = 0;`
```c++
#include <whaleroute/requestrouter.h>

class RequestProcessor{
public:
    virtual ~RequestProcessor() = default;
    virtual void processRequest(const Request&, Response&) = 0;
};

namespace demo{
class Router : public whaleroute::RequestRouter<Request, Response, Request::Method, RequestProcessor>{
    std::string getRequestPath(const TRequest& request) final
    {
        return request.uri;
    }
    
    Request::Method getRequestType(const Request& request) final
    {
        return request.method;
    }
    
    void processUnmatchedRequest(const Request&, Response& response)
    {
        response.send("HTTP/1.1 418 I'm a teapot\r\n\r\n");
    }
    
    void callRequestProcessor(RequestProcessor& processor, const Request& request, Response& response) final
    {
        processor.processRequest(request, response);
    }
};
}
```

Now we can use the router with request processing objects:
```c++
    struct DemoProcessor : public RequestProcessor{
        void processRequest(const Request&, Response& response) final
        {
            response.send("HTTP/1.1 200 OK\r\nHello world!\r\n);
        }
    };

    auto router = demo::Router{};
    auto processor = DemoProcessor{};
    router.route("/", demo::Request::Method::GET).process(processor);
    //...
    router.process(request, response);
```

We can also use a request processor by specifying its type in the `process()`'s template argument. This way the object is instanced by **whaleroute** itself, and it's shared by all routes.
```c++
router.route("/", demo::Request::Method::GET).process<DemoProcessor>();
router.route("/hello", demo::Request::Method::GET).process<DemoProcessor>();
```

#### Registering the response value setter
We can further simplify setting up the routes, by registering the response value setter and using the value instead of request processor object or lambda.
To do this pass the response value type as the last `whaleroute::RequestRouter` template argument and implement the virtual function:
* `void setResponseValue(TResponse&, const TResponseValue&) = 0;`

```c++
#include <whaleroute/requestrouter.h>

namespace demo{
class Router : public whaleroute::RequestRouter<Request, Response, Request::Method, whaleroute::_, std::string>{
    std::string getRequestPath(const TRequest& request) final
    {
        return request.uri;
    }
    
    Request::Method getRequestType(const Request& request) final
    {
        return request.method;
    }
    
    void processUnmatchedRequest(const Request&, Response& response)
    {
        response.send("HTTP/1.1 418 I'm a teapot\r\n\r\n");
    }
    
    void setResponseValue(Response& response, const std::string& value)
    {
        response.send(value);
    }
};
}
```
*Notice how we can skip using the RequestProcessor class in this example, by passing the empty type `whaleroute::_` in its place. The same can be done with any functionality controlled by last 3 template arguments.*

Now the routes have a `set` method available:
```c++
    auto router = demo::Router{};
    router.route("/", demo::Request::Method::GET).set("HTTP/1.1 200 OK\r\n\r\n");
    //...
    router.process(request, response);
```

#### Using regular expressions
Router's `route` method can take a standard regular expression instead of a string to specify the route's path:
```c++
router.route(std::regex{"/.*"}, demo::Request::Method::GET).set("HTTP/1.1 200 OK\r\n\r\n");
```

#### Processing unmatched requests
Using the `route` method without arguments registers a processor for requests that don't match any existing routes. It's an alternative of a `processUnmatchedRequest` virtual method, it won't be called if you use a `route()` instead.
```c++
    router.route("/", demo::Request::Method::GET).set("HTTP/1.1 200 OK\r\n\r\n");
    router.route().set("HTTP/1.1 418 I'm a teapot\r\n\r\n");
```

#### Authorization
Routes can require requests to be authorized by `whaleroute::RequestRouter`'s virtual method `bool isAccessAuthorized(const TRequest&)` (by default it returns true for all requests).
It's controlled by the enumeration `whaleroute::RouteAccess` that is passed to the `route()` method as the last argument.
```c++  
namespace whaleroute{
    enum class RouteAccess{
        Authorized,
        Forbidden,
        Open
    };   
}
```
The default value is `RouteAccess::Open`, routes using it, don't check a request's authorization status.
Routes using the `RouteAccess::Authorized` value match requests when `isAccessAuthorized` returns true. 
`RouteAccess::Forbidden` routes are for requests that aren't authorized by `isAccessAuthorized` method.

```c++
namespace demo{
class Router : public whaleroute::RequestRouter<Request, Response, Request::Method, whaleroute::_, std::string>{
    std::string getRequestPath(const TRequest& request) final
    {
        return request.uri;
    }
    
    Request::Method getRequestType(const Request& request) final
    {
        return request.method;
    }
    
    void processUnmatchedRequest(const Request&, Response& response)
    {
        response.send("HTTP/1.1 418 I'm a teapot\r\n\r\n");
    }
    
    void setResponseValue(Response& response, const std::string& value)
    {
        response.send(value);
    }
    
    bool isAccessAuthorized(const Request& request)
    {
        return !(request.uri.size() >= 5 && request.uri.substr(0, 5) == "/vip/");     
    }
};
```
Now we can set up the router with authorization settings:
```c++
    auto router = demo::Router{};
    router.route("/").set("HTTP/1.1 200 OK\r\n\r\n");     
    router.route(std::regex{".*"}, 
                 whaleroute::_{}, 
                 whaleroute::RouteAccess::Forbidden).set("HTTP/1.1 401 Unauthorized\r\n\r\n");
```

Now any request to the URI starting with `/vip/` will get a 401 response status. 


### Installation
Download and link the library from your project's CMakeLists.txt:
```
cmake_minimum_required(VERSION 3.14)

include(FetchContent)

FetchContent_Declare(whaleroute
    GIT_REPOSITORY "https://github.com/kamchatka-volcano/whaleroute.git"
    GIT_TAG "origin/master"
)
FetchContent_MakeAvailable(whaleroute)

add_executable(${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME} PRIVATE whaleroute)
```

For the system-wide installation use these commands:
```
git clone https://github.com/kamchatka-volcano/whaleroute.git
cd whaleroute
cmake -S . -B build
cmake --build build
cmake --install build
```

### Running tests
```
cd whaleroute
cmake -S . -B build -DENABLE_TESTS=ON
cmake --build build 
cd build/tests && ctest
```

### License
**whaleroute** is licensed under the [MS-PL license](/LICENSE.md)  
