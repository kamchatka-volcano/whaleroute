<p align="center">
  <img height="192" src="doc/logo.png"/>  
</p>

[![build & test (clang, gcc, MSVC)](https://github.com/kamchatka-volcano/whaleroute/actions/workflows/build_and_test.yml/badge.svg?branch=master)](https://github.com/kamchatka-volcano/whaleroute/actions/workflows/build_and_test.yml)

**whaleroute** - is a C++17 header-only library for request routing. It is designed to bind handlers to HTTP requests,
but it can also be easily used with other protocols since the library is implemented as a generic class template.  
If your incoming data processing function has a signature like `void(const Request&, Response&)` and you need to perform
different actions based on a string value from the request object, **whaleRoute** can be of great help.

* [Usage](#implementing-the-router)
  * [Implementing the router](#implementing-the-router)
  * [Registering route matchers](#registering-route-matchers)
  * [Registering the response value setter](#registering-the-response-value-setter)
  * [Matching multiple routes](#matching-multiple-routes)
  * [Registering the route context](#registering-the-route-context)
  * [Using regular expressions](#using-regular-expressions)
  * [Trailing slash matching](#trailing-slash-matching)
  * [Processing unmatched requests](#processing-unmatched-requests)
  * [Using RequestProcessorQueue](#using-requestprocessorqueue)
* [Installation](#installation)
* [Running tests](#running-tests)
* [License](#license)

#### Implementing the router  
Let's say that our Request and Response classes look like this:
```c++
struct Request{
    std::string uri;
    enum class Method{
        GET,
        POST
    } method;
};
struct Response{
    bool isSent = false;
    void send(const std::string& data)
    {
        isSent = true;
        std::cout << data;
    }
};
```

To create the simplest router we must use Request and Response types as template arguments and implement two virtual functions:
* `virtual std::string getRequestPath(const TRequest&) = 0;`
* `virtual void processUnmatchedRequest(const TRequest&, TResponse&) = 0;`

```c++
#include <whaleroute/requestrouter.h>

class Router : public whaleroute::RequestRouter<Request, Response>{
    std::string getRequestPath(const Request& request) final
    {
        return request.uri;
    }
    
    void processUnmatchedRequest(const Request&, Response& response) final
    {
        response.send("HTTP/1.1 404 Not Found\r\n\r\n");
    }
};

```

Now our router can be used like this:

```c++
    auto router = Router{};
    router.route("/").process([](const Request& request, Response& response){
        response.send("HTTP/1.1 200 OK\r\n\r\n");
    });
    //...
    router.process(request, response);
```

The `process` method accepts any callable that can be invoked with the registered request and response objects.
Therefore, in addition to lambdas, it is possible to use free functions and function objects. It is also possible to
specify the type of the invocable class, allowing whaleroute to instantiate and take ownership of the request processor
object.

```c++
struct Responder{
    void operator()(const Request&, Response& response)
    {
        response.send("HTTP/1.1 200 OK\r\n\r\n");
    }
};

    auto router = Router{};
    router.route("/").process<Responder>();
    router.process(request, response);
```

#### Registering route matchers

By default, routes are matched based on request paths. To include other matching attributes, you can register them by
providing a specialization of the `whaleroute::config::RouteMatcher` class template. The `operator()` function within
the specialization should take a matcher's value, request and response objects, and return a boolean result of comparing
the matcher with some property of the request or response objects.

```c++
#include <whaleroute/routematcher.h>

template<>
struct RouteMatcher<Request::Method> {
    bool operator()(Request::Method value, const Request& request, Response&) const
    {
        return value == request.method;
    }
};
}

```

Now `Request::Method` can be specified in routes:

```c++
    auto router = Router{};
    router.route("/", Request::Method::GET).process([](const Request& request, Response& response)){
        response.send("HTTP/1.1 200 OK\r\n\r\n");
    }); 
```

#### Registering the response value setter

To simplify the setup of routes even further, it is possible to register the response value setter and use the value
instead of the request processor object or lambda. To do this, it is necessary to pass the response value type as the
3rd template argument of `whaleroute::RequestRouter` and implement the virtual function:

* `void setResponseValue(TResponse&, const TResponseValue&) = 0;`

```c++
#include <whaleroute/requestrouter.h>

class Router : public whaleroute::RequestRouter<Request, Response, std::string>{
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
```

Now the routes have a `set` method available:

```c++
    auto router = Router{};
    router.route("/", Request::Method::GET).set("HTTP/1.1 200 OK\r\n\r\n");
    //...
    router.process(request, response);
```

#### Matching multiple routes

By default, route processing stops at the first registered route that matches the request's path and any specified
matchers. This behavior can be controlled by using the router's virtual function

* `isRouteProcessingFinished(const TRequest&, TResponse& response)`.

It's invoked for each matched route after the registered handler, and if the result is false, the processing continues
and tries to match the subsequent routes.  
A practical example is to stop route processing only after the response has been sent. This allows you to register
request processors that perform some preparation work before generating the response.

```c++

class Router : public whaleroute::RequestRouter<Request, Response>{
    std::string getRequestPath(const Request& request) final
    {
        return request.uri;
    }
    
    void processUnmatchedRequest(const Request&, Response& response) final
    {
        response.send("HTTP/1.1 404 Not Found\r\n\r\n");
    }
    bool isRouteProcessingFinished(const Request&, Response& response) final
    {
        return response.isSent;
    }
    
};

    auto router = Router{};
    router.route(whaleroute::rx{".*"}, Request::Method::GET).process(const Request& request, Response&)){
      log(request);
    });
    router.route("/", Request::Method::GET).process([](const Request&, Response& response)){
      response.send("HTTP/1.1 200 OK\r\n\r\n");
    });
```

#### Registering the route context

To make the matching of multiple routes more useful, it is possible to share data between route processors. This can be
achieved by registering the route context class as the last template parameter of the whaleroute::RequestRouter
template. Once registered, it is then possible to take a reference to the context object in the last request processor
parameter.

```c++
#include <whaleroute/requestrouter.h>

struct Context{
    bool isAuthorized = false;
}; 

class Router : public whaleroute::RequestRouter<Request, Response, whaleroute::_, Context>{
    //...
};

void authorize(const Request& request, Response&, Context& ctx)
{
    ctx.isAuthorized = true;
}

    router.route(whaleroute::rx{".*"}, Request::Method::POST).process(authorize);
    router.route("/", Request::Method::GET).process([](const Request&, Response& response, const Context& ctx)){
      if (ctx.isAuthorized)  
          response.send("HTTP/1.1 200 OK\r\n\r\n");
      else
          response.send("HTTP/1.1 401 Unauthorized\r\n\r\n");
    });

```

*Notice how it's possible to skip using the value setter in this example by passing the empty type `whaleroute::_` in
its place.*

#### Using regular expressions

The `route` method of the Router can accept a regular expression instead of a string to specify the path of the route:

```c++
router.route(whaleroute::rx{"/.*"}, Request::Method::GET).set("HTTP/1.1 200 OK\r\n\r\n");
```

Currently, the regular expressions use the standard C++ library with ECMAScript grammar.

#### Trailing slash matching

By default, **whaleroute** treats trailing slashes in requests and route paths as optional. For example, `/path`
and `/path/` are considered equal.

This behavior can be changed by using the `setTrailingSlashMode` method of `whaleroute::RequestRouter` and providing the
value `whaleroute::TrailingSlashMode::Strict`.

#### Processing unmatched requests

Using the `route` method without arguments registers a processor for requests that do not match any existing routes.
This is an alternative to using the `processUnmatchedRequest` virtual method, which won't be called if you use `route()`
instead.

```c++
    router.route("/", Request::Method::GET).set("HTTP/1.1 200 OK\r\n\r\n");
    router.route().set("HTTP/1.1 418 I'm a teapot\r\n\r\n");
```

#### Using RequestProcessorQueue

If you check the implementation of the `process` method in `whaleroute::RequestRouter`, you'll see that it simply
creates a `whale::RequestProcessorQueue` object and forwards the request processing by calling its `launch` method:

```c++
    auto queue = makeRequestProcessorQueue(request, response);
    queue.launch();
```

`whaleroute::RequestProcessorQueue` is a sequence of all matched route processors that can be launched and stopped by
calling its `launch` and `stop` methods. It's available in the public interface, allowing you to create and
use `RequestProcessorQueue` directly without using the `RequestRouter::process` method. This can be especially useful in
an asynchronous environment, where route processing can be delayed by stopping the queue and resumed in the request
handler's callback using a captured copy of the queue.

Otherwise, you can disregard this information and simply use the `RequestRouter::process` method.

### Installation

Download and link the library from your project's CMakeLists.txt:

```
include(FetchContent)

FetchContent_Declare(whaleroute
    GIT_REPOSITORY "https://github.com/kamchatka-volcano/whaleroute.git"
    GIT_TAG "origin/master"
)
#uncomment if you need to install whaleroot with your target
#set(INSTALL_WHALEROOT ON)
FetchContent_MakeAvailable(whaleroute)

add_executable(${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME} PRIVATE whaleroute::whaleroute)
```

To install the library system-wide, use the following commands:
```
git clone https://github.com/kamchatka-volcano/whaleroute.git
cd whaleroute
cmake -S . -B build
cmake --build build
cmake --install build
```

After installation, you can use the find_package() command to make the installed library available inside your project:
```
find_package(whaleroute 1.0.0 REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE whaleroute::whaleroute)
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
