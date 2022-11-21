
namespace xlang::experimental::reflect
{
    template <typename T>
    struct base_type
    {
        using type = Windows::Foundation::IXlangObject;
    };

    template <typename T>
    using base_type_t = typename base_type<T>::type;

    template <typename T>
    struct named_property {};

    template <typename T>
    struct properties
    {
        using type = impl::typelist<>;
    };

    template <typename T>
    using properties_t = typename properties<T>::type;

    template <typename T, typename Func>
    static constexpr inline auto for_each_property(Func&& func)
    {
        return impl::for_each<properties_t<T>>::apply(std::forward<Func>(func));
    }

    template <typename T, typename Func>
    static constexpr inline bool find_property_if(Func&& func)
    {
        return impl::find_if<properties_t<T>>::apply(std::forward<Func>(func));
    }

    template <typename MetaProperty>
    using property_name = typename MetaProperty::name;

    template <typename MetaProperty>
    inline constexpr std::wstring_view property_name_v = property_name<MetaProperty>::value;

    template <typename MetaProperty>
    using is_property_readable = typename MetaProperty::is_readable;

    template <typename MetaProperty>
    inline constexpr bool is_property_readable_v = is_property_readable<MetaProperty>::value;

    template <typename MetaProperty>
    using is_property_writable = typename MetaProperty::is_writable;

    template <typename MetaProperty>
    inline constexpr bool is_property_writable_v = is_property_writable<MetaProperty>::value;

    template <typename MetaProperty>
    using is_property_static = typename MetaProperty::is_static;

    template <typename MetaProperty>
    inline constexpr bool is_property_static_v = is_property_static<MetaProperty>::value;

    template <typename MetaProperty>
    struct property_value
    {
        using type = typename MetaProperty::property_type;
    };

    template <typename MetaProperty>
    using property_value_t = typename property_value<MetaProperty>::type;

    template <typename MetaProperty>
    struct property_target
    {
        using type = typename MetaProperty::target_type;
    };

    template <typename MetaProperty>
    using property_target_t = typename property_target<MetaProperty>::type;

    template <typename MetaProperty>
    using property_getter = typename MetaProperty::getter;

    template <typename MetaProperty>
    using property_setter = typename MetaProperty::setter;

    template <typename T>
    struct get_enumerator_names
    {
        static_assert(impl::has_category_v<T> && std::is_enum_v<T>, "T must be a xlang enum type");
    };

    template <typename T>
    struct get_enumerator_values
    {
        static_assert(impl::has_category_v<T> && std::is_enum_v<T>, "T must be a xlang enum type");
    };
}
