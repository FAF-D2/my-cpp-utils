# A really *simple* header-only lib for personal use
### just for study and my personal program =)

## | Code Introduction |

### | **xnet.hpp** |
1. linux iouring with c++20 coroutine
```cpp
// a simple echo tcp server
// same as golang go the func
// for full code see test/test_echo.cpp
xnet::detached_task echo(xnet::TCPServer conn){
    char buf[64];
    while(true){
        auto result = co_await conn.recv(buf, sizeof(buf), 0);
        if(result.rst() || *result == 0){
            // connection closed
            co_return;
        }
        else if(result.err){
            // error handler
            co_return;
        }

        auto sended = co_await conn.send(buf, *result, 0);
        if(sended.err && !sended.rst()){
            // error handler
            co_return;
        }
    }
}

// a simple all and any operator use
// same behavior like promise.all promise.any in javascripy
// for full code see test/test_operator.cpp
xnet::task<> anytester(xnet::AsyncTimer& timer){
    auto result = co_await any(
        coro1(timer, 2),
        coro2(timer, 5),
        coro2(timer, 10)
    );
    auto& r = result.get<0>();
    printf("done with %d\n", *r);
    result.destroy<0>();
}

xnet::task<> alltester(xnet::AsyncTimer& timer){
    auto& result = co_await allSettled(
        coro1(timer, 2),
        coro2(timer, 5),
        coro2(timer, 10)
    );
    auto& [r1, r2, r3] = result;
    printf("done with %d, %d, %d\n", *r1, *r2, *r3);
}
```
### | **xstring.hpp** |
1. simple 16-bytes string type implemented, supply for c++11
2. just a self-implementation for my json lib *(so use std::string in most cases)*
3. a lot of helpful functions not implemented (continuously improving...)

### | **xjson.hpp** |
1. modern json c++14 library, aligned with python json library
2. be careful that the xjson::parse will not do anything inverted in the string (no \\\" -> \", no utf-8, nothing but raw string accepted from raw json string)
```cpp
// see test/test_json.cpp for full use
xjson payload = {
    {"name", "xjson"},
    {"support", 14},
    {"byte", 16},
    {"something", xjson::Array({1, 2, 3.1415, false, nullptr})}
};
auto json_str = payload.dump();
payload["name"] = "myxjson";
payload["newfield"] = xjson::Array({"anything", true});
printf("dumped json: %s\n", json_str.c_str());
printf("after json: %s\n", payload.dump().c_str());

// payload = xjson::parse(json_str);
payload = xjson::parse(json_str.c_str(), json_str.size());
```

### | **xmeta.hpp** |
1. Be used to do some meta programming

### · Anyway, it is just a simple cpp utils lib for those who want to construct their own lib to learn