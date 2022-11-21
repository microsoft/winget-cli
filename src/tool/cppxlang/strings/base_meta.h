
namespace xlang
{
    void check_com_interop_error(com_interop_result const result);
    void check_xlang_error(xlang_error_info* result);

    template <typename D, typename I>
    D* get_self(I const& from) noexcept;

    struct take_ownership_from_abi_t {};
    inline constexpr take_ownership_from_abi_t take_ownership_from_abi{};

    template <typename T>
    struct com_ptr;

    namespace param
    {
        template <typename T>
        struct iterable;

        template <typename T>
        struct async_iterable;

        template <typename K, typename V>
        struct map_view;

        template <typename K, typename V>
        struct async_map_view;

        template <typename K, typename V>
        struct map;

        template <typename T>
        struct vector_view;

        template <typename T>
        struct async_vector_view;

        template <typename T>
        struct vector;
    }
}

namespace xlang::impl
{
    using namespace std::literals;

    template <typename Async>
    void blocking_suspend(Async const& async);

    template <typename T>
    struct reference_traits;

    template <typename T>
    struct identity
    {
        using type = T;
    };

    template <typename T, typename Enable = void>
    struct abi
    {
        using type = T;
    };

    template <typename T>
    struct abi<T, std::enable_if_t<std::is_enum_v<T>>>
    {
        using type = std::underlying_type_t<T>;
    };

    template <typename T>
    using abi_t = typename abi<T>::type;

    template <typename T>
    struct consume;

    template <typename D, typename I = D>
    using consume_t = typename consume<I>::template type<D>;

    template <typename T>
    struct delegate;

    template <typename T, typename H>
    using delegate_t = typename delegate<T>::template type<H>;

    template <typename T, typename = std::void_t<>>
    struct default_interface
    {
        using type = T;
    };

    struct basic_category;
    struct interface_category;
    struct delegate_category;
    struct enum_category;
    struct class_category;

    template <typename T>
    struct category
    {
        using type = void;
    };

    template <typename T>
    using category_t = typename category<T>::type;

    template <typename T>
    inline constexpr bool has_category_v = !std::is_same_v<category_t<T>, void>;

    template <typename... Args>
    struct pinterface_category;

    template <typename... Fields>
    struct struct_category;

    template <typename Category, typename T>
    struct category_signature;

    template <typename T>
    struct signature
    {
        static constexpr auto data{ category_signature<typename category<T>::type, T>::data };
    };

    template <typename T>
    struct missing_guid_of
    {
        static constexpr bool value{};
    };

    template <typename T>
    struct missing_guid
    {
        static_assert(missing_guid_of<T>::value, "Support for non-xlang interfaces is disabled. To enable, simply #include <unknwn.h> before any cppxlang headers.");
    };

#ifdef XLANG_WINDOWS_ABI
    template <typename T>
    struct guid_storage
    {
        static constexpr guid value{ __uuidof(T) };
    };
#else
    template <typename T>
    struct guid_storage : missing_guid<T> {};
#endif

    template <typename T>
    struct is_enum_flag : std::false_type {};

    template <typename T>
    inline constexpr bool is_enum_flag_v = is_enum_flag<T>::value;

    template <typename T>
    constexpr auto to_underlying_type(T const value) noexcept
    {
        return static_cast<std::underlying_type_t<T>>(value);
    }

    template <typename, typename = std::void_t<>>
    struct is_implements : std::false_type {};

    template <typename T>
    struct is_implements<T, std::void_t<typename T::implements_type>> : std::true_type {};

    template <typename T>
    inline constexpr bool is_implements_v = is_implements<T>::value;

    template <typename D, typename I>
    struct require_one : consume_t<D, I>
    {
        operator I() const noexcept
        {
            return static_cast<D const*>(this)->template try_as<I>();
        }
    };

    template <typename D, typename... I>
    struct XLANG_EBO require : require_one<D, I>...
    {};

    template <typename D, typename I>
    struct base_one
    {
        operator I() const noexcept
        {
            return static_cast<D const*>(this)->template try_as<I>();
        }
    };

    template <typename D, typename... I>
    struct XLANG_EBO base : base_one<D, I>...
    {};

    template <typename T>
    T empty_value() noexcept
    {
        if constexpr (std::is_base_of_v<Windows::Foundation::IUnknown, T>)
        {
            return nullptr;
        }
        else
        {
            return {};
        }
    }

    template <typename T, typename Enable = void>
    struct arg
    {
        using in = abi_t<T>;
    };

    template <typename T>
    struct arg<T, std::enable_if_t<std::is_base_of_v<Windows::Foundation::IUnknown, T>>>
    {
        using in = void*;
    };

    template <typename T>
    using arg_in = typename arg<T>::in;

    template <typename T>
    using arg_out = arg_in<T>*;

    template <typename D, typename I, typename Enable = void>
    struct produce_base;

    template <typename D, typename I>
    struct produce : produce_base<D, I>
    {
    };

    template <typename T>
    struct wrapped_type
    {
        using type = T;
    };

    template <typename T>
    struct wrapped_type<com_ptr<T>>
    {
        using type = T;
    };

    template <typename T>
    using wrapped_type_t = typename wrapped_type<T>::type;

    template <template <typename...> typename Trait, typename Enabler, typename... Args>
    struct is_detected : std::false_type {};

    template <template <typename...> typename Trait, typename... Args>
    struct is_detected<Trait, std::void_t<Trait<Args...>>, Args...> : std::true_type {};

    template <template <typename...> typename Trait, typename... Args>
    inline constexpr bool is_detected_v = std::is_same_v<typename is_detected<Trait, void, Args...>::type, std::true_type>;

    template <typename ... Types>
    struct typelist {};

    template <typename ... Lists>
    struct typelist_concat;

    template <>
    struct typelist_concat<> { using type = xlang::impl::typelist<>; };

    template <typename ... List>
    struct typelist_concat<xlang::impl::typelist<List...>> { using type = xlang::impl::typelist<List...>; };

    template <typename ... List1, typename ... List2, typename ... Rest>
    struct typelist_concat<xlang::impl::typelist<List1...>, xlang::impl::typelist<List2...>, Rest...>
        : typelist_concat<xlang::impl::typelist<List1..., List2...>, Rest...>
    {};

    template <typename T>
    struct for_each;

    template <typename ... Types>
    struct for_each<typelist<Types...>>
    {
        template <typename Func>
        static auto apply([[maybe_unused]] Func&& func)
        {
            return (func(Types{}), ...);
        }
    };

    template <typename T>
    struct find_if;

    template <typename ... Types>
    struct find_if<typelist<Types...>>
    {
        template <typename Func>
        static bool apply([[maybe_unused]] Func&& func)
        {
            return (func(Types{}) || ...);
        }
    };
}

template <typename T>
constexpr auto operator|(T const left, T const right) noexcept -> std::enable_if_t<xlang::impl::is_enum_flag_v<T>, T>
{
    return static_cast<T>(xlang::impl::to_underlying_type(left) | xlang::impl::to_underlying_type(right));
}

template <typename T>
constexpr auto operator|=(T& left, T const right) noexcept -> std::enable_if_t<xlang::impl::is_enum_flag_v<T>, T>
{
    left = left | right;
    return left;
}

template <typename T>
constexpr auto operator&(T const left, T const right) noexcept -> std::enable_if_t<xlang::impl::is_enum_flag_v<T>, T>
{
    return static_cast<T>(xlang::impl::to_underlying_type(left) & xlang::impl::to_underlying_type(right));
}

template <typename T>
constexpr auto operator&=(T& left, T const right) noexcept -> std::enable_if_t<xlang::impl::is_enum_flag_v<T>, T>
{
    left = left & right;
    return left;
}

template <typename T>
constexpr auto operator~(T const value) noexcept -> std::enable_if_t<xlang::impl::is_enum_flag_v<T>, T>
{
    return static_cast<T>(~xlang::impl::to_underlying_type(value));
}

template <typename T>
constexpr auto operator^(T const left, T const right) noexcept -> std::enable_if_t<xlang::impl::is_enum_flag_v<T>, T>
{
    return static_cast<T>(xlang::impl::to_underlying_type(left) ^ xlang::impl::to_underlying_type(right));
}

template <typename T>
constexpr auto operator^=(T& left, T const right) noexcept -> std::enable_if_t<xlang::impl::is_enum_flag_v<T>, T>
{
    left = left ^ right;
    return left;
}
