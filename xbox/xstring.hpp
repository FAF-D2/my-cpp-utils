#ifndef xstring_hpp
#define xstring_hpp

#include<utility>
#include<type_traits>
#include<cstdint>
#include<cstring>
#include<limits>
#include<cmath>
#ifdef _WIN32
#include<xstring>
#pragma warning(disable:4996)
#define __builtin_popcount __popcnt
#define __builtin_memchr __builtin_char_memchr
#define __builtin_memcmp __builtin_memcmp
#define __builtin_clzll __lzcnt64
#define __builtin_ctz   _tzcnt_u32
#endif

#if ((defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) || __cplusplus >= 201703L)
    #define INLINE_VARIABLE inline
    #define CONSTEXPR_FUNCTION constexpr
    #ifndef XSTRING_CPP17
        #define XSTRING_CPP17
    #endif
#elif ((defined(_MSVC_LANG) && _MSVC_LANG >= 201402L) || __cplusplus >= 201402L)
    #define INLINE_VARIABLE
    #define CONSTEXPR_FUNCTION constexpr
    #ifndef XSTRING_CPP14
        #define XSTRING_CPP14
    #endif
#else
    #define INLINE_VARIABLE
    #define CONSTEXPR_FUNCTION
    #ifndef XSTRING_CPP11
        #define XSTRING_CPP11
    #endif
#endif

#if defined(_MSC_VER)
#include<intrin.h>
#if defined(_M_AMD64)
#pragma intrinsic(_umul128)
#endif
#endif

// TODO split
// TODO: optimize to_double to_integral
union xstring{
private:
    char hex[16];
    struct{
        char* xptr;
        alignas(8) uint32_t xcapacity;
        uint32_t xlength;
    };
    static constexpr uint32_t INV_PAD = 0x7FFFFFFFU;
    static constexpr uint32_t PAD = 0x80000000U;

    template<bool isconst, size_t cmp_len>
    class Spliter;

public:
    template<bool isconst = true>
    class range;

    template<bool isconst = false>
    class iterator;
    using const_iterator = iterator<true>;
    template<bool isconst = false>
    class riterator;
    using const_riterator = riterator<true>;

    template<bool const_ptr, typename data>
    struct const_traits{
        using data_type = typename std::add_const<data>::type;
        using pointer_type = typename std::add_const<data>::type*;
    };
    template<typename data>
    struct const_traits<false, data>{
        using data_type = data;
        using pointer_type = data*;
    };

    template<bool isconst>
    class range
    {
        friend union xstring;
        using void_type = typename const_traits<isconst, void>::data_type;
        using data_type = typename const_traits<isconst, char>::data_type;
        using const_data_type = typename std::add_const<data_type>::type;
        data_type* p;
        size_t len;
    public:
        using iterator = xstring::iterator<isconst>;
        using const_iterator = xstring::const_iterator;
        using riterator = xstring::riterator<isconst>;
        using const_riterator = xstring::const_riterator;

        static constexpr bool immutable = isconst;
        constexpr range(data_type* begin, data_type* end) noexcept: p(begin), len(end - begin){}
        constexpr range(data_type* begin, size_t size) noexcept: p(begin), len(size){}
        constexpr range(void_type* begin, size_t size) noexcept: p(static_cast<data_type*>(begin)), len(size){}
        constexpr range() noexcept: p(nullptr), len(0){}
        template<size_t N>
        constexpr range(const char(&cptr)[N]) noexcept: p(cptr), len(N-1){
            static_assert(range::immutable, "range::range(const char(&)[N]) range should be range<true>");
        }
        constexpr range(iterator begin, iterator end) 
            noexcept: p(begin.ptr), len(end.ptr - begin.ptr)
        {}
        constexpr range(riterator begin,  riterator end)
            noexcept: p(end.ptr + 1), len(begin.ptr - end.ptr)
        {}
        constexpr range(const range& other) = default;

        // simple functions
        operator xstring() const noexcept{ return xstring(p, static_cast<uint32_t>(len)); }
        operator range<true>() const noexcept{ return range<true>(this->p, this->len); }
        data_type& operator[](size_t idx) noexcept{ return p[idx]; }
        constexpr const_data_type& operator[](size_t idx) const noexcept{ return p[idx]; }
        template<typename size_type = size_t>
        constexpr size_type size() const noexcept{ return static_cast<size_type>(len); }
        constexpr const_data_type* data() const noexcept{ return p; }
        constexpr bool empty() const noexcept { return len == 0; }

        iterator begin() noexcept{ return iterator(p); }
        constexpr const_iterator begin() const noexcept{ return const_iterator(p); }
        constexpr const_iterator cbegin() const noexcept{ return const_iterator(p); }  
        iterator end() noexcept{ return iterator(p + len); }
        constexpr const_iterator end() const noexcept{ return const_iterator(p + len); }
        constexpr const_iterator cend() const noexcept{ return const_iterator(p + len); }

        riterator rbegin() noexcept{ return riterator(p + len - 1); }
        constexpr const_riterator rbegin() const noexcept{ return const_riterator(p + len - 1); }
        constexpr const_riterator crbegin() const noexcept{ return const_riterator(p + len - 1); }
        riterator rend() noexcept{ return riterator(p - 1); }
        constexpr const_riterator rend() const noexcept{ return const_riterator(p - 1); }
        constexpr const_riterator crend() const noexcept{ return const_riterator(p - 1); }

        static constexpr range substr(iterator begin, iterator end) noexcept{
            return range<isconst>(begin, end);
        }
        static constexpr range substr(riterator begin, riterator end) noexcept{
            return range<isconst>(begin, end);
        }

        // practical functions
        xstring to_string() const noexcept{
            return xstring(p, static_cast<uint32_t>(len));
        }
        CONSTEXPR_FUNCTION size_t count(char ch) const noexcept{
            size_t counter = 0;
            for(size_t i = 0; i < len; i++) if(p[i] == ch) ++counter;
            return counter;
        }
        void toupper() noexcept{
            static_assert(!range::immutable, "range::toupper(): range<false> cannot use this function");
            xstring::xtoupper(p, p+len);
        }
        void tolower() noexcept{
            static_assert(!range::immutable, "range::tolower(): range<false> cannot use this function");
            xstring::xtolower(p, p+len);
        }
        template<typename... Args>
        xstring join(Args&&... args) const noexcept{
            static_assert(sizeof...(Args) > 0, "range::join(): parameters cannot be empty!");
            return xstring::xjoin(p, static_cast<uint32_t>(len), std::forward<Args>(args)...);
        }
        template<typename... CHS>
        Spliter<isconst, sizeof...(CHS) + 1> split(const char blank = ' ', CHS... chs) noexcept{
            return xstring::Spliter<isconst, sizeof...(CHS) + 1>(p, p + len, blank, chs...);
        }
        template<typename... CHS>
        constexpr Spliter<true, sizeof...(CHS) + 1> split(const char blank = ' ', CHS... chs) const noexcept{
            return xstring::Spliter<true, sizeof...(CHS) + 1>(p, p + len, blank, chs...);
        }
        template<bool C>
        constexpr bool starts_with(range<C> other, size_t start = 0) const noexcept{
            return other.len + start > this->len ? false : !__builtin_memcmp(p + start, other.p, other.len);
        }
        constexpr bool starts_with(range<true> other, size_t start = 0) const noexcept{
            return this->starts_with<true>(other, start);
        }
        template<bool C>
        constexpr bool ends_with(range<C> other, size_t end = 0) const noexcept{
            return (this->p + len - other.len - end) < this->p ? 
            false : !__builtin_memcmp((this->p + len - other.len - end), other.p, other.len);
        }
        constexpr bool ends_with(range<true> other, size_t end = 0) const noexcept{
            return this->ends_with<true>(other, end);
        }

        template<typename... CHS>
        CONSTEXPR_FUNCTION range strip(const char blank = ' ', CHS... chs) const noexcept{
            constexpr size_t cmp_len = sizeof...(CHS) + 1;
            const char cmp[] = {blank, static_cast<char>(chs)...};
            constexpr bool issingle = cmp_len ==  1;
            data_type* head = this->p;
            data_type* tail = this->p + this->len - 1;
            while(true){
                const char ch = *head;
                if(head <= tail && xstring::charcmp<issingle>(ch, cmp, cmp_len)) ++head; else break;
            }
            while(true){
                if(xstring::charcmp<issingle>(*tail, cmp, cmp_len)) --tail; else break;
            }
            return range(head, tail + 1);
        }
        template<typename... CHS>
        CONSTEXPR_FUNCTION range lstrip(const char blank = ' ', CHS... chs) const noexcept{
            constexpr size_t cmp_len = sizeof...(CHS) + 1;
            const char cmp[] = {blank, static_cast<char>(chs)...};
            constexpr bool issingle = cmp_len == 1;
            data_type* head = this->p;
            data_type* end = this->p + this->len;
            while(true){
                const char ch = *head;
                if(head >= end) return range();
                if(xstring::charcmp<issingle>(ch, cmp, cmp_len)) ++head; else break;
            }
            return range(head, end);
        }
        template<typename... CHS>
        CONSTEXPR_FUNCTION range rstrip(const char blank = ' ', CHS... chs) const noexcept{
            constexpr size_t cmp_len = sizeof...(CHS) + 1;
            const char cmp[] = {blank, static_cast<char>(chs)...};
            constexpr bool issingle = cmp_len == 1;
            data_type* head = this->p;
            data_type* tail = this->p + this->len - 1;
            while(true){
                if(tail < head) return range();
                if(xstring::charcmp<issingle>(*tail, cmp, cmp_len)) --tail; else break;
            }
            return range(head, tail + 1);
        }

        // operators
        xstring concat(const void* ptr, uint32_t size) const noexcept{
            return xstring::xconcat(this->p, static_cast<uint32_t>(this->len), ptr, size);
        }
        template<bool C>
        xstring operator+(range<C> other) const noexcept{
            return xstring::xconcat(p, static_cast<uint32_t>(len), other.p, static_cast<uint32_t>(other.len));
        }
        xstring operator+(range<true> other) const noexcept{
            return this->operator+<true>(other);
        }

        constexpr bool equals(const void* ptr, size_t size) const noexcept{
            return len != size ? false : !__builtin_memcmp(p, ptr, len);
        }
        template<bool C>
        constexpr bool operator==(range<C> other) const noexcept{
            return this->len != other.len ? false : !__builtin_memcmp(p, other.p, len);
        }
        constexpr bool operator==(range<true> other) const noexcept{
            return this->operator==<true>(other);
        }
        template<class T>
        constexpr bool operator!=(T&& other) const noexcept{ return !(*this == std::forward<T>(other)); }

        constexpr int compare(const void* ptr, size_t size) const noexcept{
        #ifndef XSTRING_CPP11
            bool cmp1 = this->len < size;
            int cmp2 = __builtin_memcmp(p, ptr, cmp1 ? len : size);
            return cmp2 != 0 ? cmp2 : (cmp1 ? -1 : len == size ? 0 : 1);
        #else
            return __builtin_memcmp(p, ptr, len < size ? len : size) != 0 ?
                    __builtin_memcmp(p, ptr, len < size ? len : size) :
                    (len < size ? -1 : len == size ? 0 : 1);
        #endif
        }
        template<bool C>
        constexpr int compare(range<C> other) const noexcept{
        #ifndef XSTRING_CPP11
            bool cmp1 = len < other.len;
            int cmp2 = __builtin_memcmp(p, other.p, cmp1 ? len : other.len);
            return cmp2 != 0 ? cmp2 : (cmp1 ? -1 : len == other.len ? 0 : 1);
        #else
            return __builtin_memcmp(p, other.p, (len < other.len) ? len : other.len) != 0 ?
                    __builtin_memcmp(p, other.p, (len < other.len) ? len : other.len) :
                    (len < other.len ? -1 : len == other.len ? 0 : 1);
        #endif
        }
        constexpr int compare(range<true> other) const noexcept{
            return this->compare<true>(other);
        }
        template<class T>
        constexpr bool operator<(T&& other) const noexcept{
            return this->compare(std::forward<T>(other)) < 0;
        }
        template<class T>
        constexpr bool operator>(T&& other) const noexcept{
            return this->compare(std::forward<T>(other)) > 0;
        }
        template<class T>
        constexpr bool operator<=(T&& other) const noexcept{
            return this->compare(std::forward<T>(other)) <= 0;
        }
        template<class T>
        constexpr bool operator>=(T&& other) const noexcept{
            return this->compare(std::forward<T>(other)) >= 0;
        }

        template<size_t N>
        iterator find(const char(&cptr)[N]) noexcept{
            static_assert(N != 1, "range::find(): meaningless to find a null string");
            using void_type = typename const_traits<isconst, void>::pointer_type;
            using ret_type = typename const_traits<isconst, char>::pointer_type;
            constexpr size_t cmp_len = N - 1;
            void_type res = cmp_len == 1 ? __builtin_memchr(p, cptr[0], len) : 
                xstring::memmem<isconst>(p, len, cptr, cmp_len);
            return res != nullptr ? static_cast<ret_type>(res) : p + this->len;
        }
        template<size_t N>
        CONSTEXPR_FUNCTION const_iterator find(const char(&cptr)[N]) const noexcept{
            static_assert(N != 1, "range::find(): meaningless to find a null string");
            constexpr size_t cmp_len = N - 1;
            const char* res = cmp_len == 1 ? static_cast<const char*>(__builtin_memchr(p, cptr[0], len)) :
                xstring::memmem<true>(p, len, cptr, cmp_len);
            return res != nullptr ? res : p + this->len;
        }
        template<bool C>
        iterator find(range<C> other) noexcept{
            using ret_type = typename const_traits<isconst, char>::pointer_type;
            ret_type res = static_cast<ret_type>(xstring::memmem<isconst>(p, len, other.p, other.len));
            return res != nullptr ? res : p + this->len;
        }
        iterator find(range<true> other) noexcept{
            return this->find<true>(other);
        }
        template<bool C>
        CONSTEXPR_FUNCTION const_iterator find(range<C> other) const noexcept{
            const char* res = static_cast<const char*>(xstring::memmem<true>(p, len, other.p, other.len));
            return res != nullptr ? res : p + this->len;
        }
        CONSTEXPR_FUNCTION const_iterator find(range<true> other) const noexcept{
            return this->find<true>(other);
        }
    };


    template<bool isconst>
    class iterator
    {
        friend union xstring;
        using data_type = typename const_traits<isconst, char>::data_type;
        using const_data_type = typename std::add_const<data_type>::type;
        data_type* ptr;
    public:
        static constexpr bool immutable = isconst;
        constexpr iterator(data_type* ptr) noexcept: ptr(ptr){}
        constexpr iterator(const iterator& other) = default;
        CONSTEXPR_FUNCTION iterator& operator++() noexcept{ ++ptr; return *this; }
        CONSTEXPR_FUNCTION iterator operator++(int) noexcept{ return ptr++; }
        CONSTEXPR_FUNCTION iterator& operator--() noexcept{ --ptr; return *this; }
        CONSTEXPR_FUNCTION iterator operator--(int) noexcept{ return ptr--; }
        constexpr iterator operator+(size_t i) const noexcept{ return ptr+i; }
        constexpr iterator operator-(size_t i) const noexcept{ return ptr-i; }
        CONSTEXPR_FUNCTION iterator& operator+=(size_t i) noexcept{ ptr+=i; return *this; }
        CONSTEXPR_FUNCTION iterator& operator-=(size_t i) noexcept{ ptr-=i; return *this; }
        template<bool C>
        constexpr bool operator==(const iterator<C>& other) const noexcept{ return ptr == other.ptr; }
        template<bool C>
        constexpr bool operator!=(const iterator<C>& other) const noexcept{ return ptr != other.ptr; }
        template<bool C>
        constexpr bool operator>(const iterator<C>& other) const noexcept{ return ptr > other.ptr; }
        template<bool C>
        constexpr bool operator<(const iterator<C>& other) const noexcept{ return ptr < other.ptr; }
        template<bool C>
        constexpr bool operator>=(const iterator<C>& other) const noexcept{ return ptr >= other.ptr; }
        template<bool C>
        constexpr bool operator<=(const iterator<C>& other) const noexcept{ return ptr <= other.ptr; }
        CONSTEXPR_FUNCTION data_type& operator*() noexcept{ return *ptr; }
        constexpr const_data_type& operator*() const noexcept{ return *ptr; }
    };

    template<bool isconst>
    class riterator
    {
        friend union xstring;
        using data_type = typename const_traits<isconst, char>::data_type;
        using const_data_type = typename std::add_const<data_type>::type;
        data_type* ptr;
    public:
        static constexpr bool immutable = isconst;
        constexpr riterator(data_type* ptr) noexcept: ptr(ptr){}
        constexpr riterator(const riterator& other) = default;
        CONSTEXPR_FUNCTION riterator& operator++() noexcept{ --ptr; return *this; }
        CONSTEXPR_FUNCTION riterator operator++(int) noexcept{ return ptr--; }
        CONSTEXPR_FUNCTION riterator& operator--() noexcept{ ++ptr; return *this; }
        CONSTEXPR_FUNCTION riterator operator--(int) noexcept{ return ptr++; }
        constexpr riterator operator+(size_t i) const noexcept{ return ptr-i; }
        constexpr riterator operator-(size_t i) const noexcept{ return ptr+i; }
        CONSTEXPR_FUNCTION riterator& operator+=(size_t i) noexcept{ ptr-=i; return *this; }
        CONSTEXPR_FUNCTION riterator& operator-=(size_t i) noexcept{ ptr+=i; return *this; }
        template<bool C>
        constexpr bool operator==(const riterator<C>& other) const noexcept{ return ptr == other.ptr; }
        template<bool C>
        constexpr bool operator!=(const riterator<C>& other) const noexcept{ return ptr != other.ptr; }
        template<bool C>
        constexpr bool operator>(const iterator<C>& other) const noexcept{ return ptr < other.ptr; }
        template<bool C>
        constexpr bool operator<(const iterator<C>& other) const noexcept{ return ptr > other.ptr; }
        template<bool C>
        constexpr bool operator>=(const iterator<C>& other) const noexcept{ return ptr <= other.ptr; }
        template<bool C>
        constexpr bool operator<=(const iterator<C>& other) const noexcept{ return ptr >= other.ptr; }
        CONSTEXPR_FUNCTION data_type& operator*() noexcept{ return *ptr; }
        constexpr const_data_type& operator*() const noexcept{ return *ptr; }
    };


    iterator<false> begin() noexcept{ return this->big() ? xptr : hex; }
    const_iterator begin() const noexcept{ return this->big() ? xptr : hex; }
    const_iterator cbegin() const noexcept{ return this->big() ? xptr : hex; }
    iterator<false> end() noexcept { return this->big() ? xptr + (xlength & INV_PAD) : hex + hex[15]; }
    const_iterator end() const noexcept { return this->big() ? xptr + (xlength & INV_PAD) : hex + hex[15]; }
    const_iterator cend() const noexcept { return this->big() ? xptr + (xlength & INV_PAD) : hex + hex[15]; }

    riterator<false> rbegin() noexcept { return this->big() ? xptr + (xlength & INV_PAD) - 1 : hex + hex[15] - 1; }
    const_riterator rbegin() const noexcept { return this->big() ? xptr + (xlength & INV_PAD) - 1 : hex + hex[15] - 1; }
    const_riterator crbegin() const noexcept { return this->big() ? xptr + (xlength & INV_PAD) - 1 : hex + hex[15] - 1; }
    riterator<false> rend() noexcept { return this->big() ? xptr - 1 : hex - 1; }
    const_riterator rend() const noexcept { return this->big() ? xptr - 1 : hex - 1; }
    const_riterator crend() const noexcept { return this->big() ? xptr - 1 : hex - 1; }

    template<bool isconst>
    static range<isconst> substr(iterator<isconst> begin, iterator<isconst> end) noexcept{
        return range<isconst>(begin, end);
    }
    template<bool isconst>
    static range<isconst> substr(riterator<isconst> begin, riterator<isconst> end) noexcept{
        return range<isconst>(begin, end);
    }

    template<typename size_type = size_t>
    size_type size() const noexcept{
        return this->big() ? static_cast<size_type>(xlength & INV_PAD) : static_cast<size_type>(hex[15]);
    }
    template<typename size_type = size_t>
    size_type capacity() const noexcept{
        return this->big() ? static_cast<size_type>(this->xcapacity) : 14;
    }
    const char* c_str() const noexcept{ return this->big() ? xptr : hex; }
    const char* data() const noexcept{ return this->big() ? xptr : hex; }
    bool small() const noexcept{ return !(this->hex[15] & 0x80); }
    bool big() const noexcept{ return this->hex[15] & 0x80; }
    bool empty() const noexcept{ return this->big() ? this->xlength == PAD : this->hex[15] == 0x00; }
    void flush() noexcept { if(this->big()) delete[] xptr; hex[15] = 0x00; }
    char& operator[](size_t idx) noexcept{ return this->big() ? xptr[idx]: hex[15]; }
    const char& operator[](size_t idx) const noexcept { return this->big() ? xptr[idx] : hex[idx]; }
    void pop_back(uint32_t i = 1) noexcept{
        if(this->big()){
            uint32_t new_len = (this->xlength & INV_PAD) - i;
            this->xptr[new_len] = '\0';
            this->xlength = new_len | PAD;
        }
        else{
            uint32_t new_len = this->hex[15] - i;
            this->hex[15] = static_cast<char>(new_len);
            this->hex[new_len] = '\0';
        }
    }
    void clear() noexcept{
        if(this->big()){
            xptr[0] = '\0';
            this->xlength = PAD;
        }
        else{
            hex[0] = '\0';
            hex[15] = 0x00;
        }
    }
    void swap(xstring& other) noexcept{
        std::swap(this->hex, other.hex);
    }
    static void swap(xstring& lhs, xstring& rhs) noexcept{
        lhs.swap(rhs);
    }

    template<bool isconst, size_t N>
    iterator<false> insert(iterator<isconst>& begin, const char(&cptr)[N]) noexcept{
        static_assert(!isconst, "xstring::insert(): cannot use const iterator to insert");
        static_assert(N != 1, "xstring::insert(): meaningless to insert null string");
        constexpr size_t size = N - 1;
        return this->xinsert(begin, cptr, size);
    }
    template<bool isconst>
    iterator<false> insert(iterator<isconst>& begin, const range<true> other) noexcept{
        static_assert(!isconst, "xstring::insert(): cannot use const iterator to insert");
        return xstring::xinsert(begin, other.p, other.len);
    }
    template<bool isconst, size_t N>
    iterator<false> insert(iterator<isconst>&& begin, const char(&cptr)[N]) noexcept{
        static_assert(!isconst, "xstring::insert(): cannot use const iterator to insert");
        static_assert(N != 1, "xstring::insert(): meaningless to insert null string");
        constexpr size_t size = N - 1;
        return this->xinsert(begin, cptr, size);
    }
    template<bool isconst>
    iterator<false> insert(iterator<isconst>&& begin, const range<true> other) noexcept{
        static_assert(!isconst, "xstring::insert(): cannot use const iterator to insert");
        return xstring::xinsert(begin, other.p, other.len);
    }

    template<bool isconst>
    iterator<false> erase(iterator<isconst> pos) noexcept{
        static_assert(!isconst, "xstring::erase(): cannot use const iterator to erase");
        size_t length, mv_len;
        if(this->big()){
            length = (this->xlength & INV_PAD) - 1;
            mv_len = length - (pos.ptr - this->xptr);
            this->xlength = static_cast<uint32_t>(length) | PAD;
        }
        else{
            length = static_cast<size_t>(hex[15] - 1);
            mv_len = length - (pos.ptr - this->hex);
            --this->hex[15];
        }
        memmove(pos.ptr, pos.ptr + 1, mv_len + 1);
        return pos;
    }
    template<bool isconst>
    iterator<false> erase(range<isconst> range) noexcept{
        static_assert(!isconst, "xstring::erase(): cannot use const iterator to erase");
        size_t length, mv_len;
        if(this->big()){
            length = static_cast<size_t>(this->xlength & INV_PAD) - range.len;
            mv_len = length - (range.p - this->xptr);
            this->xlength = static_cast<uint32_t>(length) | PAD;
        }
        else{
            length = static_cast<size_t>(hex[15]) - range.len;
            mv_len = length - (range.p - this->hex);
            this->hex[15] = static_cast<char>(length);
        }
        memmove(range.p, range.p+range.len, mv_len + 1);
        return range.p;
    }
    template<bool isconst>
    iterator<false> erase(iterator<isconst> begin, iterator<isconst> end) noexcept{
        return this->erase<isconst>(range<isconst>(begin, end));
    }

    template<size_t N>
    iterator<false> find(const char(&cptr)[N]) noexcept{
        static_assert(N != 1, "xstring::find(): meaningless to find a null string");
        constexpr size_t size = N - 1;
        uint32_t length;
        char* ptr = this->xstring_data(&length);
        void* res = size == 1 ? memchr(ptr, cptr[0], length) : 
            xstring::memmem<false>(ptr, length, cptr, size);
        return res != nullptr ? static_cast<char*>(res) : ptr + length; 
    }
    template<size_t N>
    const_iterator find(const char(&cptr)[N]) const noexcept{
        static_assert(N != 1, "xstring::find(): meaningless to find a null string");
        constexpr size_t size = N - 1;
        uint32_t length;
        const char* ptr = this->xstring_data(&length);
        const char* res = size == 1 ? memchr(ptr, cptr[0], length) : 
            xstring::memmem<true>(ptr, length, cptr, size);
        return res != nullptr ? res : ptr + length;
    }
    iterator<false> find(const range<true> other) noexcept{
        uint32_t length;
        char* ptr = this->xstring_data(&length);
        char* res = xstring::memmem<false>(ptr, length, other.p, other.len);
        return res != nullptr ? res : ptr + length;
    }
    const_iterator find(const range<true> other) const noexcept{
        uint32_t length;
        const char* ptr = this->xstring_data(&length);
        const char* res = xstring::memmem<true>(ptr, length, other.p, other.len);
        return res != nullptr ? res : ptr + length;
    }

    template<bool isconst>
    range<false> replace(iterator<isconst>& begin, iterator<isconst>& end, const range<true> other) noexcept{
        static_assert(!isconst, "xstirng::replace(): cannot use const iterator to replace");
        size_t old_len = end.ptr - begin.ptr;
        size_t new_len = other.len;
        if(old_len < new_len){
            const char* ptr;
            size_t capacity, cur_len;
            if(this->big()){
                ptr = this->xptr;
                cur_len = static_cast<size_t>(this->xlength & INV_PAD);
                capacity = this->xcapacity;
            }
            else{
                ptr = this->xptr;
                cur_len = static_cast<size_t>(this->hex[15]);
                capacity = 15;
            }
            size_t length = cur_len + (new_len - old_len);
            if(length < capacity){
                size_t tail_len = cur_len - (end.ptr - ptr);
                memmove(begin.ptr + new_len, end.ptr, tail_len + 1);
                memcpy(begin.ptr, other.p, new_len);
                this->xlength = static_cast<uint32_t>(length) | PAD;
                end.ptr = begin.ptr + new_len;
            }
            else{
                size_t pre_len = begin.ptr - ptr;
                size_t tail_len = cur_len - (end.ptr - ptr);
                capacity = xstring::next_capacity(static_cast<uint32_t>(length));
                char* heap_ptr = new char[capacity];
                memcpy(heap_ptr, ptr, pre_len);
                memcpy(heap_ptr + pre_len, other.p, new_len);
                memcpy(heap_ptr + pre_len + new_len, end.ptr, tail_len + 1);
                if(this->big()) delete[] this->xptr;
                this->xptr = heap_ptr;
                this->xcapacity = capacity;
                this->xlength = length | PAD;
                begin.ptr = heap_ptr + pre_len;
                end.ptr = begin.ptr + new_len;
            }
        }
        else{
            memcpy(begin.ptr, other.p, new_len);
            if(old_len > new_len){
                size_t length, mv_len;
                if(this->big()){
                    length = static_cast<size_t>(this->xlength & INV_PAD);
                    mv_len = length - (end.ptr - this->xptr);
                    this->xlength = static_cast<uint32_t>(length - (old_len - new_len)) | PAD;
                }
                else{
                    length = static_cast<size_t>(hex[15]);
                    mv_len = length - (end.ptr - this->hex);
                    this->hex[15] = static_cast<char>(length - (old_len - new_len));
                }
                memmove(begin.ptr + new_len, end.ptr, mv_len + 1);
                end.ptr = begin.ptr + new_len;
            }
        }
        return range<false>(begin, end);
    }
    template<bool isconst>
    range<false> replace(iterator<isconst>&& begin, iterator<isconst>&& end, const range<true> other){
        return this->replace(begin, end, other);
    }
    template<bool isconst>
    range<false> replace(iterator<isconst>& begin, iterator<isconst>&& end, const range<true> other){
        return this->replace(begin, end, other);
    }
    template<bool isconst>
    range<false> replace(iterator<isconst>&& begin, iterator<isconst>& end, const range<true> other){
        return this->replace(begin, end, other);
    }


    template<bool C>
    operator range<C>() noexcept{
        uint32_t length;
        char* src = this->xstring_data(&length);
        return range<C>(src, length);
    }
    operator range<true>() const noexcept{
        uint32_t length;
        const char* src = this->xstring_data(&length);
        return range<true>(src, length);
    }
    xstring copy() const noexcept{
        return *this;
    }
    class decay_raw{
        friend union xstring;
        xstring& ths;
        decay_raw(xstring& ths, char* p) noexcept: ths(ths), p(p){}
    public:
        char* p;
        
        void update(uint32_t length) noexcept{
            if(ths.big()){ ths.xlength = length | PAD; }
            else{ ths.hex[15] = static_cast<char>(length); }
        }
    };
    decay_raw decay() noexcept{
        return this->big() ? decay_raw(*this, this->xptr) : decay_raw(*this, this->hex);
    }

    CONSTEXPR_FUNCTION size_t count(char ch) const noexcept{
        uint32_t length = 0;
        const char* ptr = this->xstring_data(&length);
        size_t counter = 0;
        for(size_t i = 0; i < length; i++) if(ptr[i] == ch) ++counter;
        return counter;
    }

    void toupper() noexcept{
        return this->big() ? 
            xstring::xtoupper(xptr, xptr + (xlength & INV_PAD)) :
            xstring::xtoupper(hex, hex + hex[15]);
    }
    void tolower() noexcept{
        return this->big() ? 
            xstring::xtolower(xptr, xptr + (xlength & INV_PAD)) :
            xstring::xtolower(hex, hex + hex[15]);
    }
    template<typename... Args>
    xstring join(Args&&... args) const noexcept{
        static_assert(sizeof...(Args) > 0, "xstring::join(): parameters cannot be empty!");
        uint32_t join_len;
        const char* join_src = this->xstring_data(&join_len);
        return xstring::xjoin(join_src, join_len, std::forward<Args>(args)...);
    }
    template<typename... CHS>
    Spliter<false, sizeof...(CHS) + 1> split(const char blank = ' ', CHS... chs) noexcept{
        constexpr size_t cmp_len = sizeof...(CHS) + 1;
        uint32_t length;
        char* ptr = this->xstring_data(&length);
        return xstring::Spliter<false, cmp_len>(ptr, ptr + length, blank, chs...);
    }
    template<typename... CHS>
    Spliter<true, sizeof...(CHS) + 1> split(const char blank = ' ', CHS... chs) const noexcept{
        constexpr size_t cmp_len = sizeof...(CHS) + 1;
        uint32_t length;
        const char* ptr = this->xstring_data(&length);
        return xstring::Spliter<true, cmp_len>(ptr, ptr + length, blank, chs...);
    }

    template<class T>
    bool starts_with(T&& other, size_t start = 0) const noexcept{
        return static_cast<range<true>>(*this).starts_with(static_cast<range<true>>(other), start);
    }
    template<class T>
    bool ends_with(T&& other, size_t end = 0) const noexcept{
        return static_cast<range<true>>(*this).ends_with(static_cast<range<true>>(other), end);
    }

    template<typename... CHS>
    void strip(const char blank = ' ', CHS... chs) noexcept{
        constexpr size_t cmp_len = sizeof...(CHS) + 1;
        const char cmp[] = {blank, static_cast<char>(chs)...};
        return xstrip<cmp_len == 1>(cmp, cmp_len);
    }
    template<typename... CHS>
    void lstrip(const char blank = ' ', CHS... chs) noexcept{
        constexpr size_t cmp_len = sizeof...(CHS) + 1;
        const char cmp[] = {blank, static_cast<char>(chs)...};
        return xlstrip<cmp_len == 1>(cmp, cmp_len);
    }
    template<typename... CHS>
    void rstrip(const char blank = ' ', CHS... chs) noexcept{
        constexpr size_t cmp_len = sizeof...(CHS) + 1;
        const char cmp[] = {blank, static_cast<char>(chs)...};
        return xrstrip<cmp_len == 1>(cmp, cmp_len);
    }


    int compare(const void* ptr, size_t len) const noexcept{
        return static_cast<range<true>>(*this).compare(ptr, len);
    }
    template<class T>
    int compare(T&& other) const noexcept{
        return static_cast<range<true>>(*this).compare(std::forward<T>(other));
    }
    template<class T>
    bool operator<(T&& other) const noexcept{
        return static_cast<range<true>>(*this).compare(std::forward<T>(other)) < 0;
    }
    template<class T>
    bool operator>(T&& other) const noexcept{
        return static_cast<range<true>>(*this).compare(std::forward<T>(other)) > 0;
    }
    template<class T>
    bool operator<=(T&& other) const noexcept{
        return static_cast<range<true>>(*this).compare(std::forward<T>(other)) <= 0;
    }
    template<class T>
    bool operator>=(T&& other) const noexcept{
        return static_cast<range<true>>(*this).compare(std::forward<T>(other)) >= 0;
    }


    bool equals(const void* cptr, uint32_t size) const noexcept{
        uint32_t cur_len;
        const char* src = this->xstring_data(&cur_len);
        return cur_len != size ? false : !memcmp(src, cptr, size);
    }
    bool operator==(const xstring& other) const noexcept{
        uint32_t cur_len, cmp_len;
        const char* src = this->xstring_data(&cur_len);
        const char* cmp = other.xstring_data(&cmp_len);
        return cur_len != cmp_len ? false : !memcmp(src, cmp, cmp_len);
    }
    template<bool C>
    bool operator==(range<C> other) const noexcept{
        uint32_t cur_len;
        const char* src = this->xstring_data(&cur_len);
        size_t cmp_len = other.len;
        return cur_len != cmp_len ? false : !memcmp(src, other.p, cmp_len);
    }
    template<uint32_t N, typename std::enable_if<(N == 1), bool>::type = true>
    bool operator==(const char(&cptr)[N]) const noexcept{
        return this->big() ? this->xlength == PAD : hex[15] == 0x00;
    }
    template<uint32_t N, typename std::enable_if<(N != 1), bool>::type = true>
    bool operator==(const char(&cptr)[N]) const noexcept{
        uint32_t cur_len;
        const char* src = this->xstring_data(&cur_len);
        constexpr uint32_t cmp_len = (N % 4 == 0 ? N : N - 1);
        constexpr uint32_t size = N - 1;
        return cur_len != size ? false : !memcmp(src, cptr, cmp_len);
    }
    template<class T>
    bool operator!=(T&& other) const noexcept{
        return !(this->operator==(std::forward<T>(other)));
    }


    xstring concat(const void* cptr, uint32_t size) const noexcept{
        uint32_t cur_len;
        const char* src = this->xstring_data(&cur_len);
        return xstring::xconcat(src, cur_len, cptr, size);
    }
    xstring operator+(const xstring& other) const noexcept{
        uint32_t size1, size2;
        const char* p1 = this->xstring_data(&size1);
        const char* p2 = other.xstring_data(&size2);
        return xstring::xconcat(p1, size1, p2, size2);
    }
    template<bool C>
    xstring operator+(range<C> other) const noexcept{
        uint32_t size2 = static_cast<uint32_t>(other.len);
        return this->big() ? 
            xstring::xconcat(xptr, xlength & INV_PAD, other.p, size2) :
            xstring::xconcat(hex, static_cast<uint32_t>(hex[15]), other.p, size2);
    }
    template<uint32_t N, typename std::enable_if<(N <= 15), bool>::type = true>
    xstring operator+(const char(&cptr)[N]) const noexcept{
        static_assert(N != 1, "xstring::operator+(): meaningless to add a null string");
        constexpr uint32_t size2 = N - 1;
        return this->big() ? 
            xstring::xconcat(xptr, xlength & INV_PAD, cptr, size2) :
            xstring::xconcat(hex, static_cast<uint32_t>(hex[15]), cptr, size2);
    }
    template<uint32_t N, typename std::enable_if<(N > 15), bool>::type = true>
    xstring operator+(const char(&cptr)[N]) const noexcept{
        constexpr uint32_t size = N - 1;
        xstring sx;
        uint32_t cur_len, new_len, capacity;
        const char* src;
        if(this->big()){
            cur_len = this->xlength & INV_PAD;
            new_len = cur_len + size;
            capacity = xstring::next_capacity(new_len);
            src = this->xptr;
        }
        else{
            cur_len = static_cast<uint32_t>(hex[15]);
            new_len = cur_len + size;
            constexpr uint32_t cc = xstring::next_capacity<size, 1>();
            capacity = cc;
            src = this->hex;
        }
        sx.set_xstring(capacity, new_len | PAD);
        memcpy(sx.xptr, src, cur_len);
        memcpy(sx.xptr + cur_len, cptr, N);
        return sx;
    }


    xstring& append(const void* cptr, uint32_t size) noexcept{
        if(this->big())
        {
            uint32_t cur_len = this->xlength & INV_PAD;
            uint32_t new_len = cur_len + size;
            if(new_len < this->xcapacity){
                memcpy(xptr + cur_len, cptr, size);
                xptr[new_len] = '\0';
                this->xlength = new_len | PAD;
            }
            else{
                uint32_t capacity = xstring::next_capacity(new_len);
                char* previous = xptr;
                this->set_xstring(capacity, new_len | PAD);
                memcpy(xptr, previous, cur_len);
                memcpy(xptr + cur_len, cptr, size);
                xptr[new_len] = '\0';
                delete[] previous;
            }
        }
        else
        {
            uint32_t cur_len = static_cast<uint32_t>(hex[15]);
            uint32_t new_len = cur_len + size;
            if(new_len < 15){
                memcpy(hex + cur_len, cptr, size);
                hex[new_len] = '\0';
                hex[15] = static_cast<char>(new_len);
            }
            else{
                uint32_t capacity = xstring::next_capacity(new_len);
                char* heap_ptr = new char[capacity];
                memcpy(heap_ptr, hex, cur_len);
                memcpy(heap_ptr + cur_len, cptr, size);
                heap_ptr[new_len] = '\0';
                xptr = heap_ptr;
                this->xcapacity = capacity;
                this->xlength = new_len | PAD;
            }
        }
        return *this;
    }
    xstring& operator+=(const xstring& other) noexcept{
        uint32_t length;
        const char* ptr = other.xstring_data(&length);
        return this->append(ptr, length);
    }
    template<bool C>
    xstring& operator+=(const range<C> other) noexcept{
        return this->append(other.p, static_cast<uint32_t>(other.len));
    }

    template<uint32_t N, typename std::enable_if<(N <= 15), bool>::type = true>
    xstring& operator+=(const char(&cptr)[N]) noexcept{
        static_assert(N != 1, "xstring::operator+=(): meaningless to add a null string");
        return this->append(cptr, N - 1);
    }
    template<uint32_t N, typename std::enable_if<(N > 15), bool>::type = true>
    xstring& operator+=(const char(&cptr)[N]) noexcept{
        constexpr uint32_t size = N - 1;
        if(this->big()) return this->append(cptr, size);
        constexpr uint32_t capacity = xstring::next_capacity<size, 1>();
        char* heap_ptr = new char[capacity];
        uint32_t length = static_cast<uint32_t>(hex[15]);
        memcpy(heap_ptr, hex, length);
        memcpy(heap_ptr + length, cptr, N);
        xptr = heap_ptr;
        this->xcapacity = capacity;
        this->xlength = (length + size) | PAD;
        return *this;
    }


    xstring() noexcept: hex(){}
    ~xstring() noexcept { if(hex[15] & 0x80) delete[] xptr; }
    xstring(const xstring& other) noexcept{
        if(other.big()){
            uint32_t capacity = other.xcapacity;
            uint32_t length = other.xlength;
            this->set_xstring(capacity, length);
            memcpy(this->xptr, other.xptr, static_cast<size_t>(length & INV_PAD) + 1);
        }
        else memcpy(this->hex, other.hex, sizeof(xstring));
    }
    xstring& operator=(const xstring& other) noexcept{
        if(this != &other){
            this->~xstring();
            if(other.big()){
                uint32_t capacity = other.xcapacity;
                uint32_t length = other.xlength;
                this->set_xstring(capacity, length);
                memcpy(this->xptr, other.xptr, static_cast<size_t>(length & INV_PAD) + 1);
            }
            else memcpy(this->hex, other.hex, sizeof(xstring));
        }
        return *this;
    }
    xstring(xstring&& other) noexcept{
        memcpy(hex, other.hex, sizeof(xstring));
        other.hex[15] = 0x00;
    }
    xstring& operator=(xstring&& other) noexcept{
        this->~xstring();
        memcpy(hex, other.hex, sizeof(xstring));
        other.hex[15] = 0x00;
        return *this;
    }
    explicit xstring(uint32_t N) noexcept{
        if(N >= 15){
            uint32_t capacity = xstring::next_capacity(N);
            this->set_xstring(capacity, PAD);
        }
        else memset(hex, 0x00, 16);
    }
    xstring(const void* cptr, uint32_t size) noexcept{
        if(size >= 15){
            uint32_t capacity = xstring::next_capacity(size);
            this->set_xstring(capacity, size | PAD);
            memcpy(xptr, cptr, size);
            xptr[size] = 0x00;
        }
        else{
            memcpy(hex, cptr, size);
            hex[size] = '\0';
            hex[15] = static_cast<char>(size);
        }
    }
    template<uint32_t N, typename std::enable_if<(N == 1), bool>::type = true>
    xstring(const char(&cptr)[N]) noexcept: hex(){}
    template<uint32_t N, typename std::enable_if<(N <= 15 && N != 1), bool>::type = true>
    xstring(const char(&cptr)[N]) noexcept{
        memcpy(hex, cptr, N);
        hex[14] = '\0';
        hex[15] = static_cast<char>(N - 1);
    }
    template<uint32_t N, typename std::enable_if<(N > 15), bool>::type = true>
    xstring(const char(&cptr)[N]) noexcept{
        constexpr uint32_t capacity = xstring::next_capacity<N - 1, 1>();
        constexpr uint32_t length = (N - 1) | PAD;
        this->set_xstring(capacity, length);
        memcpy(xptr, cptr, N);
    }
    template<uint32_t N, typename std::enable_if<(N == 1), bool>::type = true>
    xstring& operator=(const char(&cptr)[N]){
        this->~xstring();
        memset(hex, 0x00, sizeof(xstring));
        return *this;
    }
    template<uint32_t N, typename std::enable_if<(N <= 15 && N != 1), bool>::type = true>
    xstring& operator=(const char(&cptr)[N]) noexcept{
        this->~xstring();
        memcpy(hex, cptr, N);
        hex[14] = '\0';
        hex[15] = static_cast<char>(N - 1);
        return *this;
    }
    template<uint32_t N, typename std::enable_if<(N > 15), bool>::type = true>
    xstring& operator=(const char(&cptr)[N]) noexcept{
        this->~xstring();
        constexpr uint32_t capacity = xstring::next_capacity<N - 1, 1>();
        constexpr uint32_t length = (N - 1) | PAD;
        this->set_xstring(capacity, length);
        memcpy(xptr, cptr, N);
        return *this;
    }

private:
    char* xstring_data(uint32_t* length) noexcept{
        if(this->big()){
            *length = this->xlength & INV_PAD;
            return this->xptr;
        }
        *length = static_cast<uint32_t>(hex[15]);
        return this->hex;
    }
    CONSTEXPR_FUNCTION const char* xstring_data(uint32_t* length) const noexcept{
        if(this->hex[15] & 0x80){
            *length = this->xlength & INV_PAD;
            return this->xptr;
        }
        *length = static_cast<uint32_t>(hex[15]);
        return this->hex;
    }

    void set_xstring(uint32_t capacity, uint32_t length) noexcept{
        this->xptr = new char[capacity];
        this->xcapacity = capacity;
        this->xlength = length;
    }

#ifdef XSTRING_CPP11
    template<uint32_t new_len, uint32_t shift, 
    typename std::enable_if<new_len == 15, bool>::type = true>
    static constexpr uint32_t next_capacity() noexcept{
        return 32;
    }
    template<uint32_t new_len, uint32_t shift,
    typename std::enable_if<(!(new_len & (new_len - 1)) && shift == 1), bool>::type = true>
    static constexpr uint32_t next_capacity() noexcept{ 
        static_assert(new_len > 15, "xstring::next_capacity(): Unknown Error");
        return new_len + new_len;
    }
    template<uint32_t new_len, uint32_t shift, 
    typename std::enable_if<((new_len & (new_len - 1)) && new_len != 15 && shift == 1), bool>::type = true>
    static constexpr uint32_t next_capacity() noexcept{
        static_assert(new_len > 15, "xstring::next_capacity(): Unknown Error");
        return next_capacity<(new_len | (new_len >> shift)), shift * 2>();
    }
    template<uint32_t new_len, uint32_t shift, 
    typename std::enable_if<(shift != 16 && shift > 1), bool>::type = true>
    static constexpr uint32_t next_capacity() noexcept{
        static_assert(new_len > 15, "xstring::next_capacity(): Unknown Error");
        return next_capacity<(new_len | (new_len >> shift)), shift * 2>();
    }
    template<uint32_t new_len, uint32_t shift, 
    typename std::enable_if<shift == 16, bool>::type = true>
    static constexpr uint32_t next_capacity() noexcept{
        return (new_len | (new_len >> shift)) + 1;
    }
#else
    template<uint32_t new_len, uint32_t shift>
    static constexpr uint32_t next_capacity() noexcept{
        return next_capacity(new_len);
    }
#endif
    static CONSTEXPR_FUNCTION uint32_t next_capacity(uint32_t new_len) noexcept{
        if(new_len == 15) return 32;
        if(new_len & (new_len - 1)){
            new_len |= new_len >> 1;
            new_len |= new_len >> 2;
            new_len |= new_len >> 4;
            new_len |= new_len >> 8;
            new_len |= new_len >> 16;
            return new_len + 1;
        }
        return new_len + new_len;
    }

    static void xtoupper(char* begin, char* end) noexcept{
        while(begin != end){
            char ch = *begin;
            *(begin++) = (ch >= 'a' && ch <= 'z') ? ch - 32 : ch;
        }
    }
    static void xtolower(char* begin, char* end) noexcept{
        while(begin != end){
            char ch = *begin;
            *(begin++) = (ch >= 'A' && ch <= 'Z') ? ch + 32 : ch;
        }
    }

    static xstring xconcat(const void* p1, uint32_t size1, const void* p2, uint32_t size2) noexcept{
        xstring sx;
        uint32_t new_len = size1 + size2;
        if(new_len >= 15){
            uint32_t capacity = xstring::next_capacity(new_len);
            sx.set_xstring(capacity, new_len | PAD);
            memcpy(sx.xptr, p1, size1);
            memcpy(sx.xptr + size1, p2, size2);
            sx.xptr[new_len] = '\0';
        }
        else{
            memcpy(sx.hex, p1, size1);
            memcpy(sx.hex + size1, p2, size2);
            sx.hex[new_len] = '\0';
            sx.hex[15] = static_cast<char>(new_len);
        }
        return sx;
    }

    template<class T>
    struct is_literal{ static constexpr bool value = false; };
    template<size_t N>
    struct is_literal<const char(&)[N]>{ static constexpr bool value = false; };

    template<typename... Args, typename std::enable_if<sizeof...(Args) == 0, bool>::type = true>
    static constexpr uint32_t xjoin_helperfunc_constsize() noexcept{
        return 0;
    }
    template<class T, typename... Args>
    static constexpr uint32_t xjoin_helperfunc_constsize() noexcept{
        using raw_type = typename std::decay<T>::type;
        return (is_literal<raw_type>::value ? 0 : sizeof(T) - 1) + xjoin_helperfunc_constsize<Args...>();
    }
    template<typename... Args, typename std::enable_if<sizeof...(Args) == 0, bool>::type = true>
    static uint32_t xjoin_helperfunc_size(Args&&... args) noexcept{
        return 0;
    }
    template<size_t N, typename... Args>
    static uint32_t xjoin_helperfunc_size(const char(&cptr)[N], Args&&... args) noexcept{
        return xjoin_helperfunc_size(std::forward<Args>(args)...);
    }
    template<class T, typename... Args>
    static uint32_t xjoin_helperfunc_size(T&& t, Args&&... args) noexcept{
        return t.template size<uint32_t>() + xjoin_helperfunc_size(std::forward<Args>(args)...);
    }
    template<typename... Args, typename std::enable_if<sizeof...(Args) == 0, bool>::type = true>
    static void xjoin_helperfunc_cpy(char* dst, const void* join_src,
        uint32_t join_len, range<true> range, Args&&... args) noexcept{
        memcpy(dst, range.p, range.len);
        *(dst + range.len) = '\0';
    }
    template<typename... Args, typename std::enable_if<sizeof...(Args) != 0, bool>::type = true>
    static void xjoin_helperfunc_cpy(char* dst, const void* join_src, 
        uint32_t join_len, range<true> range, Args&&... args) noexcept{
        memcpy(dst, range.p, range.len);
        memcpy(dst + range.len, join_src, join_len);
        return xjoin_helperfunc_cpy(dst + range.len + join_len, join_src, join_len, std::forward<Args>(args)...);
    }
    template<typename... Args>
    static xstring xjoin(const void* join_src, uint32_t join_len, Args&&... args) noexcept{
        constexpr uint32_t const_size = xjoin_helperfunc_constsize<Args...>();
        uint32_t new_len = (sizeof...(Args) - 1) * join_len + const_size;
        new_len += xjoin_helperfunc_size(std::forward<Args>(args)...);
        xstring sx(new_len);
        char* dst;
        if(sx.big()){
            dst = sx.xptr;
            sx.xlength = new_len | PAD;
        }
        else{
            dst = sx.hex;
            sx.hex[15] = static_cast<char>(new_len);
        }
        xjoin_helperfunc_cpy(dst, join_src, join_len, std::forward<Args>(args)...);
        return sx;
    }

    template<bool issingle, typename std::enable_if<issingle, bool>::type = true>
    static constexpr bool charcmp(char ch, const char* cmp, size_t) noexcept{
        return ch == cmp[0];
    }
    template<bool issingle, typename std::enable_if<!issingle, bool>::type = true>
    static constexpr bool charcmp(char ch, const char* cmp, size_t cmp_len) noexcept{
        return __builtin_memchr(cmp, ch, cmp_len) != nullptr;
    }

    template<bool issingle>
    void xstrip(const char* cmp, size_t cmp_len) noexcept{
        char *src, *head, *tail;
        bool ISBIG = this->big();
        if(ISBIG){
            head = src = xptr;
            tail = xptr + (this->xlength & INV_PAD) - 1;
        }
        else{
            head = src = hex;
            tail = hex + hex[15] - 1;
        }
        while(true){
            const char ch = *head;
            if(ch == '\0'){
                src[0] = '\0';
                if(ISBIG) this->xlength = PAD;
                else hex[15] = 0x00;
                return;
            }
            if(xstring::charcmp<issingle>(ch, cmp, cmp_len)) ++head; else break;
        }
        while(true){
            if(xstring::charcmp<issingle>(*tail, cmp, cmp_len)) --tail; else break;
        }
        size_t new_len = tail - head + 1;
        if(head != src) memmove(src, head, new_len);
        src[new_len] = '\0';
        if(ISBIG) this->xlength = static_cast<uint32_t>(new_len) | PAD;
        else hex[15] = static_cast<char>(new_len);
    }

    template<bool issingle>
    void xlstrip(const char* cmp, size_t cmp_len) noexcept{
        char *src, *head, *tail;
        bool ISBIG = this->big();
        if(ISBIG){
            head = src = xptr;
            tail = xptr + (this->xlength & INV_PAD) - 1;
        }
        else{
            head = src = hex;
            tail = hex + hex[15] - 1;
        }
        while(true){
            const char ch = *head;
            if(ch == '\0'){
                src[0] = '\0';
                if(ISBIG) this->xlength = PAD;
                else hex[15] = 0x00;
                return;
            }
            if(xstring::charcmp<issingle>(ch, cmp, cmp_len)) ++head; else break;
        }
        if(head != src){
            size_t new_len = tail - head + 1;
            memmove(src, head, new_len);
            src[new_len] = '\0';
            if(ISBIG) this->xlength = static_cast<uint32_t>(new_len) | PAD;
            else hex[15] = static_cast<char>(new_len);
        }
    }

    template<bool issingle>
    void xrstrip(const char* cmp, size_t cmp_len) noexcept{
        char* head, *tail;
        bool ISBIG = this->big();
        if(ISBIG){
            head = xptr;
            tail = xptr + (this->xlength & INV_PAD) - 1;
        }
        else{
            head = hex;
            tail = hex + hex[15] - 1;
        }
        if(!xstring::charcmp<issingle>(*tail, cmp, cmp_len)) return;
        --tail;
        while(true){
            if(xstring::charcmp<issingle>(*tail, cmp, cmp_len)){
                --tail;
                if(tail == head) break;
            }
            else break;
        }
        *(++tail) = '\0';
        if(ISBIG) this->xlength = static_cast<uint32_t>(tail - head) | PAD;
        else hex[15] = static_cast<char>(tail - head);
    }

    char* xinsert(iterator<false>& begin, const void* src, size_t size) noexcept{
        if(this->big()){
            size_t cur_len = static_cast<size_t>(this->xlength & INV_PAD);
            size_t pre_len = begin.ptr - this->xptr;
            uint32_t new_len = static_cast<uint32_t>(cur_len + size);
            if(new_len < this->xcapacity){
                memmove(begin.ptr + size, begin.ptr, cur_len - pre_len + 1);
                memcpy(begin.ptr, src, size);
                this->xlength = new_len | PAD;
            }
            else{
                uint32_t capacity = xstring::next_capacity(new_len);
                char* heap_ptr = new char[capacity];
                memcpy(heap_ptr, this->xptr, pre_len);
                memcpy(heap_ptr + pre_len, src, size);
                memcpy(heap_ptr + pre_len + size, this->xptr + pre_len, cur_len - pre_len + 1);
                delete[] this->xptr;
                this->xptr = heap_ptr;
                this->xcapacity = capacity;
                this->xlength = new_len | PAD;
                begin.ptr = heap_ptr + pre_len;
            }
            return this->xptr + pre_len + size;
        }
        size_t cur_len = static_cast<size_t>(hex[15]);
        size_t pre_len = begin.ptr - this->hex;
        uint32_t new_len = static_cast<uint32_t>(cur_len + size);
        if(new_len < 15){
            memmove(begin.ptr + size, begin.ptr, cur_len - pre_len + 1);
            memcpy(begin.ptr, src, size);
            this->hex[15] = static_cast<char>(new_len);
            return this->hex + pre_len + size;
        }
        uint32_t capacity = xstring::next_capacity(new_len);
        char* heap_ptr = new char[capacity];
        memcpy(heap_ptr, this->hex, pre_len);
        memcpy(heap_ptr + pre_len, src, size);
        memcpy(heap_ptr + pre_len + size, this->hex + pre_len, cur_len - pre_len + 1);
        this->xptr = heap_ptr;
        this->xcapacity = capacity;
        this->xlength = new_len | PAD;
        begin.ptr = heap_ptr + pre_len;
        return this->xptr + pre_len + size;
    }

    template<bool isconst, size_t cmp_len>
    class Spliter{
        friend union xstring;
        using data_type = typename const_traits<isconst, char>::data_type;
        using const_data_type = typename std::add_const<data_type>::type;
        data_type* sbegin;
        data_type* send;
        char delimeters[cmp_len];
        template<class T>
        struct always_true{ static constexpr bool value = true; };

        template<typename... CHS>
        constexpr Spliter(data_type* begin, data_type* end, CHS... chs)
            noexcept: sbegin(begin), send(end), delimeters{static_cast<char>(chs)...}
        {}


        template<class T, typename std::enable_if<(cmp_len == 1) && always_true<T>::value, bool>::type = true>
        constexpr bool findch(T ch) const noexcept{
            return ch != delimeters[0];
        }
        template<class T, typename std::enable_if<(cmp_len > 1) && always_true<T>::value, bool>::type = true>
        constexpr bool findch(T ch) const noexcept{
            return __builtin_memchr(delimeters, ch, cmp_len) == nullptr;
        }

        template<bool isconst2>
        class iterator{
            friend class Spliter;
            using data_type = typename const_traits<isconst2, char>::data_type;
            using const_data_type = typename std::add_const<data_type>::type;

            const Spliter* spliter;
            data_type* last;
            data_type* begin;
            data_type* end;

            static CONSTEXPR_FUNCTION iterator getbegin(const Spliter* spliter) noexcept{
                iterator ret(spliter, nullptr, spliter->sbegin, spliter->sbegin);
                while(ret.end < spliter->send && spliter->findch(*ret.end)){
                    ++ret.end;
                }
                return ret;
            }
            static CONSTEXPR_FUNCTION iterator getend(const Spliter* spliter) noexcept{
                iterator ret(spliter, spliter->send, spliter->send + 1, nullptr);
                while(ret.last >= spliter->sbegin && spliter->findch(*ret.last)){
                    --ret.last;
                }
                ++ret.last;
                return ret;
            }
            CONSTEXPR_FUNCTION void move_next() noexcept{
                if(end != nullptr){
                    last = begin;
                    begin = ++end;
                    while(end < spliter->send && spliter->findch(*end)){
                        ++end;
                    }
                    if(end > spliter->send){
                        begin = spliter->send + 1;
                        end = nullptr;
                    }
                }
            }
            CONSTEXPR_FUNCTION void move_back() noexcept{
                if(last != nullptr){
                    end = begin - 1;
                    begin = last;
                    last -= 2;
                    while(last >= spliter->sbegin && spliter->findch(*last)){
                        --last;
                    }
                    last = last < spliter->sbegin ? nullptr : last;
                }
            }
            constexpr iterator(const Spliter* s, data_type* l, data_type* b, data_type* e)
                noexcept: spliter(s), last(l), begin(b), end(e)
            {}
        public:
            constexpr iterator() = default;
            constexpr iterator(const iterator& other) 
                noexcept: spliter(other.spliter), last(other.last), begin(other.begin), end(other.end)
            {}
            CONSTEXPR_FUNCTION iterator& operator++() noexcept{ this->move_next(); return *this; }
            CONSTEXPR_FUNCTION iterator operator++(int) noexcept{ iterator cpy(*this); this->move_next(); return cpy; }
            CONSTEXPR_FUNCTION iterator& operator--() noexcept{ this->move_back(); return *this; }
            CONSTEXPR_FUNCTION iterator operator--(int) noexcept{ iterator cpy(*this); this->move_back(); return cpy; }
            CONSTEXPR_FUNCTION iterator operator+(size_t i) const noexcept{
                iterator cpy(*this);
                for(int j = 0; j < i; j++) cpy.move_next();
                return cpy;
            }
            CONSTEXPR_FUNCTION iterator operator-(size_t i) const noexcept{
                iterator cpy(*this);
                for(int j = 0; j < i; j++) cpy.move_back();
                return cpy;
            }
            CONSTEXPR_FUNCTION range<isconst2> operator*() noexcept{
                return range<isconst2>(this->begin, this->end);
            }
            constexpr range<true> operator*() const noexcept{
                return range<true>(this->begin, this->end);
            }
            template<bool R>
            constexpr bool operator==(const iterator<R>& other) const noexcept{
                // return !__builtin_memcmp(this, &other, sizeof(iterator));
                return (this->begin == other.begin) && (this->end == other.end);
            }
            template<bool R>
            constexpr bool operator!=(const iterator<R>& other) const noexcept{
                // return __builtin_memcmp(this, &other, sizeof(iterator));
                return (this->begin != other.begin) || (this->end != other.end);
            }
        };
    public:
        Spliter::iterator<isconst> begin() noexcept{ 
            return Spliter::iterator<isconst>::getbegin(this); 
        }
        CONSTEXPR_FUNCTION Spliter::iterator<true> begin() const noexcept{ 
            return Spliter::iterator<true>::getbegin(this); 
        }
        CONSTEXPR_FUNCTION Spliter::iterator<true> cbegin() const noexcept{ 
            return Spliter::iterator<true>::getbegin(this); 
        }
        Spliter::iterator<isconst> end() noexcept{
            return Spliter::iterator<isconst>::getend(this);
        }
        CONSTEXPR_FUNCTION Spliter::iterator<true> end() const noexcept{
            return Spliter::iterator<true>::getend(this);
        }
        CONSTEXPR_FUNCTION Spliter::iterator<true> cend() const noexcept{
            return Spliter::iterator<true>::getend(this);
        }
    };

    static CONSTEXPR_FUNCTION size_t critical_factorization(const char* needle, size_t needle_len, size_t* period){
        size_t max_suffix = SIZE_MAX;
        size_t j = 0, k = 1, p = 1;
        while(j + k < needle_len)
        {
            char a = needle[j + k];
            char b = needle[max_suffix + k];
            if(a < b){
                j += k;
                k = 1;
                p = j - max_suffix;
            }
            else if(a == b){
                if(k != p) ++k;
                else{
                    j += p;
                    k = 1;
                }
            }
            else{
                max_suffix = j++;
                k = p = 1;
            }
        }
        *period = p;
        size_t max_suffix_rev = SIZE_MAX;
        j = 0;
        k = p = 1;
        while(j + k < needle_len){
            char a = needle[j + k];
            char b = needle[max_suffix_rev + k];
            if(b < a){
                j += k;
                k = 1;
                p = j - max_suffix;
            }
            else if(a == b){
                if(k != p) ++k;
                else{
                    j += p;
                    k = 1;
                }
            }
            else{
                max_suffix = j++;
                k = p = 1;
            }
        }
        if(max_suffix_rev + 1 < max_suffix + 1) return max_suffix + 1;
        *period = p;
        return max_suffix_rev + 1;
    }

    static uint8_t hash2(const char* p) noexcept{
        return (static_cast<size_t>(p[0]) - (static_cast<size_t>(p[-1]) << 3)) % 256;
    }

public:
    // non-constexpr version
    template<bool isconst = true>
    static typename const_traits<isconst, void>::pointer_type memmem(
        typename const_traits<isconst, void>::pointer_type haystack, size_t hs_len,
        const void* needle, size_t ne_len
    ) noexcept{
        using pointer_type = typename const_traits<isconst, char>::pointer_type;
        return memmem(static_cast<pointer_type>(haystack), hs_len, static_cast<const char*>(needle), ne_len);
    }

    template<bool isconst = true>
    static CONSTEXPR_FUNCTION typename const_traits<isconst, char>::pointer_type memmem(
        typename const_traits<isconst, char>::pointer_type haystack, size_t hs_len,
        const char* needle, size_t ne_len
    ) noexcept{
        auto hs = haystack;
        const char* ne = needle;
        const char* end = hs + hs_len - ne_len;
        if(ne_len == 0){ return haystack; }
        if(hs_len < ne_len){ return nullptr; }
        if(ne_len == 1){ return static_cast<decltype(hs)>(__builtin_memchr(hs, ne[0], hs_len)); }
        else if(ne_len == 2){
            uint32_t nw = ne[0] << 16 | ne[1], hw = hs[0] << 16 | hs[1];
            for (hs++; hs <= end && hw != nw; ){
                hw = hw << 16 | *++hs;
            }
            return hw == nw ? (hs - 1) : nullptr;
        }
        if(ne_len < 256)
        {
            uint8_t m1 = static_cast<uint8_t>(ne_len - 1);
            uint8_t shift[256] = {0};
            for(uint8_t i = 1; i < m1; i++){
                shift[hash2(ne + i)] = i;
            }
            uint8_t tmp = hash2(ne + m1);
            uint8_t shift1 = m1 - shift[tmp];
            shift[tmp] = m1;

            uint8_t offset = 0;
            while(hs <= end)
            {
                do{
                    hs += m1;
                    tmp = shift[hash2(hs)];
                }while(tmp == 0 && hs <= end);

                hs -= tmp;
                if(tmp < m1) continue;

                if(m1 < 15 || __builtin_memcmp(hs + offset, ne + offset, 8) == 0){
                    if(__builtin_memcmp(hs, ne, m1) == 0) return (hs);
                    offset = (offset >= 8 ? offset : m1) - 8;
                }
                hs += shift1;
            }
        }
        else // two way
        {
            constexpr size_t mm = 1U << 8U;
            size_t period = 0;
            size_t suffix = critical_factorization(ne, ne_len, &period);
            size_t shift_table[mm] = {};
            for(uint32_t i = 0; i < ne_len; i++){
                shift_table[static_cast<uint8_t>(ne[i])] = ne_len - i - 1;
            }

            if(__builtin_memcmp(ne, ne + period, suffix) == 0){
                size_t memory = 0;
                size_t j = 0;
                while(j <= (hs_len - ne_len))
                {
                    size_t shift = shift_table[static_cast<uint8_t>(hs[j + ne_len - 1])];
                    if(shift > 0)
                    {
                        shift = (memory && shift < period) ? ne_len - period : shift;
                        memory = 0;
                        j += shift;
                        continue;
                    }
                    size_t i = suffix > memory ? suffix : memory;
                    const char* pneedle = &ne[i];
                    const char* phaystack = &hs[i + j];
                    while(i < ne_len - 1 && *pneedle++ == *phaystack++) ++i;
                    if(ne_len - 1 <= i)
                    {
                        i = suffix - 1;
                        pneedle = &ne[i];
                        phaystack = &hs[i + j];
                        while(memory < i + 1 && *pneedle-- == *phaystack--) --i;
                        if(i + 1 < memory + 1) return (hs + j);
                        j += period;
                        memory = ne_len - period;
                    }
                    else{
                        j += i - suffix + 1;
                        memory = 0;
                    }
                }
            }
            else{
                period = ne_len - suffix;
                period = (suffix > period ? suffix : period) + 1;
                size_t j = 0;
                while(j <= (hs_len - ne_len))
                {
                    size_t shift = shift_table[static_cast<uint8_t>(hs[j + ne_len - 1])];
                    if(shift > 0){
                        j += shift;
                        continue;
                    }
                    size_t i = suffix;
                    const char* pneedle = &ne[i];
                    const char* phaystack = &hs[i + j];
                    while(i < ne_len - 1 && *pneedle++ == *phaystack++) ++i;
                    if(ne_len - 1 <= i){
                        i = suffix - 1;
                        pneedle = &ne[i];
                        phaystack = &hs[i + j];
                        while(i != SIZE_MAX && *pneedle-- == *phaystack--) --i;
                        if(i == SIZE_MAX) return (hs + j);
                        j += period;
                    }
                    else j += i - suffix + 1;
                }
            }
        }
        return nullptr;
    }

    template<class T, typename std::enable_if<std::is_unsigned<T>::value, bool>::type = true>
    static uint32_t to_chars(char* buffer, T value) noexcept{
        char buf[20];
        char* end = &buf[19];
        char* dst = &buf[19];
        do{
            *(--dst) = (value % 10) + '0';
            value /= 10;
        }while(value != 0);
        uint32_t size = static_cast<uint32_t>(end - dst);
        memcpy(buffer, dst, size);
        return size;
    }
    template<class T, typename std::enable_if<std::is_integral<T>::value && !std::is_unsigned<T>::value, bool>::type = true>
    static uint32_t to_chars(char* buffer, T value) noexcept{
        char buf[21];
        char* end = &buf[20];
        char* dst = &buf[20];
        bool flag = false;
        if(value < 0){
            flag = true;
            value = -value;
        } 
        do{
            *(--dst) = (value % 10) + '0';
            value /= 10;
        }while(value != 0);
        if(flag){ *(--dst) = '-'; }
        uint32_t size = static_cast<uint32_t>(end - dst);
        memcpy(buffer, dst, size);
        return size;
    }

    template<class T, typename std::enable_if<std::is_unsigned<T>::value, bool>::type = true>
    static xstring to_string(T integral) noexcept{
        char buf[20];
        char* end = &buf[19];
        char* dst = &buf[19];
        do{
            *(--dst) = (integral % 10) + '0';
            integral /= 10;
        }while(integral != 0);
        return xstring(dst, static_cast<uint32_t>(end - dst));
    }
    template<class T, typename std::enable_if<std::is_integral<T>::value && !std::is_unsigned<T>::value, bool>::type = true>
    static xstring to_string(T integral) noexcept{
        char buf[21];
        char* end = &buf[20];
        char* dst = &buf[20];
        bool flag = false;
        if(integral < 0){
            flag = true;
            integral = -integral;
        } 
        do{
            *(--dst) = (integral % 10) + '0';
            integral /= 10;
        }while(integral != 0);
        if(flag){ *(--dst) = '-'; }
        return xstring(dst, static_cast<uint32_t>(end - dst));
    }

    static INLINE_VARIABLE constexpr uint64_t bit_table[128] = {
        1, 1, 3, 1, 7, 
        1, 9, 1, 31, 2, 
        63, 2, 99, 2, 255, 
        3, 511, 3, 999, 3, 
        2047, 4, 4095, 4, 8191, 
        4, 9999, 4, 32767, 5, 
        65535, 5, 99999, 5, 262143, 
        6, 524287, 6, 999999, 6, 
        2097151, 7, 4194303, 7, 8388607, 
        7, 9999999, 7, 33554431, 8, 
        67108863, 8, 99999999, 8, 268435455, 
        9, 536870911, 9, 999999999, 9, 
        2147483647, 10, 4294967295, 10, 8589934591, 
        10, 9999999999, 10, 34359738367, 11, 
        68719476735, 11, 99999999999, 11, 274877906943, 
        12, 549755813887, 12, 999999999999, 12, 
        2199023255551, 13, 4398046511103, 13, 8796093022207, 
        13, 9999999999999ULL, 13, 35184372088831, 14, 
        70368744177663, 14, 99999999999999ULL, 14, 281474976710655ULL, 
        15, 562949953421311, 15, 999999999999999ULL, 15, 
        2251799813685247, 16, 4503599627370495, 16, 9007199254740991ULL, 
        16, 9999999999999999ULL, 16, 36028797018963967ULL, 17, 
        72057594037927935, 17, 99999999999999999ULL, 17, 288230376151711743ULL, 
        18, 576460752303423487, 18, 999999999999999999ULL, 18, 
        2305843009213693951, 19, 4611686018427387903, 19, 9223372036854775807ULL, 
        19, 9999999999999999999ULL, 19
    };
    template<class T, typename std::enable_if<std::is_unsigned<T>::value, bool>::type = true>
    static constexpr uint32_t count_digits(T integral) noexcept{
    #ifndef XSTRING_CPP11
        if(integral == 0){ return 1; }
        auto cur_idx = 2 * (63 - __builtin_clzll(integral));
        return static_cast<uint32_t>(bit_table[cur_idx + 1] + (integral > bit_table[cur_idx]));
    #else
        return integral == 0 ? 1 :
        static_cast<uint32_t>(
            bit_table[2 * (63 - __builtin_clzll(integral)) + 1]
            + (integral > bit_table[2 * (63 - __builtin_clzll(integral))])
        );
    #endif
    }

    template<class T, typename std::enable_if<std::is_integral<T>::value && !std::is_unsigned<T>::value, bool>::type = true>
    static constexpr uint32_t count_digits(T integral) noexcept{
        using unsigned_type = typename std::make_unsigned<T>::type;
        return integral < 0 ? 
            1 + count_digits<unsigned_type>(static_cast<unsigned_type>(-integral)) : 
            count_digits<unsigned_type>(integral);
    }

    static int64_t to_integral(range<true> range, bool& success) noexcept{
        bool neg = false;
        int64_t integer = 0;
        const char* ptr = range.data();
        const char* end = ptr + range.size();
        char ch;
        while(*ptr == ' '){
            ++ptr;
            if(ptr >= end){ goto FAILURE;}
        }
        ch = *ptr;
        if(ch == '-'){
            neg = true;
            ch = *(++ptr);
        }
        while(ch == '0'){
            ++ptr;
            if(ptr >= end){ break; }
            ch = *ptr;
        }
        if(ch >= '1' && ch <= '9'){
            integer = ch - '0';
            ++ptr;
            if(ptr < end){
                ch = *ptr;
                while(ch >= '0' && ch <= '9'){
                    integer = 10 * integer + (ch - '0');
                    ++ptr;
                    if(ptr >= end){ break; }
                    ch = *ptr;
                }   
            }
        }
        while(ptr < end){
            if(*ptr != ' '){ goto FAILURE; }
            ++ptr;
        }
        success = true;
        return neg ? -integer : integer; 
    FAILURE:
        success = false;
        return 0;
    }
private:
    // tricky one with double & string
    struct grisu2{
        struct diyfp{
            uint64_t f;
            int e;
            constexpr diyfp() noexcept: f(0), e(0){}
            constexpr diyfp(uint64_t f, int e) noexcept: f(f), e(e){}

            static diyfp sub(const diyfp& x, const diyfp& y) noexcept{
                return {x.f - y.f, x.e};
            }

            static diyfp mul(const diyfp& x, const diyfp& y) noexcept{
            #if defined(_MSC_VER) && defined(_M_AMD64)
                uint64_t h;
                uint64_t l = _umul128(x.f, y.f, &h);
                if (l & (uint64_t(1) << 63u)) { ++h; }
                return {h, x.e + y.e + 64};
            #elif (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)) && defined(__x86_64__)
                __extension__ typedef unsigned __int128 uint128;
                uint128 p = static_cast<uint128>(x.f) * static_cast<uint128>(y.f);
                auto h = static_cast<uint64_t>(p >> 64u);
                auto l = static_cast<uint64_t>(p);
                if (l & (uint64_t(1) << 63u)) { ++h; }
                return {h, x.e + y.e + 64};
            #else
                constexpr uint64_t mask32 = 0xffffffffu;
                uint64_t u_lo = x.f & mask32;
                uint64_t u_hi = x.f >> 32u;
                uint64_t v_lo = y.f & mask32;
                uint64_t v_hi = y.f >> 32u;

                uint64_t p0 = u_lo * v_lo;
                uint64_t p1 = u_lo * v_hi;
                uint64_t p2 = u_hi * v_lo;
                uint64_t p3 = u_hi * v_hi;

                uint64_t p0_hi = p0 >> 32u;
                uint64_t p1_lo = p1 & mask32;
                uint64_t p1_hi = p1 >> 32u;
                uint64_t p2_lo = p2 & mask32;
                uint64_t p2_hi = p2 >> 32u;

                constexpr uint64_t round = static_cast<uint64_t>(1) << (64u - 32u - 1u);
                uint64_t Q = p0_hi + p1_lo + p2_lo + round;
                uint64_t h = p3 + p2_hi + p1_hi + (Q >> 32u);
                return {h, x.e + y.e + 64};
            #endif
            }

            diyfp normalize() const noexcept{
                int s = static_cast<int>(__builtin_clzll(f));
                return {f << s, e - s};
            }

            diyfp normalize(int target) const noexcept{
                return {f << (e - target), target};
            }
        };

        struct cached_power{
            uint64_t f;
            int e;
            int k;
        };
        static cached_power get_cached(int e) noexcept{
            static constexpr cached_power cached[] = {
                {0xAB70FE17C79AC6CA, -1060, -300}, {0xFF77B1FCBEBCDC4F, -1034, -292},
                {0xBE5691EF416BD60C, -1007, -284}, {0x8DD01FAD907FFC3C, -980, -276},
                {0xD3515C2831559A83, -954, -268},  {0x9D71AC8FADA6C9B5, -927, -260},
                {0xEA9C227723EE8BCB, -901, -252},  {0xAECC49914078536D, -874, -244},
                {0x823C12795DB6CE57, -847, -236},  {0xC21094364DFB5637, -821, -228},
                {0x9096EA6F3848984F, -794, -220},  {0xD77485CB25823AC7, -768, -212},
                {0xA086CFCD97BF97F4, -741, -204},  {0xEF340A98172AACE5, -715, -196},
                {0xB23867FB2A35B28E, -688, -188},  {0x84C8D4DFD2C63F3B, -661, -180},
                {0xC5DD44271AD3CDBA, -635, -172},  {0x936B9FCEBB25C996, -608, -164},
                {0xDBAC6C247D62A584, -582, -156},  {0xA3AB66580D5FDAF6, -555, -148},
                {0xF3E2F893DEC3F126, -529, -140},  {0xB5B5ADA8AAFF80B8, -502, -132},
                {0x87625F056C7C4A8B, -475, -124},  {0xC9BCFF6034C13053, -449, -116},
                {0x964E858C91BA2655, -422, -108},  {0xDFF9772470297EBD, -396, -100},
                {0xA6DFBD9FB8E5B88F, -369, -92},   {0xF8A95FCF88747D94, -343, -84},
                {0xB94470938FA89BCF, -316, -76},   {0x8A08F0F8BF0F156B, -289, -68},
                {0xCDB02555653131B6, -263, -60},   {0x993FE2C6D07B7FAC, -236, -52},
                {0xE45C10C42A2B3B06, -210, -44},   {0xAA242499697392D3, -183, -36},
                {0xFD87B5F28300CA0E, -157, -28},   {0xBCE5086492111AEB, -130, -20},
                {0x8CBCCC096F5088CC, -103, -12},   {0xD1B71758E219652C, -77, -4},
                {0x9C40000000000000, -50, 4},      {0xE8D4A51000000000, -24, 12},
                {0xAD78EBC5AC620000, 3, 20},       {0x813F3978F8940984, 30, 28},
                {0xC097CE7BC90715B3, 56, 36},      {0x8F7E32CE7BEA5C70, 83, 44},
                {0xD5D238A4ABE98068, 109, 52},     {0x9F4F2726179A2245, 136, 60},
                {0xED63A231D4C4FB27, 162, 68},     {0xB0DE65388CC8ADA8, 189, 76},
                {0x83C7088E1AAB65DB, 216, 84},     {0xC45D1DF942711D9A, 242, 92},
                {0x924D692CA61BE758, 269, 100},    {0xDA01EE641A708DEA, 295, 108},
                {0xA26DA3999AEF774A, 322, 116},    {0xF209787BB47D6B85, 348, 124},
                {0xB454E4A179DD1877, 375, 132},    {0x865B86925B9BC5C2, 402, 140},
                {0xC83553C5C8965D3D, 428, 148},    {0x952AB45CFA97A0B3, 455, 156},
                {0xDE469FBD99A05FE3, 481, 164},    {0xA59BC234DB398C25, 508, 172},
                {0xF6C69A72A3989F5C, 534, 180},    {0xB7DCBF5354E9BECE, 561, 188},
                {0x88FCF317F22241E2, 588, 196},    {0xCC20CE9BD35C78A5, 614, 204},
                {0x98165AF37B2153DF, 641, 212},    {0xE2A0B5DC971F303A, 667, 220},
                {0xA8D9D1535CE3B396, 694, 228},    {0xFB9B7CD9A4A7443C, 720, 236},
                {0xBB764C4CA7A44410, 747, 244},    {0x8BAB8EEFB6409C1A, 774, 252},
                {0xD01FEF10A657842C, 800, 260},    {0x9B10A4E5E9913129, 827, 268},
                {0xE7109BFBA19C0C9D, 853, 276},    {0xAC2820D9623BF429, 880, 284},
                {0x80444B5E7AA7CF85, 907, 292},    {0xBF21E44003ACDD2D, 933, 300},
                {0x8E679C2F5E44FF8F, 960, 308},    {0xD433179D9C8CB841, 986, 316},
                {0x9E19DB92B4E31BA9, 1013, 324},
            };
            static_assert(( sizeof(cached) / sizeof(cached[0]) ) == 79, "Table error");
            constexpr int alpha = -60;
            constexpr int mindec_exp = -300;
            constexpr int step = 8;
            int f = alpha - e - 1;
            int k = (f * 78913) / (1 << 18) + static_cast<int>(f > 0);
            int index = (-mindec_exp + k + (step - 1)) / step;
            return cached[index];
        }

        static void grisu2_round(char* buffer, int len, uint64_t dist, uint64_t delta, uint64_t rest, uint64_t ten_k) noexcept{
            while((rest < dist) && (delta - rest >= ten_k) && 
                ((rest + ten_k < dist) || (dist - rest > rest + ten_k - dist))){
                    --buffer[len - 1];
                    rest += ten_k;
            }
        }

        template<class FloatType>
        static void calculate_boundaries(FloatType value, int& decimal_exp, diyfp& w, diyfp& plus, diyfp& minus) noexcept{
            constexpr int precision = std::numeric_limits<FloatType>::digits;
            constexpr int bias = std::numeric_limits<FloatType>::max_exponent - 1 + (precision - 1);
            constexpr int kminexp = 1 - bias;
            constexpr uint64_t hidden = static_cast<uint64_t>(1) << (precision - 1);
            using bits_type = typename std::conditional<precision == 24, uint32_t, uint64_t>::type;

            bits_type alias;
            memcpy(&alias, &value, sizeof(FloatType));
            uint64_t bits = alias;
            uint64_t E = bits >> (precision - 1);
            uint64_t F = bits & (hidden - 1);
            diyfp v = (E != 0) ? diyfp(F + hidden, static_cast<int>(E) - bias) : diyfp(F, kminexp);
            diyfp v_plus = diyfp(2 * v.f + 1, v.e - 1);
            diyfp v_minus = (F == 0 && E > 1) ? diyfp(4 * v.f - 1, v.e - 2) : diyfp(2 * v.f - 1, v.e - 1);
            v = v.normalize();
            v_plus = v_plus.normalize();
            v_minus = v_minus.normalize(v_plus.e);
            const cached_power cached = get_cached(v_plus.e);
            decimal_exp = -cached.k;
            const diyfp c_minus_k(cached.f, cached.e);

            v_minus = diyfp::mul(v_minus, c_minus_k);
            v_plus = diyfp::mul(v_plus, c_minus_k);

            // result
            w = diyfp::mul(v, c_minus_k);
            minus = diyfp(v_minus.f + 1, v_minus.e);
            plus = diyfp(v_plus.f - 1, v_plus.e);
        }

        template<class FloatType>
        static void algorithm(char* buffer, FloatType value, int& len, int& decimal_exp) noexcept{
            diyfp w, plus, minus;
            calculate_boundaries(value, decimal_exp, w, plus, minus);

            // digits gen
            uint64_t delta = diyfp::sub(plus, minus).f;
            uint64_t dist = diyfp::sub(plus, w).f;
            const diyfp one(static_cast<uint64_t>(1) << -plus.e, plus.e);
            uint32_t p1 = static_cast<uint32_t>(plus.f >> -one.e);
            uint64_t p2 = plus.f & (one.f - 1);

            {
                int kappa = xstring::count_digits(p1);
                while(kappa > 0){
                    uint32_t d = 0, pcase = 0;
                    switch(kappa){
                    case 9:{ constexpr uint32_t pow10 = 100000000; d = p1 / pow10; p1 %= pow10; pcase = pow10; break; }
                    case 8:{ constexpr uint32_t pow10 =  10000000; d = p1 / pow10; p1 %= pow10; pcase = pow10; break; }
                    case 7:{ constexpr uint32_t pow10 =   1000000; d = p1 / pow10; p1 %= pow10; pcase = pow10; break; }
                    case 6:{ constexpr uint32_t pow10 =    100000; d = p1 / pow10; p1 %= pow10; pcase = pow10; break; }
                    case 5:{ constexpr uint32_t pow10 =     10000; d = p1 / pow10; p1 %= pow10; pcase = pow10; break; }
                    case 4:{ constexpr uint32_t pow10 =      1000; d = p1 / pow10; p1 %= pow10; pcase = pow10; break; }
                    case 3:{ constexpr uint32_t pow10 =       100; d = p1 / pow10; p1 %= pow10; pcase = pow10; break; }
                    case 2:{ constexpr uint32_t pow10 =        10; d = p1 / pow10; p1 %= pow10; pcase = pow10; break; }
                    case 1:{ constexpr uint32_t pow10 =         1; d = p1        ; p1 =      0; pcase = pow10; break; }
                    }
                    buffer[len++] = static_cast<char>('0' + d);
                    kappa--;
                    uint64_t rest = (static_cast<uint64_t>(p1) << -one.e) + p2;
                    if(rest <= delta){
                        decimal_exp += kappa;
                        return grisu2_round(buffer, len, dist, delta, rest, static_cast<uint64_t>(pcase) << -one.e);
                    }
                }

                {
                    // kappa == 0
                    int m = 0;
                    while(true){
                        ++m;
                        delta *= 10;
                        dist *= 10;
                        p2 *= 10;
                        uint64_t d = p2 >> -one.e;
                        p2 &= (one.f - 1);
                        buffer[len++] = static_cast<char>('0' + d);
                        if(p2 <= delta){ break; }
                    }
                    decimal_exp -= m;
                    return grisu2_round(buffer, len, dist, delta, p2, one.f);
                }
            }
            // digits gen done
        }

        // TODO: add format support, aligns with std::to_chars
        static uint32_t formatter(char* buffer, int len, int decimal_exp, size_t kexpmin) noexcept{
            constexpr int kexpmax = std::numeric_limits<double>::digits10;
            const int k = len;
            const int n = len + decimal_exp;
            if(k <= n && n <= kexpmax){
                memset(buffer + k, '0', static_cast<size_t>(n - k));
                buffer[n + 0] = '.';
                buffer[n + 1] = '0';
                return n + 2;
            }
            if(n > 0 && n <= kexpmax){
                memmove(buffer + (static_cast<size_t>(n) + 1), buffer + n, static_cast<size_t>(k - n));
                buffer[n] = '.';
                return k + 1;
            }
            if(n <= 0 && kexpmin > static_cast<size_t>(-n)){
                memmove((buffer - n) + 2, buffer, static_cast<size_t>(k));
                buffer[0] = '0';
                buffer[1] = '.';
                memset(buffer + 2, '0', static_cast<size_t>(-n));
                return 2 + k + (-n);
            }

            const char* origin = buffer;
            if(k == 1){ buffer += 1; }
            else{
                memmove(buffer + 2, buffer + 1, static_cast<size_t>(k) - 1);
                buffer[1] = '.';
                buffer += static_cast<size_t>(k) + 1;
            }
            *buffer++ = 'e';
            int e = n - 1;
            if(e < 0){
                e = -e;
                *buffer++ = '-';
            }
            else{
                *buffer++ = '+';
            }
            if(e < 10){
                *buffer++ = '0';
                *buffer++ = static_cast<char>('0' + e);
            }
            else if(e < 100){
                *buffer++ = static_cast<char>('0' + e / 10);
                *buffer++ = static_cast<char>('0' + e % 10);
            }
            else{
                *buffer++ = static_cast<char>('0' + e / 100);
                *buffer++ = static_cast<char>('0' + (e % 100) / 10);
                *buffer++ = static_cast<char>('0' + ((e % 100) % 10));
            }
            return static_cast<uint32_t>(buffer - origin);
        }

        static uint32_t formmater_count(int len, int decimal_exp, size_t kexpmin) noexcept{
            constexpr int kexpmax = std::numeric_limits<double>::digits10;
            const int k = len;
            const int n = len + decimal_exp;
            if(k <= n && n <= kexpmax){
                return n + 2;
            }
            if(n > 0 && n <= kexpmax){
                return k + 1;
            }
            if(n <= 0 && kexpmin > static_cast<size_t>(-n)){
                return 2 + k + (-n);
            }
            uint32_t appened = k == 1 ? 1 : k + 1;
            appened += 2; // 'e- or e+'
            int e = n - 1;
            if(e < 0){ e = -e; }
            if(e < 10){ appened += 1; }
            else if(e < 100){ appened += 2; }
            else{ appened += 3; }
            return appened;
        }

        template<class FloatType>
        static uint32_t algorithm_count(FloatType value, size_t kexpmin) noexcept{
            int len = 0, decimal_exp = 0;
            diyfp w, plus, minus;
            calculate_boundaries(value, decimal_exp, w, plus, minus);

            //digits count
            uint64_t delta = diyfp::sub(plus, minus).f;
            const diyfp one(static_cast<uint64_t>(1) << -plus.e, plus.e);
            uint32_t p1 = static_cast<uint32_t>(plus.f >> -one.e);
            uint64_t p2 = plus.f & (one.f - 1);
            static constexpr uint32_t pow10_table[] = {0, 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000};
            
            {
                int kappa = xstring::count_digits(p1);
                while(kappa > 0){
                    p1 %= pow10_table[kappa];
                    len++;
                    kappa--;
                    uint64_t rest = (static_cast<uint64_t>(p1) << -one.e) + p2;
                    if(rest <= delta){
                        decimal_exp += kappa;
                        return formmater_count(len, decimal_exp, kexpmin);
                    }
                }
                {
                    int m = 0;
                    while(true){
                        ++m;
                        ++len;
                        delta *= 10;
                        p2 *= 10;
                        p2 &= (one.f - 1);
                        if(p2 <= delta){ break; }
                    }
                    decimal_exp -= m;
                }
            }
            return formmater_count(len, decimal_exp, kexpmin);
        }
    };
public:
    static uint32_t count_digits(float x, size_t precision = 6) noexcept{
        return grisu2::algorithm_count(x, precision);
    }
    static uint32_t count_digits(double x, size_t precision = 6) noexcept{
        return grisu2::algorithm_count(x, precision);
    }

    template<class T, typename std::enable_if<std::is_floating_point<T>::value, bool>::type = true>
    static uint32_t to_chars(char* buffer, T value, size_t precision = 6) noexcept{
        bool negative = std::signbit(value);
        if(negative){
            value = -value;
            *buffer++ = '-';
        }
        if(value == 0){
            *buffer++ = '0';
            *buffer++ = '.';
            *buffer = '0';
            return negative ? 4 : 3;
        }

        int len = 0;
        int decimal_exp = 0;
        grisu2::algorithm(buffer, value, len, decimal_exp);
        return grisu2::formatter(buffer, len, decimal_exp, precision);
    }

    template<class T, typename std::enable_if<std::is_floating_point<T>::value, bool>::type = true>
    static xstring to_string(T value, size_t min_precision=6) noexcept{
        static_assert(std::numeric_limits<T>::is_iec559,
        "xstring::to_string(FloatType): Unspport format, not ieee-754");
        char buf[64];
        uint32_t len = to_chars(buf, value, min_precision);
        return xstring(buf, len);
    }

    static double to_double(range<true> range, bool& success) noexcept{
        int mode = 0x00000001;
        int64_t integer = 0;
        double decimal = 0;
        int64_t exp = 0;
        const char* ptr = range.data();
        const char* end = ptr + range.size();
        char ch;
        while(*ptr == ' '){
            ++ptr;
            if(ptr >= end){ goto FAILURE; }
        }
        ch = *ptr;
        if(ch == '-'){
            mode |= 0x00000008;
            ++ptr;
            if(ptr >= end){ goto FAILURE; }
            ch = *ptr;
        }
        while(ch == '0'){
            ++ptr;
            if(ptr >= end){ break; }
            ch = *ptr;
        }
        if(ch >= '1' && ch <= '9'){
            integer = ch - '0';
            ++ptr;
            if(ptr < end){
                ch = *ptr;
                while(ch >= '0' && ch <= '9'){
                    integer = 10 * integer + (ch - '0');
                    ++ptr;
                    if(ptr >= end){ break; }
                    ch = *ptr;
                }   
            }
        }
        if(ch == '.'){
            ++ptr;
            if(ptr < end){
                ch = *ptr;
                double tmp = 0.01;
                mode |= 0x00000002;
                while(ch >= '0' && ch <= '9'){
                    decimal = decimal + tmp * (ch - '0');
                    ++ptr;
                    if(ptr >= end){ break; }
                    tmp *= 0.1;
                    ch = *ptr;
                }
            }
        }
        if(ch == 'e' || ch == 'E'){
            mode |= 0x00000004;
            ++ptr;
            if(ptr >= end){ goto FAILURE; }
            // no break
            switch(*ptr)
            {
            case '-':{ mode |= 0x00000010; }
            case '+':{ if(++ptr >= end) { goto FAILURE; } }
            default:{
                ch = *ptr;
                if(ch < '0' && ch > '9'){ goto FAILURE; } //invalid
                exp = ch - '0';
                ++ptr;
                if(ptr < end){
                    ch = *(++ptr);
                    while(ch >= '0' && ch <= '9'){
                        exp = exp * 10 + (ch - '0');
                        ++ptr;
                        if(ptr >= end){ break; }
                        ch = *ptr;
                    }
                }
            }
            }
        }
        while(ptr < end){
            if(*ptr != ' '){ goto FAILURE; }
            ++ptr;
        }
        double ret;
        if(mode & 0x00000004){
            double x = integer + decimal;
            double e = std::pow(10.0, mode & 0x00000010 ? -exp : exp);
            ret =  mode & 0x00000008 ? -(x * e) : x * e;
        }
        else if(mode & 0x00000002){
            double x = integer + decimal;
            ret = mode & 0x00000008 ? -x : x;
        }
        else{
            double x = static_cast<double>(integer);
            ret = mode & 0x00000008 ? -x : x;
        }
        success = true;
        return ret;
    FAILURE:
        success = false;
        return NAN;
    }
};

#ifndef XSTRING_CPP17
constexpr uint64_t xstring::bit_table[128];
#endif
#endif