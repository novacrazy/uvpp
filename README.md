uv++
====

C++14 wrapper around libuv with an emphasis on simplicity and rapid development.

The github name is uvpp because github didn't like uv++ as the repository name. This project has no affiliation with any other project named uvpp.

## HEAVILY WORK IN PROGRESS!

Currently working on a predictable way of managing the lifetimes of the event loop, handles, requests, etc., automatically.

My current idea is to base everything off the lifetime of the event loop object. When it is destroyed, it should take all allocated handles with it. But it's a bit more complicated than that.

### Example

You might notice the use of `.get()` for async operations; that's because uv++ uses `promise`s, `future`s, and `shared_future`s for asynchronous results.

```C++
#define UV_OVERLOAD_OSTREAM
#define UV_USE_BOOST_LOCKFREE

#include <uv++/uv++.hpp>

#include <iostream>
#include <chrono>
#include <thread>

using namespace std;

int main() {
    auto loop = uv::default_loop();
    
    //Run this callback once per second
    loop->interval(1s, [] {
        cout << "\aBeep" << endl;
    });
    
    //Create an async task that has three parameters and a return value
    auto a = loop->async([] (int x, int y, int z) {
        return x + y * z;
    });
    
    thread([=] {
        //Open file synchronously
        auto f = loop->fs()->openSync("main.cpp");
        
        //Read in the file asynchronously and wait on the result
        cout << f->read().get() << endl;
        
        //When out of scope, f will schedule itself to be closed on the event loop thread
    }).detach();
    
    thread([=] {
        loop->schedule([=] {
            cout << "This function is run on the loop thread." << endl;
        }).get();
        
        loop->work()->queue([=] {
            cout << "This function is running in the libuv thread-pool." << endl;
            
            //Send a request to the async task and wait on the result.
            cout << a->send(1, 2, 3).get() << endl; //Will print 9
            
            std::this_thread::sleep_for(2s);
        }).get();
        
    }).detach();
    
    //Start the loop.
    loop->start();
}
```

And I'll add more if I think of it, but basically uv++ has all the functionality of libuv, but mostly thread-safe, type-safe, memory-safe and more flexible.

#### Known issues:

* Just that it's incomplete

### Completed features

* Support for all callback functors
    - While libuv only allowed "pure" `void(*cb)(uv_handle_t*)` style callback functions, uv++ supports ALL possible functors, even lambdas with captures.

* Event Loop class
    - Includes default event loop from `uv_default_loop()`
    - Ability to close handles from any thread
        - Uses an internal Async handle and queue to invoke `uv_close` on the loop thread.
    
* Hierarchical Handle classes
    - Base handle functions
    - Idle, Prepare and Check handles
    - Async handles
        - Capable of bidirectional communication, even between threads
        - Automatically deduces return and parameter types, even with lambda functions
            - Any number of additional parameters are supported.
        - Fully type safe, even with variadic parameters.
    - Signal handles
    - Automatically deduces whether or not the callback requires a pointer to the originating handle
    
* Hierarchical Request classes
    - Base request functions
    - Work requests for the libuv thread-pool
        - Totally thread safe, you can queue up work from any thread
        - Supports bidirectional communication similar to Async handles, but the arguments are given at queue time.
    
* Misc OS and Net functions

* Automatic memory management for everything

* Optional ability to use Boost lockfree data structures where applicable.

### Still to do

* Basically all Request and Stream components
    - File I/O, DNS, TCP and UDP stuff

* Polling handles
    
### Will not implement wrappers for:

* libuv threading and synchronization functions. C++11 introduced much better threading utilities, and I'd recommend those.

------

This README is updated before I sleep, and known issues will be tackled immediately in the morning or whenever I wake up.
