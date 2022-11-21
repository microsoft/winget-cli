
namespace xlang::meta::reader
{
    inline std::pair<std::string_view, std::string_view> get_type_namespace_and_name(coded_index<TypeDefOrRef> const& type)
    {
        if (type.type() == TypeDefOrRef::TypeDef)
        {
            auto const def = type.TypeDef();
            return { def.TypeNamespace(), def.TypeName() };
        }
        else if (type.type() == TypeDefOrRef::TypeRef)
        {
            auto const ref = type.TypeRef();
            return { ref.TypeNamespace(), ref.TypeName() };
        }
        else
        {
            XLANG_ASSERT(false);
            return {};
        }
    }

    inline std::pair<std::string_view, std::string_view> get_base_class_namespace_and_name(TypeDef const& type)
    {
        return get_type_namespace_and_name(type.Extends());
    }

    inline auto extends_type(TypeDef type, std::string_view typeNamespace, std::string_view typeName)
    {
        return get_base_class_namespace_and_name(type) == std::pair(typeNamespace, typeName);
    }

    enum class category
    {
        interface_type,
        class_type,
        enum_type,
        struct_type,
        delegate_type
    };

    inline category get_category(TypeDef const& type)
    {
        if (type.Flags().Semantics() == TypeSemantics::Interface)
        {
            return category::interface_type;
        }

        auto const& [extends_namespace, extends_name] = get_base_class_namespace_and_name(type);

        if (extends_name == "Enum"sv && extends_namespace == "System"sv)
        {
            return category::enum_type;
        }

        if (extends_name == "ValueType"sv && extends_namespace == "System"sv)
        {
            return category::struct_type;
        }

        if (extends_name == "MulticastDelegate"sv && extends_namespace == "System"sv)
        {
            return category::delegate_type;
        }

        return category::class_type;
    }
}
