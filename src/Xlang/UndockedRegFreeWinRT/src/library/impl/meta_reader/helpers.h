
namespace xlang::meta::reader
{
    template <typename T>
    bool empty(std::pair<T, T> const& range) noexcept
    {
        return range.first == range.second;
    }

    template <typename T>
    std::size_t size(std::pair<T, T> const& range) noexcept
    {
        return range.second - range.first;
    }

    inline auto find(TypeRef const& type)
    {
        return type.get_database().get_cache().find(type.TypeNamespace(), type.TypeName());
    }

    inline auto find_required(TypeRef const& type)
    {
        return type.get_database().get_cache().find_required(type.TypeNamespace(), type.TypeName());
    }

    inline TypeDef find_required(coded_index<TypeDefOrRef> const& type)
    {
        if (type.type() == TypeDefOrRef::TypeRef)
        {
            return find_required(type.TypeRef());
        }
        else if (type.type() == TypeDefOrRef::TypeDef)
        {
            return type.TypeDef();
        }
        else
        {
            XLANG_ASSERT(false);
            return {};
        }
    }

    inline bool is_const(ParamSig const& param)
    {
        auto is_type_const = [](auto&& type)
        {
            return type.TypeNamespace() == "System.Runtime.CompilerServices" && type.TypeName() == "IsConst";
        };

        for (auto const& cmod : param.CustomMod())
        {
            auto type = cmod.Type();

            if (type.type() == TypeDefOrRef::TypeDef)
            {
                if (is_type_const(type.TypeDef()))
                {
                    return true;
                }
            }
            else if (type.type() == TypeDefOrRef::TypeRef)
            {
                if (is_type_const(type.TypeRef()))
                {
                    return true;
                }
            }
        }

        return false;
    };
}
