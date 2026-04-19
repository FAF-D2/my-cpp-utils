
#include"../xbox/xnet.hpp"
#include<cstdio>

template<class T>
xnet::detached_task injecter(xnet::AsyncTimer& timer, T& timeop, int timeout){
    co_await timer.timeout(timeout);
    (void)timeop.cancel();
    printf("injecter!!!\n");
}

xnet::task<xnet::details::io_result<int>> coro1(xnet::AsyncTimer& timer, int timeout){
    printf("Coro 1 starting with timeout %ds...\n", timeout);
    auto op = timer.timeout(timeout);
    injecter(timer, op, timeout / 2); // cancel coro1
    auto success = co_await op;
    if(!success){
        printf("Coro1 Error: %d, cancelled: %s\n", success.err, success.cancelled() ? "true" : "false");
        co_return {0, success.err};
    }
    printf("Coro 1 exiting...\n");
    co_return {10, 0};
}

xnet::task<xnet::details::io_result<int>> coro2(xnet::AsyncTimer& timer, int timeout){
    printf("Coro 2 starting with time out %ds...\n", timeout);
    auto success = co_await timer.timeout(timeout);
    if(!success){
        printf("Coro2 Error: %d, cancelled: %s\n", success.err, success.cancelled() ? "true" : "false");
        co_return {0, success.err};
    }
    printf("Coro 2 exiting...\n");
    co_return {200 * timeout, 0};
}

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

xnet::detached_task testfunc(xnet::AsyncTimer& timer){
    printf("----------Any Tester----------\n");
    co_await anytester(timer);
    printf("----------All Tester----------\n");
    co_await alltester(timer);
}

int main(){
    using namespace xnet;
    io_context ctx;
    AsyncTimer timer(ctx);

    testfunc(timer);
    
    ctx.run();
    return 0;
}
