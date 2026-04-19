#ifndef xjson_hpp
#define xjson_hpp
#include"xstring.hpp"
#include<vector>
#include<cmath>
#include<map>
#include<cstring>
#include<utility>
#include<type_traits>
#include<cstddef>
#include<initializer_list>

#if ((defined(_MSVC_LANG) && _MSVC_LANG >= 201402L) || __cplusplus >= 201402L)
#else
#ifndef XJSON_CPP11
#define XJSON_CPP11
#endif
#endif

class xjson{
public:
    enum TYPE: int{
        NULLPTR = 0x00000000,
        STRING,
        ARRAY,
        OBJECT,
        INT64,
        DOUBLE,
        BOOL,
    };
    using Int = int64_t;
    using Double = double;
    using Bool = bool;
    using Null = std::nullptr_t;
    using String = xstring;
    using Array = std::vector<xjson>;
    #ifndef XJSON_CPP11
    using Object = std::map<String, xjson, std::less<>>;
    #else
    using Object = std::map<String, xjson>;
    #endif
private:
    // details
    template<class T>
    struct always_false{ static constexpr bool value = false; };

    template<class T>
    using decay_t = typename std::decay<T>::type;

    template<class T>
    struct check_type{
        static constexpr bool value = std::is_same<T, int64_t>::value
            || std::is_same<T, double>::value
            || std::is_same<T, bool>::value
            || std::is_same<T, String>::value
            || std::is_same<T, Array>::value
            || std::is_same<T, Object>::value
            || std::is_same<T, std::nullptr_t>::value;
    };
    template<class T>
    struct is_keytype{
        static constexpr bool isrange = std::is_convertible<T, String::range<true>>::value
                                        || std::is_convertible<T, String::range<false>>::value;
        static constexpr bool isstring = std::is_convertible<T, xstring>::value;
        static constexpr bool value = isrange || isstring;
        using range_type = String::range<std::is_convertible<T, String::range<true>>::value>;
    };

    template<class T>
    struct is_iterable{
    private:
        template<class U>
        struct is_initializer{ static constexpr bool value = false; };
        template<class E>
        struct is_initializer<std::initializer_list<E>>{ static constexpr bool value = true; };

        template<class U, bool iterable>
        struct traits{
            using data_type = void;
            static constexpr bool isarray = false;
            static constexpr bool isobject = false;
        };
        template<class U>
        struct traits<U, true>{
            using data_type = decay_t<decltype(*std::begin(std::declval<U>()))>;
            static constexpr bool isarray = std::is_convertible<data_type, xjson>::value && !is_initializer<U>::value;
            static constexpr bool isobject = std::is_convertible<data_type, std::pair<const String, xjson>>::value && !is_initializer<U>::value;
        };

        template<class U>
        static auto test(int) -> decltype(
            std::begin(std::declval<U>()),
            std::end(std::declval<U>()),
            std::true_type()
        );
        template<class U>
        static std::false_type test(...);
    public:
        static constexpr bool value = decltype(test<T>(0))::value;
        using xtraits = traits<T, value>;
        using data_type = typename xtraits::data_type;
        static constexpr bool isarray = xtraits::isarray;
        static constexpr bool isobject = xtraits::isobject;
    };
    
    class TYPE_GETTER{
        friend class xjson;
        xjson::TYPE type;
        TYPE_GETTER() = default;
        TYPE_GETTER(xjson::TYPE t) noexcept: type(t){}
    public:
        operator int() const noexcept { return static_cast<int>(type); }
        operator xjson::TYPE() const noexcept { return type; }
        operator const char*() const noexcept { return this->name(); }
        const char* name() const noexcept{
            constexpr const char* names[] = {
                "null",
                "xjson::String",
                "xjson::Array",
                "xjson::Object",
                "int64",
                "double",
                "bool"
            };
            return names[type];
        };
        template<class T>
        void operator=(T&& value){
            static_assert(always_false<T>::value, "xjson::type is read only");
        }
    };

    struct unsafe_writter{
        xstring::decay_raw raw;
        void operator+=(xstring::range<true> range) noexcept{
            size_t size = range.size();
            memcpy(raw.p, range.data(), size);
            raw.p += size;
        }
    };

    class RefWrapper{
        friend class xjson;
        Object* object;
        Object::iterator it;
        String key;
        template<class T>
        RefWrapper(Object* o, Object::iterator i, T&& k) noexcept: object(o), it(i), key(std::forward<T>(k))
        {}
    public:
        template<class T>
        RefWrapper& operator=(T&& val) noexcept{
            if(it == object->end()) it = object->emplace(std::move(key), std::forward<T>(val)).first;
            else it->second = std::forward<T>(val);
            return *this;
        }

        template<class T, typename std::enable_if<check_type<decay_t<T>>::value || std::is_same<decay_t<T>, xjson>::value, bool>::type = true>
        operator T&() noexcept{
            return static_cast<T&>(it->second);
        }
        template<class T, typename std::enable_if<check_type<decay_t<T>>::value || std::is_same<decay_t<T>, xjson>::value, bool>::type = true>
        operator const T&() const noexcept{
            return static_cast<const T&>(it->second);
        }
        template<class T, typename std::enable_if<std::is_convertible<xjson&, T>::value && !std::is_convertible<const xjson&, T>::value, bool>::type = true>
        operator T() noexcept{
            return static_cast<xjson&>(it->second);
        }
        template<class T, typename std::enable_if<std::is_convertible<const xjson&, T>::value, bool>::type = true>
        operator T() noexcept{
            return static_cast<const xjson&>(it->second);
        }
        template<class T, typename std::enable_if<std::is_convertible<const xjson&, T>::value, bool>::type = true>
        operator T() const noexcept{
            return static_cast<const xjson&>(it->second);
        }

        template<bool count_before_dump = true, typename std::enable_if<count_before_dump, bool>::type = true>
        String dump() const noexcept{
            uint32_t count = 0;
            count_recursion(*this, count);
            String str(count);
            xjson::dump_recursion(it->second, str);
            return str;
        }

        template<bool count_before_dump, typename std::enable_if<!count_before_dump, bool>::type = true>
        String dump() const noexcept{
            String str;
            xjson::dump_recursion(it->second, str);
            return str;
        }

        template<typename... Args>
        static xjson parse(Args&&... args) noexcept{
            return xjson::parse(std::forward<Args>(args)...);
        }

        template<class T, typename std::enable_if<!std::is_integral<decay_t<T>>::value, bool>::type = true>
        RefWrapper operator[](T&& key) noexcept{
            return it->second[std::forward<T>(key)];
        }
        template<class T, typename std::enable_if<!std::is_integral<decay_t<T>>::value, bool>::type = true>
        const xjson& operator[](T&& key) const noexcept{
            return it->second[std::forward<T>(key)];
        }

        xjson& operator[](size_t idx) noexcept{
            return it->second[idx];
        }
        const xjson& operator[](size_t idx) const noexcept{
            return it->second[idx];
        }

        template<class T>
        T& as() noexcept{
            static_assert(check_type<T>::value, "xjson::as(): Not a support type");
            return static_cast<T&>(*this);
        }
        template<class T>
        const T& as() const noexcept{
            static_assert(check_type<T>::value, "xjson::as(): Not a support type");
            return static_cast<T&>(*this);
        }

        bool isint() const noexcept{ return it->second.type.type == xjson::TYPE::INT64; }
        bool isdouble() const noexcept{ return it->second.type.type == xjson::TYPE::DOUBLE; }
        bool isnull() const noexcept{ return it->second.type.type == xjson::TYPE::NULLPTR; }
        bool isbool() const noexcept{ return it->second.type.type == xjson::TYPE::BOOL; }
        bool isstring() const noexcept{ return it->second.type.type == xjson::TYPE::STRING; }
        bool isarray() const noexcept{ return it->second.type.type == xjson::TYPE::ARRAY; }
        bool isobject() const noexcept{ return it->second.type.type == xjson::TYPE::OBJECT; }
        template<class T, typename std::enable_if<is_keytype<T>::isstring && !is_keytype<T>::isrange, bool>::type = true>
        Object::iterator has_key(T&& key, bool& haskey) noexcept{
            auto ret = it->second.o->find(std::forward<T>(key));
            haskey = (ret != it->second.o->end());
            return ret;
        }
        template<class T, typename std::enable_if<is_keytype<T>::isrange, bool>::type = true>
        Object::iterator has_key(T&& key, bool& haskey) noexcept{
            using range_type = typename is_keytype<T>::range_type;
            auto ret = it->second.o->find(static_cast<range_type>(std::forward<T>(key)));
            haskey = (ret != it->second.o->end());
            return ret;
        }
        template<class T, typename std::enable_if<is_keytype<T>::isstring && !is_keytype<T>::isrange, bool>::type = true>
        Object::const_iterator has_key(T&& key, bool& haskey) const noexcept{
            auto ret = it->second.o->find(std::forward<T>(key));
            haskey = (ret != it->second.o->end());
            return ret;
        }
        template<class T, typename std::enable_if<is_keytype<T>::isrange, bool>::type = true>
        Object::const_iterator has_key(T&& key, bool& haskey) const noexcept{
            using range_type = typename is_keytype<T>::range_type;
            auto ret = it->second.o->find(static_cast<range_type>(std::forward<T>(key)));
            haskey = (ret != it->second.o->end());
            return ret;
        }
    };

    static void count_recursion(const xjson& v, uint32_t& count) noexcept{
        switch(v.type.type){
        case xjson::TYPE::NULLPTR:{
            count += 4;
            break;
        }
        case xjson::TYPE::STRING: {
            count += 2 + v.s->size<uint32_t>();
            break;
        }
        case xjson::TYPE::ARRAY:{
            count += 2;
            size_t size = v.a->size();
            if(size){
                count += static_cast<uint32_t>(size - 1);
                for(auto it = v.a->cbegin(); it != v.a->cend(); ++it){
                    count_recursion(*it, count);
                }
            }
            break;
        }
        case xjson::TYPE::OBJECT:{
            count += 2;
            size_t size = v.o->size();
            if(size){
                count += static_cast<uint32_t>((4 * size) - 1);
                for(auto it = v.o->cbegin(); it != v.o->cend(); ++it){
                    count += it->first.size<uint32_t>();
                    count_recursion(it->second, count);
                }
            }
            break;
        }
        case xjson::TYPE::INT64: {
            count += String::count_digits(v.i);
            break;
        }
        case xjson::TYPE::DOUBLE: {
            count += String::count_digits(v.d);
            break;
        }
        case xjson::TYPE::BOOL: {
            count += v.b ? 4 : 5;
            break;
        }
        }
    }

    template<class T>
    static void dump_recursion(const xjson& v, T& str) noexcept{
        switch(v.type.type)
        {
        case xjson::TYPE::NULLPTR: {
            str += "null";
            break;
        }
        case xjson::TYPE::STRING: {
            str += "\"";
            str += *v.s;
            str += "\"";
            break;
        }
        case xjson::TYPE::ARRAY: {
            if(v.a->empty()) str += "[]";
            else
            {
                str += "[";
                auto it = v.a->cbegin();
                for(; it != v.a->cend() - 1; ++it){
                    dump_recursion(*it, str);
                    str += ",";
                }
                dump_recursion(*it, str);
                str += "]";
            }
            break;
        }
        case xjson::TYPE::OBJECT: {
            if(v.o->empty()) str += "{}";
            else
            {
                str += "{";
                auto it = v.o->cbegin();
                auto end = --v.o->cend();
                for(; it != end; ++it){
                    str += "\"";
                    str += it->first;
                    str += "\":";
                    dump_recursion(it->second, str);
                    str += ",";
                }
                str += "\"";
                str += it->first;
                str += "\":";
                dump_recursion(it->second, str);
                str += "}";
            }
            break;
        }
        case xjson::TYPE::INT64: {
            str += String::to_string(v.i);
            break;
        }
        case xjson::TYPE::DOUBLE: {
            str += String::to_string(v.d);
            break;
        }
        case xjson::TYPE::BOOL: {
            if(v.b) str += "true";
            else str += "false";
            break;
        }
        default:{ break; }
        }
    }

    // enum OUTPUT_MODE{
    //     INT = 0x00000001,
    //     FLOAT = 0x00000002,
    //     EXP = 0x00000004,
    //     NEGATIVE = 0x00000008,
    //     EXP_NEGATIVE = 0x00000010
    // };
    static const char* parse_recursion(const char* ptr, const char* end, xjson& v) noexcept{
        constexpr const char* skipcmp = ", \n\t\r";
        constexpr const char* blankcmp = skipcmp + 1;
        if(ptr < end){
            char ch = *ptr;
            while(memchr(blankcmp, ch, 4) != nullptr){
                ++ptr;
                if(ptr >= end){ return nullptr; }
                ch = *ptr;
            }
            switch(ch)
            {
            case '\"':{
                const char* begin = ptr + 1;
                do{
                    ++ptr;
                    ptr = static_cast<const char*>(memchr(ptr, '\"', end - ptr));
                    if(ptr == nullptr){ return nullptr; }
                }while(*(ptr - 1) == '\\' && *(ptr - 2) != '\\');
                v.s = new xjson::String(begin, static_cast<uint32_t>(ptr - begin));
                v.type.type = xjson::TYPE::STRING;
                ptr++;
                break;
            }
            case '[':{
                ch = *(++ptr);
                while(memchr(blankcmp, ch, 4) != nullptr){
                    ++ptr;
                    if(ptr >= end){ return nullptr; }
                    ch = *ptr;
                }
                v.a = new xjson::Array();
                v.type.type = xjson::TYPE::ARRAY;
                while(ch != ']')
                {
                    v.a->emplace_back();
                    ptr = xjson::parse_recursion(ptr, end, v.a->back());
                    if(ptr == nullptr){ return nullptr; }
                    ch = *ptr;
                    while(memchr(skipcmp, ch, 5) != nullptr){
                        ++ptr;
                        if(ptr > end){ return nullptr; }
                        ch = *ptr;
                    }
                }
                ++ptr;
                break;
            }
            case 'f':{
                if(memcmp(ptr, "false", 5) != 0 || ptr + 5 > end){ return nullptr; }
                v.b = false;
                v.type.type = xjson::TYPE::BOOL;
                ptr += 5;
                break;
            }
            case 'n':{
                if(memcmp(ptr, "null", 4) != 0 || ptr + 4 > end){ return nullptr; }
                v.n = nullptr;
                v.type.type = xjson::TYPE::NULLPTR;
                ptr += 4;
                break;
            }
            case 't':{
                if(memcmp(ptr, "true", 4) != 0 || ptr + 4 > end){ return nullptr; }
                v.b = true;
                v.type.type = xjson::TYPE::BOOL;
                ptr += 4;
                break;
            }
            case '{':{
                ch = *(++ptr);
                while(memchr(blankcmp, ch, 4) != nullptr){
                    ++ptr;
                    if(ptr >= end){ return nullptr; }
                    ch = *ptr;
                }
                v.o = new xjson::Object();
                v.type.type = xjson::TYPE::OBJECT;
                while(ch != '}'){
                    const char* res;
                    if(ch != '\"'){ return nullptr; }
                    ++ptr;
                    res = static_cast<const char*>(memchr(ptr, '"', end - ptr));
                    if(res != nullptr){
                        const char* colon = static_cast<const char*>(memchr(res + 1, ':', end - res + 1));
                        if(colon != nullptr){
                            ptr = xjson::parse_recursion(
                                colon + 1,
                                end,
                                v.o->emplace(std::piecewise_construct,
                                    std::forward_as_tuple(ptr, res - ptr),
                                    std::forward_as_tuple(nullptr)
                                ).first->second
                            );
                            if(ptr != nullptr){
                                ch = *ptr;
                                while(memchr(skipcmp, ch, 5) != nullptr){
                                    ++ptr;
                                    if(ptr >= end){ return nullptr; }
                                    ch = *ptr;
                                    continue;
                                }
                            }
                        }
                    }
                }
                ++ptr;
                break;
            }
            default:{
                int mode = 0x00000001;
                int64_t integer = 0;
                double decimal = 0;
                double exp = 0;
                if(ch == '-'){
                    mode |= 0x00000008;
                    ch = *(++ptr);
                }
                if(ch == '0'){
                    ch = *(++ptr);
                }
                else if(ch >= '1' && ch <= '9'){
                    integer = ch - '0';
                    ch = *(++ptr);
                    while(ch >= '0' && ch <= '9'){
                        integer = 10 * integer + ch - '0';
                        ch = *(++ptr);
                    }
                }
                else { return nullptr; }

                if(ch == '.')
                {
                    ch = *(++ptr);
                    if(ch < '0' || ch > '9'){ return nullptr; } //invalid
                    decimal = 0.1 * (ch - '0');
                    ch = *(++ptr);
                    mode |= 0x00000002;
                    double tmp = 0.01;
                    while(ch >= '0' && ch <= '9'){
                        decimal = decimal + tmp * (ch - '0');
                        tmp *= 0.1;
                        ch = *(++ptr);
                    }
                }

                if(ch == 'e' || ch == 'E'){
                    mode |= 0x00000004;
                    // no break
                    switch(*(++ptr))
                    {
                    case '-':{ mode |= 0x00000010; }
                    case '+':{ ++ptr; }
                    default:{
                        ch = *ptr;
                        if(ch < '0' && ch > '9'){ return nullptr; } //invalid
                        exp = ch - '0';
                        ch = *(++ptr);
                        while(ch >= '0' && ch <= '9'){
                            exp = exp * 10 + ch - '0';
                            ch = *(++ptr);
                        }
                    }
                    }
                }
                if(mode & 0x00000004){
                    double x = integer + decimal;
                    double e = pow(10.0, mode & 0x00000010 ? -exp : exp);
                    v.d = mode & 0x00000008 ? -(x * e) : x * e;
                    v.type.type = xjson::TYPE::DOUBLE;
                }
                else if(mode & 0x00000002){
                    double x = integer + decimal;
                    v.d = mode & 0x00000008 ? -x : x;
                    v.type.type = xjson::TYPE::DOUBLE;
                }
                else{
                    v.i = mode & 0x00000008 ? -integer : integer;
                    v.type.type = xjson::TYPE::INT64;
                }
            }
            }
        }
        return ptr;
    }

private:
    // member
    union
    {
        int64_t i;
        double d;
        bool b;
        String* s;
        Array* a;
        Object* o;
        std::nullptr_t n;
    };
public:
    TYPE_GETTER type;
    // constructors
    xjson() noexcept: n(nullptr), type(xjson::TYPE::NULLPTR)
    {}
    ~xjson(){
        switch(type.type)
        {
        case xjson::TYPE::STRING : { delete s; break; }
        case xjson::TYPE::ARRAY  : { delete a; break; }
        case xjson::TYPE::OBJECT : { delete o; break; }
        default:{ break; }
        }
    }
    xjson(const xjson& other) noexcept{
        switch(type.type = other.type.type)
        {
        case xjson::TYPE::STRING : { s = new String(*other.s); break; }
        case xjson::TYPE::ARRAY  : { a = new Array(*other.a);  break; }
        case xjson::TYPE::OBJECT : { o = new Object(*other.o); break; }
        default: { memcpy(static_cast<void*>(this), static_cast<const void*>(&other), sizeof(xjson)); }
        }
    }
    // copy && swap for preventing json["children"] = json;
    xjson& operator=(const xjson& other) noexcept{
        union DESTRUCTOR_FREE{
            DESTRUCTOR_FREE(){}
            ~DESTRUCTOR_FREE(){}
            xjson js;
            unsigned char bytes[sizeof(xjson)];
        };
        DESTRUCTOR_FREE tmp;
        switch(tmp.js.type.type = other.type.type)
        {
        case xjson::TYPE::STRING: {tmp.js.s = new String(*other.s); break; }
        case xjson::TYPE::ARRAY : {tmp.js.a = new Array(*other.a);  break; }
        case xjson::TYPE::OBJECT: {tmp.js.o = new Object(*other.o); break; }
        default: {memcpy(static_cast<void*>(&tmp.js), static_cast<const void*>(&other), sizeof(xjson)); }
        }
        this->~xjson();
        memcpy(static_cast<void*>(this), static_cast<const void*>(&tmp.js), sizeof(xjson));
        return *this;
    }
    xjson(xjson&& other) noexcept{
        memcpy(static_cast<void*>(this), static_cast<const void*>(&other), sizeof(xjson));
        other.type.type = xjson::TYPE::NULLPTR;
    }
    xjson& operator=(xjson&& other) noexcept{
        this->~xjson();
        memcpy(static_cast<void*>(this), static_cast<const void*>(&other), sizeof(xjson));
        other.type.type = xjson::TYPE::NULLPTR;
        return *this;
    }
public:
    template<class T, typename std::enable_if<std::is_integral<T>::value, bool>::type = true>
    xjson(T x) noexcept: i(static_cast<int64_t>(x)), type(xjson::TYPE::INT64){}
    template<class T, typename std::enable_if<std::is_integral<T>::value, bool>::type = true>
    xjson& operator=(T x) noexcept{
        this->~xjson();
        i = static_cast<int64_t>(x);
        type.type = xjson::TYPE::INT64;
        return *this;
    }

    template<class T, typename std::enable_if<std::is_same<T, double>::value, bool>::type = true>
    xjson(T x) noexcept: d(x), type(xjson::TYPE::DOUBLE){}
    template<class T, typename std::enable_if<std::is_same<T, double>::value, bool>::type = true>
    xjson& operator=(T x) noexcept{
        this->~xjson();
        d = x;
        type.type = xjson::TYPE::DOUBLE;
        return *this;
    }

    xjson(bool x) noexcept: b(x), type(xjson::TYPE::BOOL){}
    xjson& operator=(bool x) noexcept{
        this->~xjson();
        b = x;
        type.type = xjson::TYPE::BOOL;
        return *this;
    }

    xjson(std::nullptr_t x) noexcept: n(x), type(xjson::TYPE::NULLPTR){}
    xjson& operator=(std::nullptr_t x) noexcept{
        this->~xjson();
        n = x;
        type.type = xjson::TYPE::NULLPTR;
        return *this;
    }

    template<uint32_t N>
    xjson(const char(&cptr)[N]) 
        noexcept: s(new String(std::forward<decltype(cptr)>(cptr))), type(xjson::TYPE::STRING){}
    template<uint32_t N>
    xjson& operator=(const char(&cptr)[N]) noexcept{
        this->~xjson();
        s = new String(std::forward<decltype(cptr)>(cptr));
        type.type = xjson::TYPE::STRING;
        return *this;
    }

    template<class T, typename std::enable_if<std::is_same<decay_t<T>, String>::value, bool>::type = true>
    xjson(T&& x) noexcept: s(new String(std::forward<T>(x))), type(xjson::TYPE::STRING){}
    template<class T, typename std::enable_if<std::is_same<decay_t<T>, String>::value, bool>::type = true>
    xjson& operator=(T&& x) noexcept{
        this->~xjson();
        s = new String(std::forward<T>(x));
        type.type = xjson::TYPE::STRING;
        return *this;
    }

    template<class T, typename std::enable_if<std::is_same<decay_t<T>, Array>::value, bool>::type = true>
    xjson(T&& x) noexcept: a(new Array(std::forward<T>(x))), type(xjson::TYPE::ARRAY){}
    template<class T, typename std::enable_if<std::is_same<decay_t<T>, Array>::value, bool>::type = true>
    xjson& operator=(T&& x) noexcept{
        this->~xjson();
        a = new Array(std::forward<T>(x));
        type.type = xjson::TYPE::ARRAY;
        return *this;
    }
    template<class T, typename std::enable_if<!check_type<decay_t<T>>::value && is_iterable<decay_t<T>>::isarray, bool>::type = true>
    xjson(T&& x) noexcept: a(new Array()), type(xjson::TYPE::ARRAY){
        a->reserve(x.size());
        for (const auto& v : x) {
            this->a->emplace_back(v);
        }
    }
    template<class T, typename std::enable_if<!check_type<decay_t<T>>::value && is_iterable<decay_t<T>>::isarray, bool>::type = true>
    xjson& operator=(T&& x){
        this->~xjson();
        a = new Array();
        type.type = xjson::TYPE::ARRAY;
        a->reserve(x.size());
        for (const auto& v : x) {
            this->a->emplace_back(v);
        }
        return *this;
    }

    template<class T, typename std::enable_if<std::is_same<decay_t<T>, Object>::value, bool>::type = true>
    xjson(T&& x) noexcept: o(new Object(std::forward<T>(x))), type(xjson::TYPE::OBJECT){}
    template<class T, typename std::enable_if<std::is_same<decay_t<T>, Object>::value, bool>::type = true>
    xjson& operator=(T&& x) noexcept{
        this->~xjson();
        o = new Object(std::forward<T>(x));
        type.type = xjson::TYPE::OBJECT;
        return *this;
    }

    using dict = std::initializer_list<std::pair<xstring::range<true>, xjson>>;
    xjson(dict ls) noexcept: o(new Object(ls.begin(), ls.end())), type(xjson::TYPE::OBJECT){}
    xjson& operator=(dict&& ls) noexcept{
        this->~xjson();
        o = new Object(ls.begin(), ls.end());
        type.type = xjson::TYPE::OBJECT;
        return *this;
    }

    template<class T, typename std::enable_if<!check_type<decay_t<T>>::value && is_iterable<decay_t<T>>::isobject, bool>::type = true>
    xjson(T&& x) noexcept: o(new Object()), type(xjson::TYPE::OBJECT){
        for (const auto& v : x) {
            this->o->emplace(v);
        }
    }
    template<class T, typename std::enable_if<!check_type<decay_t<T>>::value && is_iterable<decay_t<T>>::isobject, bool>::type = true>
    xjson& operator=(T&& x){
        this->~xjson();
        o = new Object();
        type.type = xjson::TYPE::OBJECT;
        for (const auto& v : x) {
            this->o->emplace(v);
        }
        return *this;
    }
public:
    // implicit conversion
    template<class T, typename std::enable_if<std::is_integral<T>::value && !std::is_same<T, int64_t>::value, bool>::type = true>
    operator T() const noexcept{ return static_cast<T>(i); } 
    operator int64_t&() noexcept{ return i; }
    operator const int64_t&() const noexcept{ return i; }
    operator double&() noexcept{ return d; }
    operator const double&() const noexcept{ return d; }
    operator bool&() noexcept { return b; }
    operator const bool&() const noexcept{ return b; }
    operator String&() noexcept{ return *s; }
    operator const String&() const noexcept{ return *s; }
    operator Array&() noexcept{ return *a; }
    operator const Array&() const noexcept{ return *a; }
    operator Object&() noexcept{ return *o; }
    operator const Object&() const noexcept{ return *o; }
    operator std::nullptr_t&() noexcept{ return n; }
    operator const std::nullptr_t&() const noexcept{ return n; }
public:
    // utils
    bool isint() const noexcept{ return type.type == xjson::TYPE::INT64; }
    bool isdouble() const noexcept{ return type.type == xjson::TYPE::DOUBLE; }
    bool isnull() const noexcept{ return type.type == xjson::TYPE::NULLPTR; }
    bool isbool() const noexcept{ return type.type == xjson::TYPE::BOOL; }
    bool isstring() const noexcept{ return type.type == xjson::TYPE::STRING; }
    bool isarray() const noexcept{ return type.type == xjson::TYPE::ARRAY; }
    bool isobject() const noexcept{ return type.type == xjson::TYPE::OBJECT; }
    void swap(xjson& other) noexcept{ std::swap(this->o, other.o); std::swap(this->type.type, other.type.type); }
    static void swap(xjson& lhs, xjson& rhs) noexcept{ lhs.swap(rhs); }
    template<class T, typename std::enable_if<is_keytype<T>::isstring && !is_keytype<T>::isrange, bool>::type = true>
    Object::iterator has_key(T&& key, bool& haskey) noexcept{
        auto ret = o->find(std::forward<T>(key));
        haskey = (ret != o->end());
        return ret;
    }
    template<class T, typename std::enable_if<is_keytype<T>::isrange, bool>::type = true>
    Object::iterator has_key(T&& key, bool& haskey) noexcept{
        using range_type = typename is_keytype<T>::range_type;
        auto ret = o->find(static_cast<range_type>(std::forward<T>(key)));
        haskey = (ret != o->end());
        return ret;
    }
    template<class T, typename std::enable_if<is_keytype<T>::isstring && !is_keytype<T>::isrange, bool>::type = true>
    Object::const_iterator has_key(T&& key, bool& haskey) const noexcept{
        auto ret = o->find(std::forward<T>(key));
        haskey = (ret != o->end());
        return ret;
    }
    template<class T, typename std::enable_if<is_keytype<T>::isrange, bool>::type = true>
    Object::const_iterator has_key(T&& key, bool& haskey) const noexcept{
        using range_type = typename is_keytype<T>::range_type;
        auto ret = o->find(static_cast<range_type>(std::forward<T>(key)));
        haskey = (ret != o->end());
        return ret;
    }

    template<class T>
    T& as() noexcept{
        static_assert(check_type<T>::value, "xjson::as(): Not a support type");
        return static_cast<T&>(*this);
    }
    template<class T>
    const T& as() const noexcept{
        static_assert(check_type<T>::value, "xjson::as(): Not a support type");
        return static_cast<T&>(*this);
    }

    template<class T, 
    typename std::enable_if<is_keytype<T>::isstring && !is_keytype<T>::isrange && !std::is_same<decay_t<T>, String>::value, bool>::type = true>
    RefWrapper operator[](T&& key) noexcept{
        // need conversion
        String temp(std::forward<T>(key));
        auto it = this->o->find(temp);
        return RefWrapper(this->o, it, it == this->o->end() ? std::move(temp) : String());
    }
    template<class T, typename std::enable_if<std::is_same<decay_t<T>, String>::value, bool>::type = true>
    RefWrapper operator[](T&& key) noexcept{
        auto it = this->o->find(key);
        return RefWrapper(this->o, it, it == this->o->end() ? std::forward<T>(key) : String());
    }
    template<class T, typename std::enable_if<is_keytype<T>::isrange && !std::is_same<decay_t<T>, String>::value, bool>::type = true>
    RefWrapper operator[](T&& key) noexcept{
        using range_type = typename is_keytype<T>::range_type;
        range_type range = static_cast<range_type>(std::forward<T>(key));
        auto it = this->o->find(range);
        return RefWrapper(this->o, it, it == this->o->end() ? range.to_string() : String());
    }
    template<class T, typename std::enable_if<is_keytype<T>::isstring && !is_keytype<T>::isrange, bool>::type = true>
    const xjson& operator[](T&& key) const noexcept{
        return o->find(std::forward<T>(key))->second;
    }
    template<class T, typename std::enable_if<is_keytype<T>::isrange, bool>::type = true>
    const xjson& operator[](T&& key) const noexcept{
        using range_type = typename is_keytype<T>::range_type;
        return o->find(static_cast<range_type>(std::forward<T>(key)))->second;
    }
    xjson& operator[](size_t idx) noexcept{
        return (*a)[idx];
    }
    const xjson& operator[](size_t idx) const noexcept{
        return (*a)[idx];
    }

    template<bool count_before_dump = true, typename std::enable_if<count_before_dump, bool>::type = true>
    String dump() const noexcept{
        uint32_t count = 0;
        count_recursion(*this, count);
        String str(count);
        unsafe_writter writter{str.decay()};
        xjson::dump_recursion(*this, writter);
        *writter.raw.p = '\0';
        writter.raw.update(count);
        return str;
    }

    template<bool count_before_dump, typename std::enable_if<!count_before_dump, bool>::type = true>
    String dump() const noexcept{
        String str;
        xjson::dump_recursion(*this, str);
        return str;
    }

    static xjson parse(const String::range<true> x) noexcept{
        const char* ptr = x.data();
        xjson js;
        if(xjson::parse_recursion(ptr, ptr + x.size(), js) == nullptr){
            js = nullptr;
        }
        return js;
    }
    template<class T, typename std::enable_if<!is_keytype<T>::value, bool>::type = true>
    static xjson parse(T&& x) noexcept{
        const char* ptr = x.data();
        xjson js;
        if(xjson::parse_recursion(ptr, ptr + x.size(), js) == nullptr){
            js = nullptr;
        }
        return js;
    }
    static xjson parse(const void* begin, const void* end) noexcept{
        xjson js;
        using pointer = const char*;
        if(xjson::parse_recursion(static_cast<pointer>(begin), static_cast<pointer>(end), js) == nullptr){
            js = nullptr;
        }
        return js;
    }
    static xjson parse(const void* begin, size_t size) noexcept{
        xjson js;
        using pointer = const char*;
        if(xjson::parse_recursion(static_cast<pointer>(begin), static_cast<pointer>(begin) + size, js) == nullptr){
            js = nullptr;
        }
        return js;
    }
};

#endif