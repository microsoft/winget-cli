#pragma once

namespace xlang
{
    using namespace std::filesystem;
    using namespace text;
    using namespace meta::reader;

    struct type_name
    {
        std::string_view name;
        std::string_view name_space;

        explicit type_name(TypeDef const& type) :
            name(type.TypeName()),
            name_space(type.TypeNamespace())
        {
        }

        explicit type_name(TypeRef const& type) :
            name(type.TypeName()),
            name_space(type.TypeNamespace())
        {
        }
    };

    bool operator==(type_name const& left, std::string_view const& right)
    {
        if (left.name.size() + 1 + left.name_space.size() != right.size())
        {
            return false;
        }

        if (right[left.name_space.size()] != '.')
        {
            return false;
        }

        if (0 != right.compare(left.name_space.size() + 1, left.name.size(), left.name))
        {
            return false;
        }

        return 0 == right.compare(0, left.name_space.size(), left.name_space);
    }

    static auto remove_tick(std::string_view const& name)
    {
        return name.substr(0, name.rfind('`'));
    }

    template <typename First, typename...Rest>
    auto get_impl_name(First const& first, Rest const&... rest)
    {
        std::string result;

        auto convert = [&](auto&& value)
        {
            for (auto&& c : value)
            {
                result += c == '.' ? '_' : c;
            }
        };

        convert(first);
        ((result += '_', convert(rest)), ...);
        return result;
    }

    struct writer : writer_base<writer>
    {
        using writer_base<writer>::write;

        std::string type_namespace;
        bool abi_types{};
        bool param_names{};
        bool consume_types{};
        bool async_types{};
        std::map<std::string_view, std::set<TypeDef>> depends;
        std::vector<std::vector<std::string>> generic_param_stack;

        struct generic_param_guard
        {
            explicit generic_param_guard(writer* arg = nullptr)
                : owner(arg)
            {}

            ~generic_param_guard()
            {
                if (owner)
                {
                    owner->generic_param_stack.pop_back();
                }
            }

            generic_param_guard(generic_param_guard&& other)
                : owner(other.owner)
            {
                owner = nullptr;
            }

            generic_param_guard& operator=(generic_param_guard&& other)
            {
                owner = std::exchange(other.owner, nullptr);
                return *this;
            }

            generic_param_guard& operator=(generic_param_guard const&) = delete;
            writer* owner;
        };

        void add_depends(TypeDef const& type)
        {
            auto ns = type.TypeNamespace();

            if (ns != type_namespace)
            {
                depends[ns].insert(type);
            }
        }

        [[nodiscard]] auto push_generic_params(std::pair<GenericParam, GenericParam> const& params)
        {
            if (empty(params))
            {
                return generic_param_guard{ nullptr };
            }

            std::vector<std::string> names;

            for (auto&& param : params)
            {
                names.push_back(std::string{ param.Name() });
            }

            generic_param_stack.push_back(std::move(names));
            return generic_param_guard{ this };
        }

        [[nodiscard]] auto push_generic_params(GenericTypeInstSig const& signature)
        {
            std::vector<std::string> names;

            for (auto&& arg : signature.GenericArgs())
            {
                names.push_back(write_temp("%", arg));
            }

            generic_param_stack.push_back(std::move(names));
            return generic_param_guard{ this };
        }

        void write_value(int32_t value)
        {
            write_printf("%d", value);
        }

        void write_value(uint32_t value)
        {
            write_printf("%#0x", value);
        }

        void write_code(std::string_view const& value)
        {
            for (auto&& c : value)
            {
                if (c == '.')
                {
                    write("::");
                }
                else if (c == '`')
                {
                    return;
                }
                else
                {
                    write(c);
                }
            }
        }

        void write(Constant const& value)
        {
            switch (value.Type())
            {
            case ConstantType::Int32:
                write_value(value.ValueInt32());
                break;
            case ConstantType::UInt32:
                write_value(value.ValueUInt32());
                break;
            default:
                throw_invalid("Unexpected constant type");
            }
        }

        void write(TypeDef const& type)
        {
            add_depends(type);
            auto ns = type.TypeNamespace();
            auto name = type.TypeName();
            auto generics = type.GenericParam();

            if (!empty(generics))
            {
                write("@::%<%>", ns, remove_tick(name), bind_list(", ", generics));
                return;
            }

            // TODO: get rid of all these renames once parity with cppxlang.exe has been reached...

            if (name == "EventRegistrationToken" && ns == "Foundation")
            {
                write("xlang::event_token");
            }
            else if (abi_types)
            {
                auto category = get_category(type);

                if (category == category::struct_type)
                {
                    if ((name == "DateTime" || name == "TimeSpan") && ns == "Foundation")
                    {
                        write("int64_t");
                    }
                    else
                    {
                        write("struct struct_%_%", get_impl_name(ns), name);
                    }
                }
                else if (category == category::enum_type)
                {
                    write(type.FieldList().first.Signature().Type());
                }
                else
                {
                    write("void*");
                }
            }
            else
            {
                write("@::%", ns, name);
            }
        }

        void write(TypeRef const& type)
        {
            if (type_name(type) == "System.Guid")
            {
                write("xlang::guid");
            }
            else
            {
                write(find_required(type));
            }
        }

        void write(GenericParam const& param)
        {
            write(param.Name());
        }

        void write(coded_index<TypeDefOrRef> const& type)
        {
            switch (type.type())
            {
            case TypeDefOrRef::TypeDef:
                write(type.TypeDef());
                break;
            case TypeDefOrRef::TypeRef:
                write(type.TypeRef());
                break;
            case TypeDefOrRef::TypeSpec:
                write(type.TypeSpec().Signature().GenericTypeInst());
                break;
            }
        }

        void write(GenericTypeInstSig const& type)
        {
            if (abi_types)
            {
                write("void*");
            }
            else
            {
                auto generic_type = type.GenericType();
                auto[ns, name] = get_type_namespace_and_name(generic_type);
                name.remove_suffix(name.size() - name.rfind('`'));
                add_depends(find_required(generic_type));

                if (consume_types)
                {
                    static constexpr std::string_view iterable("Foundation::Collections::IIterable<"sv);
                    static constexpr std::string_view vector_view("Foundation::Collections::IVectorView<"sv);
                    static constexpr std::string_view map_view("Foundation::Collections::IMapView<"sv);
                    static constexpr std::string_view vector("Foundation::Collections::IVector<"sv);
                    static constexpr std::string_view map("Foundation::Collections::IMap<"sv);

                    consume_types = false;
                    auto full_name = write_temp("@::%<%>", ns, name, bind_list(", ", type.GenericArgs()));
                    consume_types = true;

                    if (starts_with(full_name, iterable))
                    {
                        if (async_types)
                        {
                            write("param::async_iterable%", full_name.substr(iterable.size() - 1));
                        }
                        else
                        {
                            write("param::iterable%", full_name.substr(iterable.size() - 1));
                        }
                    }
                    else if (starts_with(full_name, vector_view))
                    {
                        if (async_types)
                        {
                            write("param::async_vector_view%", full_name.substr(vector_view.size() - 1));
                        }
                        else
                        {
                            write("param::vector_view%", full_name.substr(vector_view.size() - 1));
                        }
                    }

                    else if (starts_with(full_name, map_view))
                    {
                        if (async_types)
                        {
                            write("param::async_map_view%", full_name.substr(map_view.size() - 1));
                        }
                        else
                        {
                            write("param::map_view%", full_name.substr(map_view.size() - 1));
                        }
                    }
                    else if (starts_with(full_name, vector))
                    {
                        write("param::vector%", full_name.substr(vector.size() - 1));
                    }
                    else if (starts_with(full_name, map))
                    {
                        write("param::map%", full_name.substr(map.size() - 1));
                    }
                    else
                    {
                        write(full_name);
                    }
                }
                else
                {
                    write("@::%<%>", ns, name, bind_list(", ", type.GenericArgs()));
                }
            }
        }

        void write(TypeSig::value_type const& type)
        {
            call(type,
                [&](ElementType type)
                {
                    if (type == ElementType::Boolean) { write("bool"); }
                    else if (type == ElementType::Char) { write("char16_t"); }
                    else if (type == ElementType::I1) { write("int8_t"); }
                    else if (type == ElementType::U1) { write("uint8_t"); }
                    else if (type == ElementType::I2) { write("int16_t"); }
                    else if (type == ElementType::U2) { write("uint16_t"); }
                    else if (type == ElementType::I4) { write("int32_t"); }
                    else if (type == ElementType::U4) { write("uint32_t"); }
                    else if (type == ElementType::I8) { write("int64_t"); }
                    else if (type == ElementType::U8) { write("uint64_t"); }
                    else if (type == ElementType::R4) { write("float"); }
                    else if (type == ElementType::R8) { write("double"); }
                    else if (type == ElementType::String)
                    {
                        if (abi_types)
                        {
                            write("void*");
                        }
                        else if (consume_types)
                        {
                            write("param::hstring");
                        }
                        else
                        {
                            write("hstring");
                        }
                    }
                    else if (type == ElementType::Object)
                    {
                        if (abi_types)
                        {
                            write("void*");
                        }
                        else
                        {
                            write("Windows::Foundation::IXlangObject");
                        }
                    }
                    else
                    {
                        XLANG_ASSERT(false);
                    }
                },
                [&](GenericTypeIndex var)
                {
                    write(generic_param_stack.back()[var.index]);
                },
                    [&](auto&& type)
                {
                    write(type);
                });
        }

        void write(TypeSig const& signature)
        {
            if (!abi_types && signature.is_szarray())
            {
                write("com_array<%>", signature.Type());
            }
            else
            {
                write(signature.Type());
            }
        }

        void write(RetTypeSig const& value)
        {
            if (value)
            {
                write(value.Type());
            }
            else
            {
                write("void");
            }
        }

        void write(Field const& value)
        {
            write(value.Signature().Type());
        }

        void write_root_include(std::string_view const& include)
        {
            auto format = R"(#include %xlang/%.h%
)";

            write(format,
                settings.brackets ? '<' : '\"',
                include,
                settings.brackets ? '>' : '\"');
        }

        void write_depends(std::string_view const& ns, char impl = 0)
        {
            if (impl)
            {
                write_root_include(write_temp("impl/%.%", ns, impl));
            }
            else
            {
                write_root_include(ns);
            }
        }

        void save_header(char impl = 0)
        {
            auto filename{ settings.output_folder + "xlang/" };

            if (impl)
            {
                filename += "impl/";
            }

            filename += type_namespace;

            if (impl)
            {
                filename += '.';
                filename += impl;
            }

            filename += ".h";
            flush_to_file(filename);
        }
    };
}
