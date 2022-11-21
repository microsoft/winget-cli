#pragma once

namespace pywinrt
{
    using namespace std::literals;
    using namespace std::filesystem;
    using namespace xlang;
    using namespace xlang::meta::reader;
    using namespace xlang::text;

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

    struct writer : indented_writer_base<writer>
    {
        using indented_writer_base<writer>::write;

        std::string_view current_namespace{};
        std::set<std::string> needed_namespaces{};

#pragma region generic param handling
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

            generic_param_guard(generic_param_guard && other)
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

        [[nodiscard]] auto push_generic_params(std::pair<GenericParam, GenericParam>&& params)
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

        [[nodiscard]] auto push_generic_params(std::vector<type_semantics> const& type_arguments)
        {
            if (type_arguments.size() == 0)
            {
                return generic_param_guard{ nullptr };
            }

            std::vector<std::string> names;

            for (auto&& arg : type_arguments)
            {
                // TODO real code here
                names.push_back(write_temp("%", arg));
            }

            generic_param_stack.push_back(std::move(names));
            return generic_param_guard{ this };
        }

#pragma endregion

        void write_value(bool value)
        {
            write(value ? "TRUE"sv : "FALSE"sv);
        }

        void write_value(char16_t value)
        {
            write_printf("%#0hx", value);
        }

        void write_value(int8_t value)
        {
            write_printf("%hhd", value);
        }

        void write_value(uint8_t value)
        {
            write_printf("%#0hhx", value);
        }

        void write_value(int16_t value)
        {
            write_printf("%hd", value);
        }

        void write_value(uint16_t value)
        {
            write_printf("%#0hx", value);
        }

        void write_value(int32_t value)
        {
            write_printf("%d", value);
        }

        void write_value(uint32_t value)
        {
            write_printf("%#0x", value);
        }

        void write_value(int64_t value)
        {
            write_printf("%lld", value);
        }

        void write_value(uint64_t value)
        {
            write_printf("%#0llx", value);
        }

        void write_value(float value)
        {
            write_printf("%f", value);
        }

        void write_value(double value)
        {
            write_printf("%f", value);
        }

        void write_value(std::string_view value)
        {
            write("\"%\"", value);
        }

        void write(Constant const& value)
        {
            switch (value.Type())
            {
            case ConstantType::Boolean:
                write_value(value.ValueBoolean());
                break;
            case ConstantType::Char:
                write_value(value.ValueChar());
                break;
            case ConstantType::Int8:
                write_value(value.ValueInt8());
                break;
            case ConstantType::UInt8:
                write_value(value.ValueUInt8());
                break;
            case ConstantType::Int16:
                write_value(value.ValueInt16());
                break;
            case ConstantType::UInt16:
                write_value(value.ValueUInt16());
                break;
            case ConstantType::Int32:
                write_value(value.ValueInt32());
                break;
            case ConstantType::UInt32:
                write_value(value.ValueUInt32());
                break;
            case ConstantType::Int64:
                write_value(value.ValueInt64());
                break;
            case ConstantType::UInt64:
                write_value(value.ValueUInt64());
                break;
            case ConstantType::Float32:
                write_value(value.ValueFloat32());
                break;
            case ConstantType::Float64:
                write_value(value.ValueFloat64());
                break;
            case ConstantType::String:
                write_value(value.ValueString());
                break;
            case ConstantType::Class:
                write("null");
                break;
            }
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

        void write(TypeRef const& type)
        {
            auto ns = type.TypeNamespace();
            auto name = type.TypeName();

            if (name == "Guid" && ns == "System")
            {
                write("winrt::guid");
            }
            else
            {
                write(find_required(type));
            }
        }

        void register_type_namespace(std::string_view ns)
        {
            if (ns != current_namespace && ns != "System")
            {
                needed_namespaces.emplace(ns);
            }
        }

        void register_type_namespace(GenericTypeInstSig const& t)
        {
            register_type_namespace(t.GenericType());
            for(auto&& type_arg : t.GenericArgs())
            {
                register_type_namespace(type_arg);
            }
        }

        void register_type_namespace(coded_index<TypeDefOrRef> const& type)
        {
            switch (type.type())
            {
            case TypeDefOrRef::TypeDef:
                register_type_namespace(type.TypeDef().TypeNamespace());
                break;

            case TypeDefOrRef::TypeRef:
            {
                auto tr = type.TypeRef();
                if (tr.TypeName() != "Guid" || tr.TypeNamespace() != "System" )
                {
                    register_type_namespace(type.TypeRef().TypeNamespace());
                }
            }
                break;

            case TypeDefOrRef::TypeSpec:
                register_type_namespace(type.TypeSpec().Signature().GenericTypeInst());
                break;
            }
        }

        void register_type_namespace(TypeSig const& type)
        {
            call(type.Type(),
                [&](ElementType) {},
                [&](GenericTypeIndex) {},
                [&](GenericMethodTypeIndex) { throw_invalid("Generic methods not supported"); },
                [&](auto&& t) { register_type_namespace(t); });
        }

        void write(TypeDef const& type)
        {
            auto ns = type.TypeNamespace();
            auto name = type.TypeName();

            if (ns != current_namespace)
            {
                needed_namespaces.emplace(ns);
            }

            if ((ns == "Windows.Foundation") && (name == "HResult"))
            {
                write("winrt::hresult");
            }
            else if ((ns == "Windows.Foundation") && (name == "EventRegistrationToken"))
            {
                write("winrt::event_token");
            }
            else
            {
                if (ns == "Windows.Foundation.Numerics")
                {
                    static const std::map<std::string_view, std::string_view> custom_numerics = {
                        { "Matrix3x2", "float3x2" },
                        { "Matrix4x4", "float4x4" },
                        { "Plane", "plane" },
                        { "Quaternion", "quaternion" },
                        { "Vector2", "float2"},
                        { "Vector3", "float3" },
                        { "Vector4", "float4" }
                    };

                    auto custom_numeric = custom_numerics.find(name);
                    if (custom_numeric != custom_numerics.end())
                    {
                        name = custom_numeric->second;
                    }
                }

                write("winrt::@::@", ns, name);
            }
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
            write("%<%>", type.GenericType(), bind_list(", ", type.GenericArgs()));
        }

        static std::string_view get_cpp_type(ElementType type)
        {
            switch (type)
            {
            case ElementType::Boolean:
                return "bool";
            case ElementType::Char:
                return "char16_t";
            case ElementType::I1:
                return "int8_t";
            case ElementType::U1:
                return "uint8_t";
            case ElementType::I2:
                return "int16_t";
            case ElementType::U2:
                return "uint16_t";
            case ElementType::I4:
                return "int32_t";
            case ElementType::U4:
                return "uint32_t";
            case ElementType::I8:
                return "int64_t";
            case ElementType::U8:
                return "uint64_t";
            case ElementType::R4:
                return "float";
            case ElementType::R8:
                return "double";
            case ElementType::String:
                return "winrt::hstring";
            case ElementType::Object:
                return "winrt::Windows::Foundation::IInspectable";
            default:
                throw_invalid("element type not supported");
            }
        }

        void write(ElementType type)
        {
            write(writer::get_cpp_type(type));
        }

        void write(GenericTypeIndex var)
        {
            write(generic_param_stack.back()[var.index]);
        }

        void write(GenericMethodTypeIndex)
        {
            throw_invalid("Generic methods not supported");
        }

        void write(TypeSig const& signature)
        {
            call(signature.Type(),
                [&](auto&& type)
            {
                write(type);
            });
        }

        void write(fundamental_type type)
        {
            switch (type)
            {
            case fundamental_type::Boolean:
                write(ElementType::Boolean);
                break;
            case fundamental_type::Char:
                write(ElementType::Char);
                break;
            case fundamental_type::Int8:
                write(ElementType::I1);
                break;
            case fundamental_type::UInt8:
                write(ElementType::U1);
                break;
            case fundamental_type::Int16:
                write(ElementType::I2);
                break;
            case fundamental_type::UInt16:
                write(ElementType::U2);
                break;
            case fundamental_type::Int32:
                write(ElementType::I4);
                break;
            case fundamental_type::UInt32:
                write(ElementType::U4);
                break;
            case fundamental_type::Int64:
                write(ElementType::I8);
                break;
            case fundamental_type::UInt64:
                write(ElementType::U8);
                break;
            case fundamental_type::Float:
                write(ElementType::R4);
                break;
            case fundamental_type::Double:
                write(ElementType::R8);
                break;
            case fundamental_type::String:
                write(ElementType::String);
                break;
            }
        }

        void write(object_type const&)
        {
            write(ElementType::Object);
        }

        void write(guid_type const&)
        {
            write("winrt::guid");
        }

        void write(generic_type_instance type)
        {
            write("%<%>", type.generic_type, bind_list(", ", type.generic_args));
        }

        void write(type_semantics semantics)
        {
            call(semantics,
                [&](auto&& type) { write(type); });
        }
    };

    struct separator
    {
        writer& w;
        std::string_view _separator{ ", " };
        bool first{ true };

        void operator()()
        {
            if (first)
            {
                first = false;
            }
            else
            {
                w.write(_separator);
            }
        }
    };

    template <typename F>
    void enumerate_required_types(writer& w, TypeDef const& type, F func)
    {
        std::set<TypeDef> types;

        auto enumerate_types_impl = [&](type_semantics const& semantics, auto const& lambda) -> void
        {
            auto type = get_typedef(semantics);
            auto generic_args = std::visit(
                impl::overloaded{
                    [](type_definition) -> std::vector<type_semantics> { return {}; },
                    [](generic_type_instance type_instance) { return type_instance.generic_args; },
                    [](auto) -> std::vector<type_semantics> { throw_invalid("type doesn't contain typedef"); }
                }, semantics);

            auto guard{ w.push_generic_params(generic_args) };

            if (!contains(types, type))
            {
                types.insert(type);
                func(type);
            }

            if (get_category(type) == category::interface_type)
            {
                for (auto&& ii : type.InterfaceImpl())
                {
                    lambda(get_type_semantics(ii.Interface()), lambda);
                }
            }
        };

        enumerate_types_impl(type, enumerate_types_impl);
    }

    template <typename F>
    void enumerate_methods(writer& w, TypeDef const& type, F func)
    {
        enumerate_required_types(w, type, [&](TypeDef const& required_type)
        {
            for (auto&& method : required_type.MethodList())
            {
                if (is_constructor(method) || method.SpecialName()) continue;

                func(method);
            }
        });
    }

    template <typename F>
    void enumerate_properties(writer& w, TypeDef const& type, F func)
    {
        enumerate_required_types(w, type, [&](TypeDef const& required_type)
        {
            for (auto&& prop : required_type.PropertyList())
            {
                func(prop);
            }
        });
    }

    template <typename F>
    void enumerate_events(writer& w, TypeDef const& type, F func)
    {
        enumerate_required_types(w, type, [&](TypeDef const& required_type)
        {
            for (auto&& evt : required_type.EventList())
            {
                func(evt);
            }
        });
    }
}