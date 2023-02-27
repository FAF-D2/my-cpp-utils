# A really *simple* header-only lib for personal use
### just for study and my personal program =)

## | Code Introduction |

### | **coro.hpp** |
1. Simple coroutine implemented in windows, supply for c++11 (no linux now)
2. Core classes: *io_context*, *Awaiter*, *Coroutine*
3. self-defined keyword like $yield(x), $await(x), ..., the code example is as follow
```cpp
// a simple echo tcp server
// a little bit troublesome but it works better than using threads
struct Echo: public Coroutine{
    async_socket client;
    unsigned int ret;
    char buf[BUFFER_SIZE];
    Echo(async_socket&& s):
    client(std::forward<async_socket>(s)) ret(BUFFER_SIZE), buf()
    {}
    void reusme(){
        $begin(){
            while(true){
                ret = 128;
                $await(client.recv(this, buf, &ret)); // await for client send
                $await(client.send(this, buf, ret)); // await for client recv
            }
        }
    }
};

struct listener: public Coroutine{
    async_acceptor acceptor;
    async_socket client;
    listener(): acceptor(), client()
    {}
    void resume(){
        $begin(){
            std::cout << acceptor.port() << std::endl;
            while(true){
                $await(acceptor.accept(this, &client)); // await for client connection
                $spawn(Echo(std::move(client))); // spawn a new coroutine
            }
        }
    }
};

int main(){
    io_context ctx; // define a scheduler
    ctx.spawn(listener());
    ctx.run();
    return 0;
}
```
### | **stringv.hpp** |
1. Simple 16-bytes string implemented, supply for c++11
2. Just a self-implementation for my json lib *(so use std::string in most cases)*
3. a lot of helpful functions not implemented (I will continue to improve it if I need those features)


### | **profiler.hpp** |
1. A simplest profiler for detecting memory-leak and time-consuming when contructing my own lib
2. supply for c++14, can' t work on placement-new and malloc
3. the code example is as follow
```cpp
// memory leak detect
#define _MEMORY_CHECK_
#define _NEW_DETAIL_    // define this macro will show all the new detail(file, line)
#include"profiler.hpp"
memory_report() // cout the memory not deleted yet.

// time check
#define _TIME_CHECK_
#include"profiler.hpp"
// self-defined macro, report the time consumed when leaving the "with" statement
$with(TimerCounter t){
    // do something
}
// with statement exit!, auto cout the time
```

### | **lazyjson.hpp** |
1. still trying to implement it (I have to admit it is a little bit difficult without STL)


### · Anyway, it is just a simple cpp utils lib for those who want to construct their own lib to learn