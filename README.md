uv++
====

C++14 wrapper around libuv with an emphasis on simplicity and rapid development.

The github name is uvpp because github didn't like uv++ as the repository name. This project has no affiliation with any other project named uvpp.

## HEAVILY WORK IN PROGRESS!

#### Known issues:

* Just that it's incomplete

### Completed features

* Support for all callback functors
    - While libuv only allowed "pure" `void(*cb)(uv_handle_t*)` style callback functions, uv++ supports ALL possible functors, even lambdas with captures.

* Event Loop class
    - Includes default event loop from `uv_default_loop()`
    - Ability to close handles from any thread
        - Uses an internal Async handle and queue to invoke `uv_close` on the loop thread.
    
* Hierarchical handle classes
    - Base handle functions
    - Idle, Prepare and Check handles
    - Async handles
        - Capable of bidirectional communication, even between threads
        - Automatically deduces return and parameter types, even with lambda functions
            - Any number of additional parameters are supported.
        - Fully type safe, even with variadic parameters.
    - Signal handles
    - Automatically deduces whether or not the callback requires a pointer to the originating handle
    
* Misc OS and Net functions

* Automatic memory management for everything

* Optional ability to use Boost lockfree data structures where applicable.

### Still to do

* Basically all Request and Stream components
    - File I/O, DNS, TCP and UDP stuff

* Thread-pool coordination utilities
    - Still working out the best way to do those
    
* Polling handles
    
### Will not implement wrappers for:

* libuv threading and synchronization functions. C++11 introduced much better threading utilities, and I'd recommend those.

------

This README is updated before I sleep, and known issues will be tackled immediately in the morning or whenever I wake up.
