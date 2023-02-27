#ifndef _STRINGV_HPP
#define _STRINGV_HPP
#include<string.h>
#include<type_traits>

#ifndef _NOAUTO_CSTR2SV_
#pragma message("auto const char* to stringv is opening") 
#define _AUTO_CSTR2SV_
#endif

#define _STRINGV_BIG(str) ((str).hex[15]&0x80)
#define _STRINGV_NULL(str) ((str).hex[0]=='\0')
#define _RESET_STRINGV() if(hex[15]&0x80){delete[] ptr;}
#define _STRINGV_LENGTH(str) (*((unsigned int*)((str).hex+12)))
#define _STRINGV_CAPACITY(str) (*((unsigned int*)((str).hex+8)))
constexpr unsigned int _INV_PAD = 0x7FFFFFFFU;
constexpr unsigned int _PAD = 0x80000000U;
constexpr int _BEGIN = 0x3f3f3f3f;
constexpr int _END = 0x7f7f7f7f;

#define _SV(cstr) stringv::_CREATE_STRINGV<sizeof((cstr))>((cstr))

union stringv{
private:
    char hex[16];
    char* ptr;
public:
    // utils
    void clear(){
        _RESET_STRINGV();
        hex[0] = 0x00;
        hex[15] = 0x00;
    }
    const char* c_str() {
        return _STRINGV_BIG(*this) ? ptr : hex;
    }
    bool small() {
        return !_STRINGV_BIG(*this);
    }
    bool empty() {
        return _STRINGV_BIG(*this) ? false: _STRINGV_NULL(*this);
    }
    unsigned int length() {
        if (_STRINGV_BIG(*this)) {
            return _STRINGV_LENGTH(*this) & _INV_PAD;
        }
        return strlen(hex);
    }
    unsigned int capacity() {
        return _STRINGV_BIG(*this) ? _STRINGV_CAPACITY(*this) - 1 : 15;
    }
    typedef void (*Func)(char&, int);
    void forEach(Func func) {
        if (_STRINGV_BIG(*this)) {
            int idx = 0, length = _STRINGV_LENGTH(*this) & _INV_PAD;
            while (idx < length) {
                func(ptr[idx], idx);
                idx++;
            }
        }
        else {
            int idx = 0;
            while (hex[idx] != '\0') {
                func(hex[idx], idx, iter);
                idx++;
            }
        }
    }
public:
    // basic constructor
    stringv(){hex[0]=0x00;hex[15]=0x00;}
    explicit stringv(unsigned int N){
        if(N < 16){
            hex[0]=0x00;
            hex[15]0x00;
            return;
        }
        else{
            unsigned int _capacity = _NEXT_CAPACITY(N);
            ptr = new char[_capacity];
            _STRINGV_CAPACITY(*this) = _capacity;
            _STRINGV_LENGTH(*this) = 0;
        }
    }
    ~stringv(){_RESET_STRINGV();}
    stringv(const stringv& other){
        if(_STRINGV_BIG(other)){
            unsigned int _capacity = _STRINGV_CAPACITY(other);
            unsigned int _length = _STRINGV_LENGTH(other);
            ptr = new char[_capacity];
            _STRINGV_CAPACITY(*this) = _capacity;
            _STRINGV_LENGTH(*this) = _length;
            memcpy(ptr, other.ptr, (size_t)(_length & _INV_PAD) + 1);
        }else{memcpy(hex, other.hex, 16);}
    }
    stringv& operator=(const stringv& other){
        if(this == &other){return *this;}
        _RESET_STRINGV();
        if(_STRINGV_BIG(other)){
            unsigned int _capacity = _STRINGV_CAPACITY(other);
            unsigned int _length = _STRINGV_LENGTH(other);
            ptr = new char[_capacity];
            _STRINGV_CAPACITY(*this) = _capacity;
            _STRINGV_LENGTH(*this) = _length;
            memcpy(ptr, other.ptr, (size_t)(_length & _INV_PAD) + 1);
        }else{memcpy(hex, other.hex, 16);}
        return *this;
    }
    stringv(stringv&& other) noexcept {
        memcpy(hex, other.hex, 16);
        other.hex[15] = 0x00;
    }
    stringv& operator=(stringv&& other) noexcept {
        _RESET_STRINGV();
        memcpy(hex, other.hex, 16);
        other.hex[15] = 0x00;
        return *this;
    }
private:
    // compitable for C++11
    template<unsigned int N, unsigned int _TIMES>
    struct _ROUNDUP_POW2{
        static constexpr unsigned int value = _ROUNDUP_POW2<N|(N>>_TIMES), _TIMES*2>::value;
    };
    template<unsigned int N>
    struct _ROUNDUP_POW2<N, 16>{
        static constexpr unsigned int value = (N|(N>>16)) + 1;
    };
    template<unsigned int N, typename std::enable_if<(N&(N - 1)), bool>::type = true>
    static constexpr unsigned int _CAPACITY_COMPUTE() {
        return _ROUNDUP_POW2<N, 1>::value;
    }
    template<unsigned int N, typename std::enable_if<!(N&(N - 1)), bool>::type = true>
    static constexpr unsigned int _CAPACITY_COMPUTE() {
        return N * 2;
    }
public:
    // construct stringv in compiling
    template<unsigned int N, typename std::enable_if<(N < 16 && N != 1), bool>::type = true>
    static stringv _CREATE_STRINGV(const char* cptr) {
        stringv sv;
        memcpy(sv.hex, cptr, N);
        sv.hex[15] = 0x00;
        return sv;
    }
    template<unsigned int N, typename std::enable_if<(N > 16), bool>::type = true>
    static stringv _CREATE_STRINGV(const char* cptr) {
        constexpr unsigned int _capacity = stringv::_CAPACITY_COMPUTE<N - 1>();
        stringv sv;
        sv.ptr = new char[_capacity];
        _STRINGV_CAPACITY(sv) = _capacity;
        _STRINGV_LENGTH(sv) = (N - 1) | _PAD;
        memcpy(sv.ptr, cptr, N);
        return sv;
    }
    template<unsigned int N, typename std::enable_if<(N == 1), bool>::type = true>
    static stringv _CREATE_STRINGV(const char* cptr) {
        return stringv();
    }
    template<unsigned int N, typename std::enable_if<(N == 16), bool>::type = true>
    static stringv _CREATE_STRINGV(const char* cptr){
        stringv sv;
        memcpy(sv.hex, cptr, 16);
        return sv;
    }
#ifdef _AUTO_CSTR2SV_
    template<unsigned int N, typename std::enable_if<(N == 1), bool>::type = true>
    stringv(const char(&cptr)[N]){
        hex[0] = 0x00;
    }
    template<unsigned int N, typename std::enable_if<(N == 16), bool>::type = true>
    stringv(const char(&cptr)[N]){
        memcpy(hex, cptr, 16);
    }
    template<unsigned int N, typename std::enable_if<(N < 16 && N != 1), bool>::type = true>
    stringv(const char(&cptr)[N]){
        memcpy(hex, cptr, N);
        hex[15] = 0x00;
    }
    template<unsigned int N, typename std::enable_if<(N > 16), bool>::type = true>
    stringv(const char(&cptr)[N]){
        constexpr unsigned int _capacity = stringv::_CAPACITY_COMPUTE<N - 1>();
        ptr = new char[_capacity];
        _STRINGV_CAPACITY(*this) = _capacity;
        _STRINGV_LENGTH(*this) = (N - 1) | _PAD;
        memcpy(ptr, cptr, N);
    }
    template<unsigned int N, typename std::enable_if<(N == 1), bool>::type = true>
    stringv& operator=(const char(&cptr)[N]){
        _RESET_STRINGV();
        hex[0] = 0x00;
        return *this;
    }
    template<unsigned int N, typename std::enable_if<(N > 16), bool>::type = true>
    stringv& operator=(const char(&cptr)[N]){
        if(_STRINGV_BIG(*this)){
            if(_STRINGV_CAPACITY(*this) >= N){
                memcpy(ptr, cptr, N);
                _STRINGV_LENGTH(*this) = (N - 1) | _PAD;
                return *this;
            }else{delete[] ptr;}
        }
        constexpr unsigned int _capacity = stringv::_CAPACITY_COMPUTE<N - 1>();
        ptr = new char[_capacity];
        _STRINGV_CAPACITY(*this) = _capacity;
        _STRINGV_LENGTH(*this) = (N - 1) | _PAD;
        memcpy(ptr, cptr, N);
        return *this;
    }
    template<unsigned int N, typename std::enable_if<(N == 16), bool>::type = true>
    stringv& operator=(const char(&cptr)[N]){
        if(_STRINGV_BIG(*this)){
            memcpy(ptr, cptr, N);
            _STRINGV_LENGTH(*this) = (N - 1) | _PAD;
        }else{memcpy(ptr, cptr, N);}
        return *this;
    }
    template<unsigned int N, typename std::enable_if<(N <= 16 && N != 1), bool>::type = true>
    stringv& operator=(const char(&cptr)[N]){
        if(_STRINGV_BIG(*this)){
            memcpy(ptr, cptr, N);
            _STRINGV_LENGTH(*this) = (N - 1) | _PAD;
        }else{memcpy(hex, cptr, N);hex[15] = 0x00;}
        return *this;
    }
#else
    stringv(const char* cptr){
        unsigned int len = strlen(cptr);
        if(len > 16){
            unsigned int _capacity = this->_NEXT_CAPACITY(len);
            ptr = new char[_capacity];
            _STRINGV_CAPACITY(*this) = _capacity;
            _STRINGV_LENGTH(*this) = len | _PAD;
            memcpy(ptr, cptr, len + 1);
        }
        else{memcpy(hex, cptr, len + 1);hex[15] = 0x00;}
    }
    stringv& operator=(const char* cptr){
        bool self_big = _STRINGV_BIG(*this);
        if(cptr == (self_big ? ptr : hex)){return *this;}
        unsigned int len = strlen(cptr);
        if(self_big){
            if(_STRINGV_CAPACITY(*this) > len){
                memcpy(ptr, cptr, len + 1);
                _STRINGV_LENGTH(*this) = len | _PAD;
                return *this;
            }else{delete[] ptr;}
        }
        if(len > 16){
            unsigned int _capacity = this->_NEXT_CAPACITY(len);
            ptr = new char[_capacity];
            _STRINGV_CAPACITY(*this) = _capacity;
            _STRINGV_LENGTH(*this) = len | _PAD;
            memcpy(ptr, cptr, len + 1);
        }else{memcpy(hex, cptr, len + 1);hex[15] = 0x00;}
        return *this;
    }
#endif

    // size is strlen(), like "D2", size should be 2 instead of 3
    stringv& append(const char* cptr, unsigned int size){
        if(_STRINGV_BIG(*this)){
            unsigned int cur_len = _STRINGV_LENGTH(*this) & _INV_PAD;
            unsigned int new_len = cur_len + size;
            if(new_len >= _STRINGV_CAPACITY(*this)){
                unsigned int _capacity = this->_NEXT_CAPACITY(new_len);
                char* heap_ptr = new char[_capacity];
                _STRINGV_CAPACITY(*this) = _capacity;
                _STRINGV_LENGTH(*this) = new_len | _PAD;
                memcpy(heap_ptr, ptr, cur_len);
                memcpy(heap_ptr + cur_len, cptr, (size_t)size + 1);
                delete[] ptr;
                ptr = heap_ptr;
            }else{
                memcpy(ptr+cur_len, cptr, size);
                ptr[new_len] = '\0'; // prevent for overlap (ptr == cptr)
                _STRINGV_LENGTH(*this) = new_len | _PAD;
            }
        }
        else{
            unsigned int cur_len = strlen(hex);
            unsigned int new_len = cur_len + size;
            if(new_len >= 16){
                unsigned int _capacity = this->_NEXT_CAPACITY(new_len);
                char* heap_ptr = new char[_capacity];
                memcpy(heap_ptr, hex, cur_len);
                memcpy(heap_ptr + cur_len, cptr, (size_t)size + 1);
                ptr = heap_ptr;
                _STRINGV_CAPACITY(*this) = _capacity;
                _STRINGV_LENGTH(*this) = new_len | _PAD;
            }else{
                memcpy(hex + cur_len, cptr, size);
                hex[new_len] = '\0'; // prevent for overlap (hex == cptr)
            }
        }
        return *this;
    }
    
    stringv& operator+=(char c){
        if(_STRINGV_BIG(*this)){
            unsigned int _capacity = _STRINGV_CAPACITY(*this);
            unsigned int _length = _STRINGV_LENGTH(*this);
            if(_length + 1 == _capacity){
                char* heap_ptr = new char[(size_t)_capacity + _capacity];
                _STRINGV_CAPACITY(*this) = _capacity + _capacity;
                _STRINGV_LENGTH(*this) = _capacity | _PAD;
                memcpy(heap_ptr, ptr, _length);
                heap_ptr[_length] = c;
                heap_ptr[_length + 1] = '\0';
                delete[] ptr;
                ptr = heap_ptr;
            }else{
                ptr[_length] = c;
                ptr[_length+1] = '\0';
                _STRINGV_LENGTH(*this) = (_length + 1) | _PAD;
            }
        }
        else{
            unsigned int _length = strlen(hex);
            if(_length + 1 == 16){
                char* heap_ptr = new char[32];
                _STRINGV_CAPACITY(*this) = 32;
                _STRINGV_LENGTH(*this) = (16 | _PAD);
                memcpy(heap_ptr, hex, 16);
                heap_ptr[_length] = c;
                heap_ptr[_length + 1] = '\0';
                ptr = heap_ptr;
            }else{ptr[_length] = c;ptr[_length+1] = '\0';}
        }
        return *this;
    }
    stringv& operator+=(const stringv& other){
        if(_STRINGV_BIG(other)){
            return this->append(other.ptr, _STRINGV_LENGTH(other) & _INV_PAD);
        }
        else if(_STRINGV_NULL(other)){
            return *this;
        }
        else if(this == &other){
            unsigned int len = strlen(other.hex);
            unsigned int new_len = len + len;
            if(new_len >= 16){
                char* heap_ptr = new char[32];
                memcpy(heap_ptr, hex, len);
                memcpy(heap_ptr + len, hex, (size_t)len + 1);
                ptr = heap_ptr;
                _STRINGV_CAPACITY(*this) = 32;
                _STRINGV_LENGTH(*this) = new_len | _PAD;
            }
            else{
                memcpy(hex+len, hex, len);
                ptr[new_len] = '\0';    // prevent overlap
            }
        }
        else{
            return this->append(other.hex, strlen(other.hex));
        }
        return *this;
    }

#ifdef _AUTO_CSTR2SV_
    template<unsigned int N, typename std::enable_if<(N <= 16), bool>::type = true>
    stringv& operator+=(const char(&cptr)[N]){
        return this->append(cptr, N - 1);
    }
    template<unsigned int N, typename std::enable_if<(N > 16), bool>::type = true>
    stringv& operator+=(const char(&cptr)[N]){
        if(!_STRINGV_BIG(*this)){
            constexpr unsigned int _capacity = stringv::_CAPACITY_COMPUTE<N - 1>();
            char* heap_ptr = new char[_capacity];
            size_t i = 0;
            while(hex[i] != '\0'){
                heap_ptr[i] = hex[i];
                i++;
            }
            memcpy(heap_ptr + i, cptr, N);
            ptr = heap_ptr;
            _STRINGV_CAPACITY(*this) = _capacity;
            _STRINGV_LENGTH(*this) = (i + N - 1) | _PAD;
        }else{return this->append(cptr, N - 1);}
        return *this;
    }
#else
    stringv& operator+=(const char* cptr){
        unsigned int len = strlen(cptr);
        if(len != 0){
            return this->append(cptr, len);
        }
        return *this;
    }
#endif

#define _SV_ALLOC() \
    {sv.ptr=new char[_capacity];_STRINGV_CAPACITY(sv)=_capacity;_STRINGV_LENGTH(sv)=new_len|_PAD;}
    stringv operator+(const stringv& other){
        bool other_big = _STRINGV_BIG(other);
        if(!other_big && _STRINGV_NULL(other)){
            return *this;
        }
        stringv sv;
        if(_STRINGV_BIG(*this)){
            unsigned cur_len = _STRINGV_LENGTH(*this) & _INV_PAD;
            unsigned int cat_len;
            const char* other_src;
            if(other_big){
                cat_len = _STRINGV_LENGTH(other) & _INV_PAD;
                other_src = other.ptr;
            }
            else{
                cat_len = strlen(other.hex);
                other_src = other.hex;
            }
            unsigned int new_len = cur_len + cat_len;
            unsigned int _capacity = sv._NEXT_CAPACITY(new_len);
            _SV_ALLOC();
            memcpy(sv.ptr, ptr, cur_len);
            memcpy(sv.ptr + cur_len, other_src, (size_t)cat_len + 1);
        }
        else if(other_big){
            unsigned int cat_len = _STRINGV_LENGTH(other) & _INV_PAD;
            unsigned int cur_len = strlen(hex);
            unsigned int new_len = cur_len + cat_len;
            unsigned int _capacity = sv._NEXT_CAPACITY(new_len);
            _SV_ALLOC();
            memcpy(sv.ptr, hex, cur_len);
            memcpy(sv.ptr + cur_len, other.ptr, (size_t)cat_len + 1);
        }
        else{
            unsigned cur_len, cat_len;
            if(this == &other){
                cur_len = cat_len = strlen(other.hex);
            }
            else{
                cat_len = strlen(other.hex);
                cur_len = strlen(hex);
            }
            unsigned int new_len = cur_len + cat_len;
            if(cat_len + cur_len >= 16){
                sv.ptr = new char[32];
                _STRINGV_CAPACITY(sv) = 32;
                _STRINGV_LENGTH(sv) = new_len | _PAD;
                memcpy(sv.ptr, hex, cur_len);
                memcpy(sv.ptr + cur_len, other.hex, (size_t)cat_len + 1);
            }
            else{
                memcpy(sv.hex, hex, cur_len);
                memcpy(sv.hex + cur_len, other.hex, (size_t)cat_len + 1);
            }
        }
        return sv;
    }
#undef _SV_ALLOC

#ifdef _AUTO_CSTR2SV_
    template<unsigned int N, typename std::enable_if<(N <= 16), bool>::type = true>
    stringv operator+(const char(&cptr)[N]){
        stringv sv;
        unsigned int cur_len;
        const char* src;
        if(_STRINGV_BIG(*this)){
            src = ptr;
            cur_len = _STRINGV_LENGTH(*this) & _INV_PAD;
        }
        else{
            src = hex;
            cur_len = strlen(hex);
        }
        unsigned int new_len = cur_len + N - 1;
        if(new_len >= 16){
            unsigned int _capacity = sv._NEXT_CAPACITY(new_len);
            sv.ptr = new char[_capacity];
            _STRINGV_CAPACITY(sv) = _capacity;
            _STRINGV_LENGTH(sv) = new_len | _PAD;
            memcpy(sv.ptr, src, cur_len);
            memcpy(sv.ptr + cur_len, cptr, N);
        }
        else{
            memcpy(sv.hex, src, cur_len);
            memcpy(sv.hex + cur_len, cptr, N);
        }
        return sv;
    }
    template<unsigned int N, typename std::enable_if<(N > 16), bool>::type = true>
    stringv operator+(const char(&cptr)[N]){
        stringv sv;
        if(!_STRINGV_BIG(*this)){
            constexpr unsigned int _capacity = stringv::_CAPACITY_COMPUTE<N - 1>();
            sv.ptr = new char[_capacity];
            size_t i = 0;
            while(hex[i] != '\0'){
                sv.ptr[i] = hex[i];
                i++;
            }
            memcpy(sv.ptr + i, cptr, N);
            _STRINGV_CAPACITY(sv) = _capacity;
            _STRINGV_LENGTH(sv) = (i + N - 1) | _PAD;
        }
        else{
            unsigned int cur_len = _STRINGV_LENGTH(*this) & _INV_PAD;
            unsigned int _capacity = sv._NEXT_CAPACITY(cur_len + N - 1);
            sv.ptr = new char[_capacity];
            _STRINGV_CAPACITY(sv) = _capacity;
            _STRINGV_LENGTH(sv) = (cur_len + N - 1) | _PAD;
            memcpy(sv.ptr, ptr, cur_len);
            memcpy(sv.ptr + cur_len, cptr, N);
        }
        return sv;
    }
#else
    stringv operator+(const char* cptr){
        unsigned int cat_len = strlen(cptr);
        unsigned int cur_len;
        const char* src;
        if(_STRINGV_BIG(*this)){
            src = ptr;
            cur_len = _STRINGV_LENGTH(*this) & _INV_PAD;
        }else{
            src = hex;
            cur_len = strlen(hex);
        }
        stringv sv;
        unsigned int new_len = cur_len + cat_len;
        if(new_len >= 16){
            unsigned int _capacity = sv._NEXT_CAPACITY(new_len);
            sv.ptr = new char[_capacity];
            _STRINGV_CAPACITY(sv) = _capacity;
            _STRINGV_LENGTH(sv) = new_len | _PAD;
            memcpy(sv.ptr, src, cur_len);
            memcpy(sv.ptr + cur_len, cptr, (size_t)cat_len + 1);
        }
        else{
            memcpy(sv.hex, src, cur_len);
            memcpy(sv.hex + cur_len, cptr, (size_t)cat_len + 1);
        }
        return sv;
    }
#endif

    char& operator=(unsigned int idx){
        return _STRINGV_BIG(*this) ? ptr[idx] : hex[idx];
    }

#define _ALLOC() \
    unsigned int _capacity=sv._NEXT_CAPACITY(new_len); \
    sv.ptr = new char[_capacity]; \
    _STRINGV_CAPACITY(sv)=_capacity; \
    _STRINGV_LENGTH(sv)=new_len|_PAD;

#define _SV_ALLOC() \
    if(new_len>16){_ALLOC();dst=sv.ptr;} \
    else if(new_len<=0){return stringv();} \
    else{dst=sv.hex;}

    stringv operator()(int begin, int end, int step=1){
        if(step == 0){return stringv();}
        int _length;
        const char* src;
        if(_STRINGV_BIG(*this)){
            src = ptr;
            _length = *((int*)(hex + 12)) & _INV_PAD;
        }
        else{
            src = hex;
            _length = strlen(src);
        }
        stringv sv;
        if(step > 0){
            if(begin != _BEGIN){
                begin = (begin >= 0 ? begin : (_length + begin <= 0 ? 0 : _length + begin));
            }else{begin = 0;}
            if(end != _BEGIN){
                if(end >= 0){
                    end = (end > _length ? _length : end);
                }
                else{
                    end = (_length + end <= 0 ? 0 : _length + end);
                }
            }else{end = 0;}
            int new_len = end - begin;
            char* dst;
            _SV_ALLOC();
            int i = 0;
            for(; begin < end; begin+=step){
                dst[i++] = src[begin];
            }
            dst[i] = '\0';
        }
        else{
            if(begin != _BEGIN){
                if(begin >= 0){
                    begin = (begin >= _length ? _length - 1 : begin);
                }
                else{
                    begin = (_length + begin <= 0 ? 0 : _length + begin);
                }
            }else{begin = -1;}
            if(end != _BEGIN){
                end = (end >= 0 ? end : (_length + end <= 0 ? 0 : _length + end));
            }else{end = -1;}
            int new_len = begin - end;
            char* dst;
            _SV_ALLOC();
            int i = 0;
            for(; begin > end ; begin+=step){
                dst[i++] = src[begin];
            }
            dst[i] = '\0';
        }
        return sv;
    }
#undef _ALLOC
#undef _SV_ALLOC

    bool operator==(const stringv& other){
        bool self_big = _STRINGV_BIG(*this);
        bool other_big = _STRINGV_BIG(other);
        if(self_big != other_big){
            return false;
        }
        if(self_big){
            unsigned int self_length = _STRINGV_LENGTH(*this);
            unsigned int other_length = _STRINGV_LENGTH(other);
            if(self_length != other_length){
                return false;
            }
            return !memcmp(ptr, other.ptr, self_length);
        }
        return !strcmp(hex, other.hex);
    }

#ifdef _AUTO_CSTR2SV_
    template<unsigned int N, typename std::enable_if<(N <= 16), bool>::type = true>
    bool operator==(const char(&cptr)[N]){
        if(_STRINGV_BIG(*this)){
            return false;
        }
        return !memcmp(hex, cptr, N-1);
    }

    template<unsigned int N, typename std::enable_if<(N > 16), bool>::type = true>
    bool operator==(const char(&cptr)[N]){
        if(!_STRINGV_BIG(*this)){
            return false;
        }
        unsigned int cur_len = _STRINGV_LENGTH(*this) & _INV_PAD;
        if(cur_len != N){
            return false;
        }
        return !memcmp(ptr, cptr, N-1);
    }
#else
    bool operator==(const char* cptr){
        return _STRINGV_BIG(*this) ? !strcmp(ptr, cptr) : !strcmp(hex, cptr);
    }

#endif

private:
    static unsigned int _NEXT_CAPACITY(unsigned int new_len){
        if((new_len & (new_len - 1))){
            new_len |= new_len >> 1;
            new_len |= new_len >> 2;
            new_len |= new_len >> 4;
            new_len |= new_len >> 8;
            new_len |= new_len >> 16;
            return new_len + 1;
        }
        return new_len + new_len;
    }
};

#undef _STRINGV_BIG
#undef _STRINGV_NULL
#undef _RESET_STRINGV
#undef _STRINGV_LENGTH
#undef _STRINGV_CAPACITY
#endif