#ifndef profiler_hpp
#define profiler_hpp
#include<malloc.h>
#include<functional>
#include<type_traits>
#include<assert.h>
#include<iostream>
#include<chrono>

#ifdef _MEMORY_CHECK_
class _INFO_LIST {
    using uint = unsigned int;
    friend void memory_report();
    friend void* operator new(size_t size, const char* file, unsigned int line);
    friend void* operator new[](size_t size, const char* file, unsigned int line);
    friend void operator delete(void* p);
    friend void operator delete[](void* p);

    void* p;
    const char* file;
    _INFO_LIST* next;
    uint line;
    uint size;
    uint if_array;
    static _INFO_LIST head;
    static _INFO_LIST* tail;
    static uint total_size;

    _INFO_LIST() :p(nullptr), file(nullptr), next(nullptr), line(0), size(0), if_array(0)
    {}
    static void _SET_NEW_INFO(void* p, const char* file, uint line, uint size, uint if_array) {
        total_size += size;
        tail->next = (_INFO_LIST*)malloc(sizeof(_INFO_LIST));
        tail = tail->next;
        tail->p = p;
        tail->file = file;
        tail->next = nullptr;
        tail->line = line;
        tail->size = size;
        tail->if_array = if_array;
#ifdef _NEW_DETAIL_
        printf("File [%s] line %u, new ptr: %p\n", file, line, p);
#endif
    }
    static void _DELETE_NEW_INFO(void* p, int if_array) {
#ifdef _NEW_DETAIL_
        printf("delete ptr: %p\n", p);
#endif
        _INFO_LIST* prev = &_INFO_LIST::head;
        while (prev->next != nullptr) {
            if (prev->next->p != p) {
                prev = prev->next;
            }
            else {
                break;
            }
        }
        _INFO_LIST* info = prev->next;
        if (info == nullptr) {
            printf("err: [delete unknown ptr]\n");
            assert(false);
        }
        else if (info->if_array != if_array) {
            printf("File [%s] line %u, err: [inconsistent delete method]\n", info->file, info->line);
            assert(false);
        }
        else {
            total_size -= info->size;
            prev->next = info->next;
            if (info == tail) {
                tail = prev;
            }
            free(info);
        }
    }
};

_INFO_LIST _INFO_LIST::head;
_INFO_LIST* _INFO_LIST::tail = &head;
unsigned int _INFO_LIST::total_size = 0;

inline void memory_report() {
    printf("----memory info----\n");
    printf("rest memory: %u\n", _INFO_LIST::total_size);
    for (auto ptr = _INFO_LIST::head.next; ptr != nullptr; ptr = ptr->next) {
        printf("File [%s] line %u, size: %u\n", ptr->file, ptr->line, ptr->size);
    }
    printf("----memory info----\n");
}

inline void* operator new(size_t size, const char* file, unsigned int line) {
    if (size == 0) {
        printf("File [%s] line %u, err: [new for 0 byte]\n", file, line);
        assert(false);
    }
    void* ptr = malloc(size);
    if (ptr == nullptr) {
        printf("File [%s] line %u, err: [new error nullptr]\n", file, line);
    }
    else {
        _INFO_LIST::_SET_NEW_INFO(ptr, file, line, size, 0);
    }
    return ptr;
}
inline void* operator new[](size_t size, const char* file, unsigned int line) {
    if (size == 0) {
        printf("File [%s] line %u, err: [new for 0 byte]\n", file, line);
        assert(false);
    }
    void* ptr = malloc(size);
    if (ptr == nullptr) {
        printf("File [%s] line %u, err: [new error nullptr]\n", file, line);
    }
    else {
        _INFO_LIST::_SET_NEW_INFO(ptr, file, line, size, 1);
    }
    return ptr;
}

inline void operator delete(void* p) {
    _INFO_LIST::_DELETE_NEW_INFO(p, 0);
    free(p);
}

inline void operator delete[](void* p) {
    _INFO_LIST::_DELETE_NEW_INFO(p, 1);
    free(p);
}
#define new new(__FILE__, __LINE__)
#endif


#ifdef _TIME_CHECK_
template<class _RET, class ...Args, std::enable_if_t<!std::is_void<_RET>::value, bool> = true>
auto _TIME_WRAPPER(_RET(*fn)(Args ...args)){
    return [fn](Args ...args) -> _RET{
        auto begin = std::chrono::high_resolution_clock::now();
        auto ret = fn(args...);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = (end - begin).count();
        printf("%.3f s | %f ms | %f us\n", duration / 10e9, duration / 10e6, duration / 10e3);
        return ret;
    };
}

template<class _RET, class ...Args, std::enable_if_t<std::is_void<_RET>::value, bool> = true>
auto _TIME_WRAPPER(_RET(*fn)(Args ...args)){
    return [fn](Args ...args) -> _RET{
        auto begin = std::chrono::high_resolution_clock::now();
        fn(args...);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = (end - begin).count();
        printf("%.3f s | %f ms | %f us\n", duration / 10e9, duration / 10e6, duration / 10e3);
    };
}

// for lambda(no return)
template<class Fn>
void _TIME_WRAPPER(Fn fn) {
    return [fn](auto&& ...args){
        auto begin = std::chrono::high_resolution_clock::now();
        fn(args...);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = (end - begin).count();
        printf("%.3f s | %f ms | %f us\n", duration / 10e9, duration / 10e6, duration / 10e3);
    };
}
#ifndef $with
#define $with(statement) if(int $_WITH_STATE = false);else for(statement;$_WITH_STATE!=1;$_WITH_STATE=1)
#endif
class TimeCounter{
    std::chrono::steady_clock::time_point begin;
public:
    TimeCounter():begin(std::chrono::high_resolution_clock::now())
    {}
    ~TimeCounter(){
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = (end - begin).count();
        printf("%.3f s | %f ms | %f us\n", duration / 10e9, duration / 10e6, duration / 10e3);
    }
};

#define _TIME_COUNT(func) _TIME_WRAPPER(func)
#endif

#endif