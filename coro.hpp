#ifndef _CORO_HPP
#define _CORO_HPP
#include<type_traits>
#include<utility>
#include<new>
#include<stdio.h>

#ifdef _WIN32
#include<WinSock2.h>
#include<fileapi.h>
#pragma warning(disable:4996)
#pragma comment(lib, "ws2_32.lib")
#define sleep(s) SleepEx(s, true)
#else
#include<unistd.h>
#endif

#define $with(statement) if(int $_WITH_STATE = false);else for(statement;$_WITH_STATE!=1;$_WITH_STATE=1)
#define $getter(type, name, begin, func) struct getter{type name=begin;operator type(){func;}}name;
#define $setter(type, name, begin, func) struct setter{type name=begin;void operator=(type&& value){func}}name;

#define $begin() switch(state) case -1:
#define $yield(x) do{state=__COUNTER__+1;return x;case __COUNTER__:;}while(0)
#define $destroy(co) do{(co)->ready=IO_STATE::DONE;}while(0);return
#define $await(func) do{state=__COUNTER__+1;this->ready=IO_STATE::PENDING;this->ctx->spawn(func);return;case __COUNTER__:;}while(0)
#define $spawn(co) do{this->ctx->spawn((co));}while(0)
#define $async 


enum IO_STATE{
    PENDING = 0x3f3f3f3f,
    FULLFILLED,
    REJECTED,
    LOOPING,
    DONE,
};

enum IOS{
    r = 0x00000001,
    w = 0x00000002,
    a = 0x00000004,
    rb = 0x00000011,
    wb = 0x00000012,
    ab = 0x00000014
};

class Awaiter{
protected:
    int state;
public:
    unsigned int ready;
    Awaiter():state(-1), ready(IO_STATE::FULLFILLED)
    {}
    ~Awaiter()
    {}
};


class Coroutine;
class io_context{
private:
    typedef void(*resume)(Awaiter*c, int mode);
    struct colist{
        colist(Awaiter* c, colist* n, resume f):c(c), next(n), func(f)
        {}
        ~colist()
        {}
        Awaiter* c;
        colist* next;
        resume func;
    };
    struct colist* head;
    struct colist* pre; // the pre-node
public:
    io_context():head(new colist(NULL, NULL, NULL)), pre(head)
    {}
    ~io_context(){
        delete head;
    }

    template<class _Base, class _Derived>
    using traits = std::is_base_of<_Base, _Derived>;

    template<class T, typename std::enable_if<traits<Coroutine, T>::value, bool>::type = true>
    void spawn(T&& coroutine){
        char* block = (char*)malloc(sizeof(colist) + sizeof(T));
        colist* temp = new(block) colist(new(block + sizeof(colist)) T(std::forward<T>(coroutine)), NULL, [](Awaiter* c, int mode){
            if(mode == 0) ((T*)c)->resume(); else ((T*)c)->~T();
        });
        temp->next = pre->next;
        pre->next = temp;
        ((T*)temp->c)->ctx = this;
        ((T*)temp->c)->resume();
    }

    template<class T, typename std::enable_if<traits<Awaiter, T>::value && !traits<Coroutine, T>::value, bool>::type = true>
    void spawn(T&& coroutine){
        char* block = (char*)malloc(sizeof(colist) + sizeof(T));
        colist* temp = new(block) colist(new(block + sizeof(colist)) T(std::forward<T>(coroutine)), NULL, [](Awaiter* c, int mode){
            if(mode == 0) ((T*)c)->resume(); else ((T*)c)->~T();
        });
        temp->next = pre->next;
        pre->next = temp;
        ((T*)temp->c)->resume();
    }
    void run(){
        while(true)
        {
            bool busy = false;
            pre = head;
            struct colist* cur = pre->next;
            if(!cur){
                break;
            }
            while(cur)
            {
                switch(cur->c->ready){
                    case IO_STATE::PENDING: {
                        pre = cur;
                        cur = cur->next;
                        break;
                    }
                    case IO_STATE::FULLFILLED: {
                        cur->func(cur->c, 0);
                        busy = true;
                        pre = cur;
                        cur = cur->next;
                        break;
                    }
                    case IO_STATE::REJECTED: {
                        // fail
                        pre->next = cur->next;
                        cur->func(cur->c, 1);
                        free(cur);
                        cur = pre->next;
                        busy = true;
                        break;
                    }
                    case IO_STATE::LOOPING: {
                        cur->func(cur->c, 0);
                        pre = cur;
                        cur = cur->next;
                        break;
                    }
                    case IO_STATE::DONE: {
                        pre->next = cur->next;
                        cur->func(cur->c, 1);
                        free(cur);
                        cur = pre->next;
                        busy = true;
                        break;
                    }
                }    
            }
            if(!busy){
                sleep(0);
            }
        }
    }
};

class Coroutine : public Awaiter{
public:
    io_context* ctx;
    Coroutine():ctx(NULL)
    {}
    ~Coroutine()
    {}
};


#ifdef _WIN32
static bool _WSA_INIT(){
    static bool _WSA_INIT = false;
    static WSADATA wasdata = [](){
        WSADATA data;
        _WSA_INIT = (WSAStartup(MAKEWORD(2, 2), &data) == 0);
        return data;
    }();
    return _WSA_INIT;
}

class async_socket{
    friend class async_acceptor;
private:
    SOCKET client;

    $async class sender: public Awaiter{
        friend class async_socket;
        friend class io_context;
        SOCKET* client;
        Coroutine* coroutine;
        const char* buf;
        unsigned int len;
        unsigned int offset;

        sender(SOCKET* s, Coroutine* co, const char* buf, unsigned int len):
        client(s), coroutine(co), buf(buf), len(len), offset(0)
        {
            this->ready = IO_STATE::LOOPING;
        }
        sender() = delete;

        void resume(){
            $begin(){
                if(*client == INVALID_SOCKET){
                    coroutine->ready = IO_STATE::REJECTED;
                    $destroy(this);
                }
                while(true){
                    int ret;
                    ret = ::send(*client, buf + offset, len - offset, 0);
                    if(ret == SOCKET_ERROR){
                        if(WSAGetLastError() != WSAEWOULDBLOCK){
                            closesocket(*client);
                            *client = INVALID_SOCKET;
                            coroutine->ready = IO_STATE::REJECTED;
                            $destroy(this);
                        }
                    }
                    else if(ret == 0){
                        // connection closed
                        closesocket(*client);
                        *client = INVALID_SOCKET;
                        coroutine->ready = IO_STATE::REJECTED;
                        $destroy(this);
                    }
                    else{
                        offset += ret;
                        if(offset == len){
                            coroutine->ready = IO_STATE::FULLFILLED;
                            $destroy(this);
                        }
                    }
                    $yield();
                }
            }
        }
    };

    $async class receiver: public Awaiter{
        friend class async_socket;
        friend class io_context;
        SOCKET* client;
        Coroutine* coroutine;
        unsigned int* result;
        char* buf;

        receiver(SOCKET* s, Coroutine* co, unsigned int* ret, char* buf):
        client(s), coroutine(co), result(ret), buf(buf)
        {
            this->ready = IO_STATE::LOOPING;
        }
        receiver() = delete;

        void resume(){
            $begin(){
                if(*client == INVALID_SOCKET){
                    *result = 0;
                    coroutine->ready = IO_STATE::REJECTED;
                    $destroy(this);
                }
                while(true){
                    int ret;
                    ret = ::recv(*client, buf, *result, 0);
                    if(ret == SOCKET_ERROR){
                        if(WSAGetLastError() != WSAEWOULDBLOCK){
                            closesocket(*client);
                            *client = INVALID_SOCKET;
                            *result = 0;
                            coroutine->ready = IO_STATE::REJECTED;
                            $destroy(this);
                        }
                    }
                    else if(ret == 0){
                        // connection closed
                        closesocket(*client);
                        *client = INVALID_SOCKET;
                        *result = 0;
                        coroutine->ready = IO_STATE::REJECTED;
                        $destroy(this);
                    }
                    else{
                        *result = ret;
                        coroutine->ready = IO_STATE::FULLFILLED;
                        $destroy(this);
                    }
                    $yield();
                }
            }
        }
    };

    $async class connector: public Awaiter{
        friend class async_socket;
        friend class io_context;
        SOCKET* client;
        Coroutine* coroutine;
        sockaddr_in addr;
        connector(SOCKET* s, Coroutine* co, sockaddr_in&& addr):
        client(s), coroutine(co), addr(std::forward<sockaddr_in>(addr))
        {
            this->ready = IO_STATE::LOOPING;
        }
        connector() = delete;

        void resume(){
            $begin(){
                if(*client == INVALID_SOCKET){
                    coroutine->ready = IO_STATE::REJECTED;
                    $destroy(this);
                }
                while(true){
                    if(::connect(*client, (LPSOCKADDR)&addr, sizeof(addr)) == SOCKET_ERROR){
                        if(WSAGetLastError() != WSAEWOULDBLOCK){
                            closesocket(*client);
                            *client = INVALID_SOCKET;
                            coroutine->ready = IO_STATE::REJECTED;
                            $destroy(this);
                        }
                    }
                    else{
                        coroutine->ready = IO_STATE::FULLFILLED;
                        $destroy(this);
                    }
                    $yield();
                }
            }
        }
    };

public:
    $async connector connect(Coroutine* coroutine, const char* host, u_short port){
        if(client == INVALID_SOCKET){
            if(_WSA_INIT()){
                client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            }
        }
        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr(host);
        addr.sin_port = htons(port);
        return connector(&client, coroutine, std::move(addr));
    }

    $async sender send(Coroutine* co, const char* buf, unsigned int len){
        return sender(&client, co, buf, len);
    }

    $async receiver recv(Coroutine* co, char* buf, unsigned int* len){
        return receiver(&client, co, len, buf);
    }
public:
    async_socket():client(INVALID_SOCKET)
    {}
    async_socket(const async_socket& other) = delete;
    async_socket(async_socket&& other) noexcept {
        this->client = other.client;
        other.client = INVALID_SOCKET;
    }
    ~async_socket(){
        if(client != INVALID_SOCKET){
            closesocket(client);
        }
    }

    void close(){
        if(client != INVALID_SOCKET){
            closesocket(client);
            client = INVALID_SOCKET;
        }
    }

    sockaddr_in addr(){
        if(client == INVALID_SOCKET){
            return sockaddr_in();
        }
        sockaddr_in addr;
        int size = sizeof(addr);
        if(getsockname(client, (LPSOCKADDR)&addr, &size) != 0){
            return sockaddr_in();
        }
        return addr;
    }

    bool operator==(SOCKET other){
        return client == other;
    }

};

class async_acceptor{
private:
    SOCKET server;

    $async class acceptor: public Awaiter{
        friend class async_acceptor;
        friend class io_context;
        SOCKET server;
        Coroutine* coroutine;
        async_socket* result;
        acceptor(SOCKET s, Coroutine* co, async_socket* ret):server(s), coroutine(co), result(ret)
        {
            this->ready = IO_STATE::LOOPING;
        }
        acceptor() = delete;
        void resume(){
            $begin(){
                if(server == INVALID_SOCKET){
                    result->client = INVALID_SOCKET;
                    coroutine->ready = IO_STATE::REJECTED;
                    $destroy(this);
                }
                while(true){
                    result->client = ::accept(server, NULL, NULL);
                    if(result->client != SOCKET_ERROR){
                        u_long mode = 1;
                        if(ioctlsocket(server, FIONBIO, &mode) == SOCKET_ERROR){
                            closesocket(result->client);
                            result->client = INVALID_SOCKET;
                            coroutine->ready = IO_STATE::REJECTED;
                        }else{
                            coroutine->ready = IO_STATE::FULLFILLED;
                        }
                        $destroy(this);
                    }
                    else if(WSAGetLastError() != WSAEWOULDBLOCK){
                        result->client = INVALID_SOCKET;
                        coroutine->ready = IO_STATE::REJECTED;
                        $destroy(this);
                    }
                    $yield();
                }
            }
        }
    };
public:
    $async acceptor accept(Coroutine* coroutine, async_socket* result){
        return acceptor(this->server, coroutine, result);
    }
public:
    async_acceptor(u_short port = 0): 
    server(_WSA_INIT() ? socket(AF_INET, SOCK_STREAM, IPPROTO_TCP) : INVALID_SOCKET)
    {
        if(server != INVALID_SOCKET){
            u_long mode = 1;
            if(ioctlsocket(server, FIONBIO, &mode) == SOCKET_ERROR){
                closesocket(server);
                server = INVALID_SOCKET;
            }
            if(mode == 0){
                closesocket(server);
                server = INVALID_SOCKET;
            }
        }
        if(server == INVALID_SOCKET){
            return;
        }
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);
        if(bind(server, (LPSOCKADDR)&addr, sizeof(addr)) == SOCKET_ERROR){
            closesocket(server);
            server = INVALID_SOCKET;
            return;
        }
        if(listen(server, SOMAXCONN) == SOCKET_ERROR){
            closesocket(server);
            server = INVALID_SOCKET;
        }
    }
    async_acceptor(const async_acceptor& other) = delete;
    async_acceptor(async_acceptor&& other) noexcept {
        this->server = other.server;
        other.server = INVALID_SOCKET;
    }
    ~async_acceptor(){
        if(server != INVALID_SOCKET){
            closesocket(server);
        }
    }
    void close(){
        if(server != INVALID_SOCKET){
            closesocket(server);
            server = INVALID_SOCKET;
        }
    }

    u_short port(){
        if(server == INVALID_SOCKET){
            return 0;
        }
        sockaddr_in addr;
        int size = sizeof(addr);
        if (getsockname(server, (LPSOCKADDR)&addr, &size) != 0) {
            return 0;
        }
        return ntohs(addr.sin_port);
    }

    bool operator==(SOCKET s){
        return this->server == s;
    }
};

class async_stream{
private:
    HANDLE stream;
    unsigned int m;
    $async class reader: public Awaiter{
        friend class async_stream;
        friend class io_context;
        Coroutine* coroutine;
        HANDLE stream;
        char* buf;
        unsigned long* toread;
        OVERLAPPED o;
        reader() = delete;
        reader(Coroutine* co, HANDLE s, char* buf, unsigned long* r):
        coroutine(co), stream(s), buf(buf), toread(r), o({0})
        {
            this->ready = IO_STATE::LOOPING;
        }
        static void CALLBACK callback(DWORD err, DWORD bytes, LPOVERLAPPED o){
            *((reader*)o->hEvent)->toread = bytes;
            ((reader*)o->hEvent)->coroutine->ready = IO_STATE::FULLFILLED;
            $destroy(((reader*)o->hEvent));
        }
        void resume(){
            $begin(){
                if(stream == INVALID_HANDLE_VALUE){
                    *toread = -1;
                    coroutine->ready = IO_STATE::REJECTED;
                    $destroy(this);
                }
                o.hEvent = this;
                if(!ReadFileEx(stream, (LPVOID)buf, *toread, &o, reader::callback)){
                    *toread = -1;
                    coroutine->ready = IO_STATE::REJECTED;
                    $destroy(this);
                }
                while(true){
                    WaitForSingleObjectEx(stream, 0, true);
                    $yield();
                }
            }
        }
    };
    $async class writer: public Awaiter{
        friend class async_stream;
        friend class io_context;
        Coroutine* coroutine;
        HANDLE stream;
        const char* buf;
        unsigned long* towrite;
        OVERLAPPED o;
        writer() = delete;
        writer(Coroutine* co, HANDLE s, const char* buf, unsigned long* w):
        coroutine(co), stream(s), buf(buf), towrite(w), o({0})
        {
            this->ready = IO_STATE::LOOPING;
        }
        static void CALLBACK callback(DWORD err, DWORD bytes, LPOVERLAPPED o){
            *((writer*)o->hEvent)->towrite = bytes;
            ((writer*)o->hEvent)->coroutine->ready = IO_STATE::FULLFILLED;
            $destroy(((writer*)o->hEvent));
        }
        void resume(){
            $begin(){
                if(stream == INVALID_HANDLE_VALUE){
                    *towrite = -1;
                    coroutine->ready = IO_STATE::REJECTED;
                    $destroy(this);
                }
                o.hEvent = this;
                if(!WriteFileEx(stream, (LPVOID)buf, *towrite, &o, writer::callback)){
                    *towrite = -1;
                    coroutine->ready = IO_STATE::REJECTED;
                    $destroy(this);
                }
                while(true){
                    WaitForSingleObjectEx(stream, 0, true);
                    $yield();
                }
            }
        }
    };
public:
    $async reader read(Coroutine* coroutine, char* buf, unsigned long* toread){
        if(m & IOS::r){
            return reader(coroutine, stream, buf, toread);
        }
        return reader(coroutine, INVALID_HANDLE_VALUE, NULL, toread);
    }
    $async writer write(Coroutine* coroutine, const char* buf, unsigned long* towrite){
        if((m & IOS::w) || (m & IOS::a)){
            return writer(coroutine, stream, buf, towrite);
        }
        return writer(coroutine, INVALID_HANDLE_VALUE, NULL, towrite);
    }
public:
    async_stream() = delete;
    async_stream(const async_stream&) = delete;
    async_stream(async_stream&& other){
        stream = other.stream;
        other.stream = INVALID_HANDLE_VALUE;
        m = other.m;
    }
    // the handler must be created by FILE_FLAG_OVERLAPPED
    async_stream(HANDLE handler, IOS mode):stream(handler), m(mode)
    {}
    ~async_stream(){
        if(stream != INVALID_HANDLE_VALUE){
            CloseHandle(stream);
        }
    }

    int close(){
        if(stream != INVALID_HANDLE_VALUE){
            int ret = CloseHandle(stream);
            stream = INVALID_HANDLE_VALUE;
            m = 0x00000000U;
            return ret;
        }
        return 1;
    }

    int flush(){
        if(stream != INVALID_HANDLE_VALUE && (m & IOS::w)){
            return FlushFileBuffers(stream);
        }
        return 1;
    }

    static async_stream create_async_file(LPCWSTR name, IOS mode){
        HANDLE hfile = INVALID_HANDLE_VALUE;
        if(mode & IOS::r){
            hfile = CreateFile(name, GENERIC_READ, 0, 
            NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OVERLAPPED, NULL);
        }
        else if(mode & IOS::w){
            hfile = CreateFile(name, GENERIC_WRITE, 0, 
            NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OVERLAPPED, NULL);
        }
        else if(mode & IOS::a){
            hfile = CreateFile(name, GENERIC_WRITE, 0, 
            NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OVERLAPPED, NULL);
            SetFilePointer(hfile, NULL, NULL, FILE_END);
        }
        return async_stream(hfile, mode);
    }
};

#elif __linux__
// Not Implemented
#endif

#endif