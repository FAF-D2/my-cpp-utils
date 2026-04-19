#include"../xbox/xnet.hpp"
#include<cstdlib>
#include<cstdio>

const char* ip = "127.0.0.1";
uint16_t port = 12000;


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

xnet::detached_task monitor(xnet::io_context& ctx){
    xnet::AsyncTimer timer(ctx);

    while(true){
        auto success = co_await timer.timeout(5);
        if(!success){
            printf("[Monitor] Error: %d\n", success.err);
            co_return;
        }
        printf("[Monitor] How many tasks: %ld\n", ctx.num_evs());
    }
}

xnet::task<> myAccepter(xnet::io_context& ctx){
    printf("Server [%s:%d]: waiting for accept...\n", ip, port);

    xnet::io_context::TCPAccepter accepter(ctx, xnet::v4addr(ip, port));
    if(accepter.invalid()){
        printf("Accepter init failed\n");
        exit(0);
        co_return;
    }

    while(true){
        auto conn = co_await accepter.accept().timeout(10);
        if(conn.cancelled()){
            printf("[Accepter] Still no accepting within 10s\n");
            continue;
        }
        if(conn.err){
            printf("[Accepter] Some error: %d\n", conn.err);
            conn.destroy();
            if(accepter.invalid()){
                co_return;
            }
            continue;
        }
        echo(conn.move()); // spawn a echo coroutine
    }
}


int main(){
    xnet::io_context ctx(1024);
    if(ctx.invalid()){
        printf("context init failed\n");
        return 0;
    }
    auto coro = myAccepter(ctx); 
    coro.start();
    monitor(ctx);

    ctx.run<256>(); // run forever
    return 0;
}
