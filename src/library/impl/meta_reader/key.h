
namespace xlang::meta::reader
{
    template <typename T>
    template <typename Row>
    Row index_base<T>::get_row() const
    {
        XLANG_ASSERT(type() == (index_tag_v<T, Row>));
        return get_database().template get_table<Row>()[index()];
    }

    inline auto typed_index<CustomAttributeType>::MemberRef() const
    {
        return get_row<reader::MemberRef>();
    }

    inline auto typed_index<CustomAttributeType>::MethodDef() const
    {
        return get_row<reader::MethodDef>();
    }

    inline auto typed_index<HasConstant>::Field() const
    {
        return get_row<reader::Field>();
    }

    inline auto typed_index<HasConstant>::Param() const
    {
        return get_row<reader::Param>();
    }

    inline auto typed_index<HasConstant>::Property() const
    {
        return get_row<reader::Property>();
    }

    inline auto typed_index<HasSemantics>::Property() const
    {
        return get_row<reader::Property>();
    }

    inline auto typed_index<HasSemantics>::Event() const
    {
        return get_row<reader::Event>();
    }

    inline auto typed_index<MethodDefOrRef>::MethodDef() const
    {
        return get_row<reader::MethodDef>();
    }

    inline auto typed_index<MethodDefOrRef>::MemberRef() const
    {
        return get_row<reader::MemberRef>();
    }

    inline auto typed_index<ResolutionScope>::Module() const
    {
        return get_row<reader::Module>();
    }

    inline auto typed_index<ResolutionScope>::ModuleRef() const
    {
        return get_row<reader::ModuleRef>();
    }

    inline auto typed_index<ResolutionScope>::AssemblyRef() const
    {
        return get_row<reader::AssemblyRef>();
    }

    inline auto typed_index<ResolutionScope>::TypeRef() const
    {
        return get_row<reader::TypeRef>();
    }

    inline TypeDef typed_index<TypeDefOrRef>::TypeDef() const
    {
        return get_row<reader::TypeDef>();
    }

    inline TypeRef typed_index<TypeDefOrRef>::TypeRef() const
    {
        return get_row<reader::TypeRef>();
    }

    inline TypeSpec typed_index<TypeDefOrRef>::TypeSpec() const
    {
        return get_row<reader::TypeSpec>();
    }

    inline auto typed_index<TypeDefOrRef>::CustomAttribute() const
    {
        if (type() == TypeDefOrRef::TypeDef)
        {
            return TypeDef().CustomAttribute();
        }

        if (type() == TypeDefOrRef::TypeRef)
        {
            return TypeRef().CustomAttribute();
        }

        return TypeSpec().CustomAttribute();
    }

    inline auto typed_index<MemberRefParent>::TypeRef() const
    {
        return get_row<reader::TypeRef>();
    }

    inline auto typed_index<MemberRefParent>::TypeDef() const
    {
        return get_row<reader::TypeDef>();
    }

    inline bool TypeDef::is_enum() const
    {
        return extends_type(*this, "System"sv, "Enum"sv);
    }

    struct EnumDefinition
    {
        explicit EnumDefinition(TypeDef const& type)
            : m_typedef(type)
        {
            XLANG_ASSERT(type.is_enum());
            for (auto field : type.FieldList())
            {
                if (!field.Flags().Literal() && !field.Flags().Static())
                {
                    XLANG_ASSERT(m_underlying_type == ElementType::End);
                    m_underlying_type = std::get<ElementType>(field.Signature().Type().Type());
                    XLANG_ASSERT(ElementType::Boolean <= m_underlying_type && m_underlying_type <= ElementType::U8);
                }
            }
        }

        auto get_enumerator(std::string_view const& name) const
        {
            auto fields = m_typedef.FieldList();

            auto field = std::find_if(begin(fields), end(fields), [&](auto&& field)
            {
                return field.Name() == name;
            });

            XLANG_ASSERT(field != end(fields));
            return field;
        }

        TypeDef m_typedef;
        ElementType m_underlying_type{};
    
    };

    inline auto TypeDef::get_enum_definition() const
    {
        return EnumDefinition{ *this };
    }

    template <typename T>
    CustomAttribute get_attribute(T const& row, std::string_view const& type_namespace, std::string_view const& type_name)
    {
        for (auto&& attribute : row.CustomAttribute())
        {
            auto pair = attribute.TypeNamespaceAndName();

            if (pair.first == type_namespace && pair.second == type_name)
            {
                return attribute;
            }
        }

        return {};
    }
}
