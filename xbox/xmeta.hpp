#ifndef xmeta_hpp
#define xmeta_hpp
#if !((defined(_MSVC_LANG) && _MSVC_LANG >= 201402L) || __cplusplus >= 201402L)
#error xmeta.hpp c++14 required
#endif
#include<tuple>
#include<cstddef>

namespace xmeta{
    template<class T>
    struct always_false: std::false_type{};
    template<class T>
    struct always_true: std::true_type{};

    template <typename T>
    struct function_traits: public function_traits<decltype(&T::operator())>{};
    template <typename Ret, typename... Args >
    struct function_traits<Ret(Args...)>: public function_traits<Ret(*)(Args...)>{};
    template <typename Ret, typename... Args >
    struct function_traits<Ret(&)(Args...)>: public function_traits<Ret(*)(Args...)>{};
    template <typename Ret, typename... Args >
    struct function_traits<Ret(&&)(Args...)>: public function_traits<Ret(*)(Args...)>{};

    template <typename Ret, typename... Args >
    struct function_traits<Ret(*)(Args...)>
    {
        using ret_type = Ret;
        using args_type = std::tuple<Args...>;
        static constexpr size_t num_args = sizeof...(Args);

        template <size_t i>
        struct arg{
            using type = typename std::tuple_element<i, std::tuple<Args...>>::type;
        };
    };

    template <typename Class , typename Ret, typename... Args >
    struct function_traits<Ret(Class::*)(Args...)>
    {
        using class_type = Class;
        using ret_type = Ret;
        using args_type = std::tuple<Args...>;
        static constexpr size_t num_args = sizeof...(Args);

        template <size_t i>
        struct arg{
            using type = typename std::tuple_element<i, std::tuple<Args...>>::type;
        };
    };

    template <typename Class , typename Ret, typename... Args >
    struct function_traits<Ret(Class::*)(Args...) const>
    {
        using class_type = Class;
        using ret_type = Ret;
        using args_type = std::tuple<Args...>;
        static constexpr size_t num_args = sizeof...(Args);

        template <size_t i>
        struct arg{
            using type = typename std::tuple_element<i, std::tuple<Args...>>::type;
        };
    };

    template<size_t xsize>
    struct string_literal{
        class size_sugar{
            static constexpr size_t value = xsize;
        public:
            constexpr size_t operator()() const noexcept { return value; }
            constexpr operator size_t() const noexcept { return value; }
        };
        

        char ptr[xsize + 1]; // left '/0' in the end
        static constexpr size_sugar size{};

        constexpr string_literal(const char(&cptr)[xsize + 1]) noexcept: ptr(){
            for(size_t i = 0; i < xsize + 1; i++) ptr[i] = cptr[i];
        }

        constexpr string_literal(const string_literal& other) noexcept: ptr(){
            for(size_t i = 0; i < xsize + 1; i++) ptr[i] = other.ptr[i];
        }

        template<size_t xsize2>
        constexpr string_literal<xsize + xsize2> operator+(const string_literal<xsize2>& other) const noexcept{
            char temp[xsize + xsize2 + 1] = {};
            char* p = temp;
            for(size_t i = 0; i < xsize; i++) *p++ = this->ptr[i];
            for(size_t i = 0; i < xsize2 + 1; i++) *p++ = other.ptr[i];
            return string_literal<xsize + xsize2>(temp);
        }

        template<size_t N>
        constexpr string_literal<xsize + N - 1> operator+(const char(&cptr)[N]) const noexcept{
            char temp[xsize + N] = {};
            char* p = temp;
            for(size_t i = 0; i < xsize; i++) *p++ = this->ptr[i];
            for(size_t i = 0; i < N; i++) *p++ = cptr[i];
            return string_literal<xsize + N - 1>(temp);
        }

        template<size_t xsize2>
        constexpr string_literal<xsize + xsize2> concat(const char* cptr) const noexcept{
            char temp[xsize + xsize2 + 1] = {};
            char* p = temp;
            for(size_t i = 0; i < xsize; i++) *p++ = this->ptr[i];
            for(size_t i = 0; i < xsize2; i++) *p++ = cptr[i];
            *p = '\0';
            return string_literal<xsize + xsize2>(temp);
        }

        constexpr const char& operator[](size_t idx) const noexcept{ return this->ptr[idx]; }

        template<class T>
        constexpr operator T() const noexcept{
            return T{static_cast<const char*>(ptr), xsize};
        }
    };

    template<size_t N>
    static constexpr string_literal<N - 1> make_literal(const char(&cptr)[N]) noexcept{
        return string_literal<N - 1>(std::forward<decltype(cptr)>(cptr));
    }

    template<class K, class V>
    struct MapPair{
        constexpr MapPair() noexcept: k(), v(){}
        constexpr MapPair(const MapPair& other) = default;
        constexpr MapPair(K k, V v) noexcept: k(k), v(v){
            static_assert(
                std::is_trivially_copyable<K>::value && std::is_trivially_copyable<V>::value,
                "MapPair::MapPair(K, V): key and value type should be trivial type."
                );
        }
        K k;
        V v;
    };

    template<size_t table_size, class K, class V>
    class StaticMapping{
    public:
        using key_type = K;
        using value_type = V;
        using pair_type = MapPair<K, V>;
    private:
        static constexpr size_t get_bstree_size(size_t tsize, int i, int j){
            int mid = ((j - i) / 2) + i;
            size_t lmax = tsize, rmax = tsize;
            if(i < mid) lmax = get_bstree_size(tsize * 2, i, mid);
            if(mid + 1 < j) rmax = get_bstree_size(tsize * 2 + 1, mid + 1, j);
            return lmax < rmax ? rmax : lmax;
        }

        constexpr void build_bstree(int ptr, const pair_type* sortedkeys, int i, int j){
            int mid = ((j - i) / 2) + i;
            this->table[ptr - 1] = sortedkeys[mid];
            if(i < mid) build_bstree(2 * ptr, sortedkeys, i, mid);
            if(mid + 1 < j) build_bstree(2 * ptr + 1, sortedkeys, mid + 1, j);
        }

        static constexpr int qsort_partition(pair_type* temp, int low, int high){
            pair_type pivot = temp[low];
            while(low < high){
                while(low < high && StaticMapping::compare(temp[high].k, pivot.k) >= 0){ --high; }
                temp[low] = temp[high];
                while(low < high && StaticMapping::compare(temp[low].k, pivot.k) <= 0){ ++low; }
                temp[high] = temp[low];
            }
            temp[low] = pivot;
            return low;
        }

        static constexpr void qsort(pair_type* temp, int low, int high){
            if(low < high){
                int pivot = qsort_partition(temp, low, high);
                qsort(temp, low, pivot - 1);
                qsort(temp, pivot + 1, high);
            }
        }

        template<size_t N>
        static constexpr void sortkey(pair_type* temp, const pair_type(&table)[N]){
            for(size_t i = 0; i < N; i++){ temp[i] = table[i]; }
            qsort(temp, 0, N - 1);
        }

        template<class T, std::enable_if_t<!std::is_arithmetic<std::decay_t<T>>::value, bool> = true>
        static constexpr int compare(const T& left, const T& right) noexcept{
            return left.compare(right);
        }
        template<class T, std::enable_if_t<std::is_arithmetic<std::decay_t<T>>::value, bool> = true>
        static constexpr int compare(const T left, const T right) noexcept{
            return (left > right) ? 1 : (left == right ? 0 : -1);
        }

    public:
        static constexpr size_t bstree_size = get_bstree_size(1, 0, table_size);
        pair_type table[bstree_size];

        constexpr StaticMapping(const pair_type(&table)[table_size]){
            static_assert(table_size <= bstree_size,
                "StaticMapping::StaticMapping(const pair_type&) bstree_size should be greater than table");
            pair_type temp[table_size] = {};
            StaticMapping::sortkey(temp, std::forward<decltype(table)>(table));
            this->build_bstree(1, temp, 0, table_size);
        }

        constexpr V find(const K& key) const noexcept{
            size_t pos = 1;
            while(pos <= bstree_size){
                const pair_type& sheet = table[pos-1];
                pos *= 2;
                int res = this->compare<K>(key, sheet.k);
                if(res > 0) pos += 1;
                else if(res == 0) return sheet.v;
            }
            return V();
        }

        template<class Map, Map map_func, class Reduce, Reduce reduce_func>
        class MapReduce{
            friend StaticMapping;

            template<class Ret, typename... Args>
            static Ret ret_traits(Ret(*)(Args...));
            struct map_t{
                template<class Ret, typename... Args>
                static std::tuple<Args...> args_traits(Ret(*)(const pair_type&, Args...));
                using ret_type = decltype(ret_traits(map_func));
                using arg_type = decltype(args_traits(map_func));
                static constexpr size_t num_args = std::tuple_size<arg_type>{};
            };
            struct reduce_t{
                template<class Ret, class MapRet, typename... Args>
                static std::tuple<Args...> args_traits(Ret(*)(std::initializer_list<MapRet>, Args...));
                using ret_type = decltype(ret_traits(reduce_func));
                using arg_type = decltype(args_traits(reduce_func));
                static constexpr size_t num_args = std::tuple_size<arg_type>{};
            };

            const pair_type* table;
            constexpr MapReduce(const pair_type* table) noexcept: table(table){}

            template<size_t... I, size_t... num_args1, size_t... num_args2>
            constexpr typename reduce_t::ret_type unpack(
                std::index_sequence<I...>, std::index_sequence<num_args1...>, typename map_t::arg_type&& map_arg,
                std::index_sequence<num_args2...>, typename reduce_t::arg_type&& reduce_arg
            ) const noexcept {
                return reduce_func(
                    {map_func(this->table[I], std::get<num_args1>(std::forward<typename map_t::arg_type>(map_arg))...)...}, 
                    std::get<num_args2>(std::forward<typename reduce_t::arg_type>(reduce_arg))...
                );
            }
        public:
            constexpr MapReduce() noexcept: table(nullptr) {}
            constexpr MapReduce(const MapReduce& other) noexcept: table(other.table){}

            constexpr typename reduce_t::ret_type operator()(
                typename map_t::arg_type&& map_arg, 
                typename reduce_t::arg_type&& reduce_arg
                )const noexcept{
                return unpack(
                    std::make_index_sequence<table_size>(), 
                    std::make_index_sequence<map_t::num_args>(), std::forward<typename map_t::arg_type>(map_arg),
                    std::make_index_sequence<reduce_t::num_args>(), std::forward<typename reduce_t::arg_type>(reduce_arg)
                );
            }
        };

        template<class Ret, typename... Args>
        using MapFunc = Ret(*)(const pair_type&, Args...);
        template<class Ret, class MapRet, typename...Args>
        using ReduceFunc = Ret(*)(std::initializer_list<MapRet>, Args...);

        template<class Map, Map map_func, class Reduce, Reduce reduce_func>
        constexpr MapReduce<Map, map_func, Reduce, reduce_func> map_reduce() const noexcept{
            return MapReduce<Map, map_func, Reduce, reduce_func>(this->table);
        }
    };

    template<class K, class V, size_t N>
    constexpr StaticMapping<N, K, V> make_staticmapping(const MapPair<K, V>(&table)[N]) noexcept{
        return StaticMapping<N, K, V>(std::forward<decltype(table)>(table));
    }

    // sample not directly used
    template<class Sender, class Receiver>
    struct Combinator{
        explicit constexpr Combinator() noexcept: sender(), receiver()
        {}

        template<class S, class R>
        explicit constexpr Combinator(S&& s, R&& r) 
        noexcept: sender(std::forward<S>(s)), receiver(std::forward<R>(r))
        {}
        constexpr Combinator(const Combinator& other)
        noexcept: sender(other.sender), receiver(other.receiver)
        {}

        template<class T, typename std::enable_if<std::is_same<T, void>::value, bool>::type = true, typename... Args>
        auto exec(Args&&... args) noexcept{
            sender(std::forward<Args>(args)...);
            return receiver();
        }
        template<class T, typename std::enable_if<std::is_same<T, void>::value, bool>::type = true, typename... Args>
        auto exec(Args&&... args) const noexcept{
            sender(std::forward<Args>(args)...);
            return receiver();
        }
        template<class T, typename std::enable_if<!std::is_same<T, void>::value, bool>::type = true, typename... Args>
        auto exec(Args&&... args) noexcept{
            return receiver(sender(std::forward<Args>(args)...));
        }
        template<class T, typename std::enable_if<!std::is_same<T, void>::value, bool>::type = true, typename... Args>
        auto exec(Args&&... args) const noexcept{
            return receiver(sender(std::forward<Args>(args)...));
        }

        template<typename... Args>
        auto operator()(Args&&... args) noexcept{
            using sender_ret_type = decltype(this->sender(std::forward<Args>(args)...));
            return exec<sender_ret_type>(std::forward<Args>(args)...);
        }
        template<typename... Args>
        constexpr auto operator()(Args&&... args) const noexcept{
            using sender_ret_type = decltype(this->sender(std::forward<Args>(args)...));
            return exec<sender_ret_type>(std::forward<Args>(args)...);
        }

        // Right fold
        template<class Then>
        constexpr Combinator<Sender, Combinator<Receiver, Then>> then(Then&& then_func) const noexcept {
            using ret_type = decltype(std::declval<decltype(then_func)>()(std::declval<decltype(*this)>()()));
            static_assert(always_true<ret_type>::value, "Sender ret type mismatch with Receiver args type");
            using Type = Combinator<Sender, Combinator<Receiver, Then>>;
            return Type(this->sender, Combinator<Receiver, Then>(this->receiver, std::forward<Then>(then_func)));
        }

        template<class Then>
        constexpr Combinator<Sender, Combinator<Receiver, Then>> operator|(Then&& then_func) const noexcept {
            return this->then(std::forward<Then>(then_func));
        }

    private:
        Sender sender;
        Receiver receiver;
    };

    template<class Sender, class Receiver>
    static constexpr Combinator<Sender, Receiver> make_combinator(Sender&& S, Receiver&& R) noexcept{
        return Combinator<Sender, Receiver>(std::forward<Sender>(S), std::forward<Receiver>(R));
    }
};

#endif