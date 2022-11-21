#include "pch.h"

using namespace std::chrono;
using namespace std::experimental::filesystem;
using namespace std::string_view_literals;
using namespace xlang;
using namespace xlang::meta::reader;
using namespace xlang::text;
using namespace xlang::cmd;

template <typename...T> struct overloaded : T... { using T::operator()...; };
template <typename...T> overloaded(T...)->overloaded<T...>;

struct writer : indented_writer_base<writer>
{
    using indented_writer_base<writer>::write;

    std::string_view current;

    std::vector<std::pair<GenericParam, GenericParam>> generic_param_stack;

    struct generic_param_guard
    {
        explicit generic_param_guard(writer* arg)
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
        generic_param_guard& operator=(generic_param_guard const&) = delete;
        writer* owner;
    };

    generic_param_guard push_generic_params(std::pair<GenericParam, GenericParam>&& arg)
    {
        generic_param_stack.push_back(std::move(arg));
        return generic_param_guard{ this };
    }

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

    void write(TypeDef const& type)
    {
        write("%.%", type.TypeNamespace(), type.TypeName());
    }

    void write(TypeRef const& type)
    {
        auto ns = type.TypeNamespace();

        if (ns == current)
        {
            write("%", type.TypeName());
        }
        else
        {
            write("%.%", type.TypeNamespace(), type.TypeName());
        }
    }

    void write(TypeSpec const& type)
    {
        write(type.Signature().GenericTypeInst());
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
            write(type.TypeSpec());
            break;
        }
    }

    void write(GenericTypeInstSig const& type)
    {
        write("%<%>",
            type.GenericType(),
            bind_list(", ", type.GenericArgs()));
    }

    void write(TypeSig const& signature)
    {
        std::visit(overloaded
            {
                [&](ElementType type)
                {
                    if (type <= ElementType::String)
                    {
                        static constexpr const char* primitives[]
                        {
                            "End",
                            "Void",
                            "Boolean",
                            "Char",
                            "Int8",
                            "UInt8",
                            "Int16",
                            "UInt16",
                            "Int32",
                            "UInt32",
                            "Int64",
                            "UInt64",
                            "Single",
                            "Double",
                            "String"
                        };

                        write(primitives[static_cast<uint32_t>(type)]);
                    }
                    else if (type == ElementType::Object)
                    {
                        write("Object");
                    }
                },
                [&](GenericTypeIndex var)
                {
                    write("%", begin(generic_param_stack.back())[var.index].Name());
                },
                [&](GenericMethodTypeIndex)
                {
                    throw_invalid("Generic methods not supported.");
                },
                [&](auto&& type)
                {
                    write(type);
                }
            },
            signature.Type());
    }

    void write(InterfaceImpl const& impl)
    {
        write(impl.Interface());
    }

    void write(MethodDef const& method)
    {
        auto signature{ method.Signature() };

        auto param_list = method.ParamList();
        Param param;

        if (method.Signature().ReturnType() && !empty(param_list) && param_list.first.Sequence() == 0)
        {
            param = param_list.first + 1;
        }
        else
        {
            param = param_list.first;
        }

        bool first{ true };

        for (auto&& arg : signature.Params())
        {
            if (first)
            {
                first = false;
            }
            else
            {
                write(", ");
            }

            if (arg.ByRef())
            {
                write("ref ");
            }

            if (is_const(arg))
            {
                write("const ");
            }

            write("% %", arg.Type(), param.Name());
            ++param;
        }
    }

    void write(RetTypeSig const& signature)
    {
        if (signature)
        {
            write(signature.Type());
        }
        else
        {
            write("void");
        }
    }

    std::vector<Field> find_enumerators(ElemSig::EnumValue const& arg)
    {
        std::vector<Field> result;
        uint64_t const original_value = std::visit([](auto&& value) { return static_cast<uint64_t>(value); }, arg.value);
        uint64_t flags_value = original_value;

        auto get_enumerator_value = [](auto&& arg) -> uint64_t
        {
            if constexpr (std::is_integral_v<std::remove_reference_t<decltype(arg)>>)
            {
                return static_cast<uint64_t>(arg);
            }
            else
            {
                throw_invalid("Non-integral enumerator encountered");
            }
        };

        for (auto const& field : arg.type.m_typedef.FieldList())
        {
            if (auto const& constant = field.Constant())
            {
                uint64_t const enumerator_value = std::visit(get_enumerator_value, constant.Value());
                if (enumerator_value == original_value)
                {
                    result.assign(1, field);
                    return result;
                }
                else if ((flags_value & enumerator_value) == enumerator_value)
                {
                    result.push_back(field);
                    flags_value &= (~enumerator_value);
                }
            }
        }

        // Didn't find a match, or a set of flags that could build up the value
        if (flags_value != 0)
        {
            result.clear();
        }
        return result;
    }

    void write(FixedArgSig const& arg)
    {
        std::visit(overloaded{
            [this](ElemSig::SystemType arg)
            {
                write(arg.name);
            },
            [this](ElemSig::EnumValue arg)
            {
                auto const enumerators = find_enumerators(arg);
                if (enumerators.empty())
                {
                    std::visit([this](auto&& value) { write_value(value); }, arg.value);
                }
                else
                {
                    bool first = true;
                    for (auto const& enumerator : enumerators)
                    {
                        if (!first)
                        {
                            write(" | ");
                        }
                        write("%.%.%", arg.type.m_typedef.TypeNamespace(), arg.type.m_typedef.TypeName(), enumerator.Name());
                        first = false;
                    }
                }
            },
            [this](auto&& arg)
            {
                write_value(arg);
            }
        }, std::get<ElemSig>(arg.value).value);
    }

    void write(NamedArgSig const& arg)
    {
        write(arg.value);
    }

    void write_uuid(CustomAttributeSig const& arg)
    {
        auto const& args = arg.FixedArgs();

        write_printf("[Windows.Foundation.Metadata.GuidAttribute(%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X)]",
            std::get<uint32_t>(std::get<ElemSig>(args[0].value).value),
            std::get<uint16_t>(std::get<ElemSig>(args[1].value).value),
            std::get<uint16_t>(std::get<ElemSig>(args[2].value).value),
            std::get<uint8_t>(std::get<ElemSig>(args[3].value).value),
            std::get<uint8_t>(std::get<ElemSig>(args[4].value).value),
            std::get<uint8_t>(std::get<ElemSig>(args[5].value).value),
            std::get<uint8_t>(std::get<ElemSig>(args[6].value).value),
            std::get<uint8_t>(std::get<ElemSig>(args[7].value).value),
            std::get<uint8_t>(std::get<ElemSig>(args[8].value).value),
            std::get<uint8_t>(std::get<ElemSig>(args[9].value).value),
            std::get<uint8_t>(std::get<ElemSig>(args[10].value).value));
    }

    void write(CustomAttribute const& attr)
    {
        auto const& name = attr.TypeNamespaceAndName();
        auto const& sig = attr.Value();

        if (name.first == "Windows.Foundation.Metadata"sv && name.second == "GuidAttribute"sv)
        {
            write_uuid(sig);
            return;
        }

        write("[%.%", name.first, name.second);

        bool first = true;
        for (auto const& fixed_arg : sig.FixedArgs())
        {
            if (first)
            {
                first = false;
                write("(%", fixed_arg);
            }
            else
            {
                write(", %", fixed_arg);
            }
        }
        for (auto const& named_arg : sig.NamedArgs())
        {
            if (first)
            {
                first = false;
                write("(%", named_arg);
            }
            else
            {
                write(", %", named_arg);
            }
        }
        if (first)
        {
            write("]");
        }
        else
        {
            write(")]");
        }
    }
};

void write_custom_attribute(writer& w, CustomAttribute const& attr)
{
    w.write("\n%", attr);
}

void write_type_name(writer& w, std::string_view name)
{
    w.write(name);
    bool found = false;
    for (auto const& param : w.generic_param_stack.back())
    {
        if (found)
        {
            w.write(", %", param.Name());
        }
        else
        {
            found = true;
            w.write("<%", param.Name());
        }
    }
    if (found)
    {
        w.write(">");
    }
}

void write_enum_field(writer& w, Field const& field)
{
    writer::indent_guard _{ w };

    if (auto const& constant = field.Constant())
    {
        w.write("\n% = %,", field.Name(), constant);
    }
}

void write_enum(writer& w, TypeDef const& type)
{
    writer::indent_guard _{ w };

    w.write("%\nenum %\n{%\n};\n",
        bind_each<write_custom_attribute>(type.CustomAttribute()),
        type.TypeName(),
        bind_each<write_enum_field>(type.FieldList()));
}

void write_struct_field(writer& w, Field const& field)
{
    writer::indent_guard _{ w };

    w.write("\n% %;", field.Signature().Type(), field.Name());
}

void write_struct(writer& w, TypeDef const& type)
{
    writer::indent_guard _{ w };

    w.write("%\nstruct %\n{%\n};\n",
        bind_each<write_custom_attribute>(type.CustomAttribute()),
        type.TypeName(),
        bind_each<write_struct_field>(type.FieldList()));
}

void write_delegate(writer& w, TypeDef const& type)
{
    auto guard{ w.push_generic_params(type.GenericParam()) };
    auto methods = type.MethodList();
    writer::indent_guard _{ w };

    auto method = std::find_if(begin(methods), end(methods), [](auto&& method)
    {
        return method.Name() == "Invoke";
    });

    if (method == end(methods))
    {
        throw_invalid("Delegate's Invoke method not found");
    }

    w.write("%\ndelegate % %(%);\n",
        bind_each<write_custom_attribute>(type.CustomAttribute()),
        method.Signature().ReturnType(),
        bind<write_type_name>(type.TypeName()),
        method);
}

void write_method(writer& w, MethodDef const& method)
{
    w.write("%\n% %(%);",
        bind_each<write_custom_attribute>(method.CustomAttribute()),
        method.Signature().ReturnType(),
        method.Name(),
        method);
}

void write_method_semantic(writer& w, MethodSemantics const& method_semantic, MethodDef& method)
{
    auto const& target = method_semantic.Association();
    auto const& semantic = method_semantic.Semantic();

    if (target.type() == HasSemantics::Property)
    {
        if (!semantic.Getter() && !semantic.Setter())
        {
            xlang::throw_invalid("Invalid semantic: properties can only have a setter and/or getter");
        }
        auto const& property = target.Property();
        auto const& accessors = property.MethodSemantic();

        if (distance(accessors) < 1 || 2 < distance(accessors))
        {
            xlang::throw_invalid("Properties can only have 1 or 2 accessors");
        }

        if (distance(accessors) == 2)
        {
            auto const& other_method_semantic = (accessors.first == method_semantic) ? accessors.first + 1 : accessors.first;
            auto const& other_method = other_method_semantic.Method();
            if (method < other_method)
            {
                // First accessor seen for this property
                w.write(bind_each<write_custom_attribute>(property.CustomAttribute()));
            }

            if (semantic.Getter())
            {
                if (method + 1 == other_method)
                {
                    if (!other_method_semantic.Semantic().Setter())
                    {
                        xlang::throw_invalid("Invalid semantic: properties can only have a setter and/or getter");
                    }
                    w.write("\n% %;", property.Type().Type(), property.Name());
                    ++method;
                }
                else
                {
                    XLANG_ASSERT(semantic.Getter());
                    w.write("\n% % { get; };", property.Type().Type(), property.Name());
                }
            }
            else
            {
                XLANG_ASSERT(semantic.Setter());
                w.write("\n% % { set; };", property.Type().Type(), property.Name());
            }
        }
        else
        {
            XLANG_ASSERT(distance(accessors) == 1);
            w.write(bind_each<write_custom_attribute>(property.CustomAttribute()));

            if (semantic.Getter())
            {
                w.write("\n% % { get; };", property.Type().Type(), property.Name());
            }
            else
            {
                XLANG_ASSERT(semantic.Setter());
                w.write("\n% % { set; };", property.Type().Type(), property.Name());
            }
        }
    }
    else
    {
        if (!semantic.AddOn() && !semantic.RemoveOn())
        {
            xlang::throw_invalid("Invalid semantic: events can only have an add and/or remove");
        }
        auto const& event = target.Event();
        auto const& accessors = event.MethodSemantic();

        if (distance(accessors) < 1 || 2 < distance(accessors))
        {
            xlang::throw_invalid("Events can only have 1 or 2 accessors");
        }

        if (distance(accessors) == 2)
        {
            auto const& other_method_semantic = (accessors.first == method_semantic) ? accessors.first + 1 : accessors.first;
            auto const& other_method = other_method_semantic.Method();
            if (method < other_method)
            {
                // First accessor seen for this event
                w.write(bind_each<write_custom_attribute>(event.CustomAttribute()));
            }

            if (semantic.AddOn())
            {
                if (method + 1 == other_method)
                {
                    if (!other_method_semantic.Semantic().RemoveOn())
                    {
                        xlang::throw_invalid("Invalid semantic: events can only have a add and/or remove");
                    }
                    w.write("\n% %;", event.EventType(), event.Name());
                    ++method;
                }
                else
                {
                    XLANG_ASSERT(semantic.AddOn());
                    w.write("\n% % { add; };", event.EventType(), event.Name());
                }
            }
            else
            {
                XLANG_ASSERT(semantic.RemoveOn());
                w.write("\n% % { remove; };", event.EventType(), event.Name());
            }
        }
        else
        {
            XLANG_ASSERT(distance(accessors) == 1);
            w.write(bind_each<write_custom_attribute>(event.CustomAttribute()));

            if (semantic.AddOn())
            {
                w.write("\n% % { add; };", event.EventType(), event.Name());
            }
            else
            {
                XLANG_ASSERT(semantic.RemoveOn());
                w.write("\n% % { remove; };", event.EventType(), event.Name());
            }
        }
    }
}


void write_required_interface(writer& w, InterfaceImpl const& interface_impl)
{
    writer::indent_guard _{ w };

    w.write("\n%", interface_impl.Interface());
}

void write_required(writer& w, std::string_view const& requires, TypeDef const& type)
{
    auto interfaces{ type.InterfaceImpl() };

    if (begin(interfaces) == end(interfaces))
    {
        return;
    }

    w.write(" %%",
        requires,
        bind_list<write_required_interface>(",", interfaces));
}

void write_interface_methods(writer& w, TypeDef const& type)
{
    auto const& methods = type.MethodList();
    auto const& properties = type.PropertyList();
    auto const& events = type.EventList();
    writer::indent_guard _{ w };

    auto method_semantic = [&properties, &events](MethodDef const& method) -> MethodSemantics
    {
        for (auto const& prop : properties)
        {
            for (auto const& semantic : prop.MethodSemantic())
            {
                if (semantic.Method() == method)
                {
                    return semantic;
                }
            }
        }
        for (auto const& event : events)
        {
            for (auto const& semantic : event.MethodSemantic())
            {
                if (semantic.Method() == method)
                {
                    return semantic;
                }
            }
        }
        return {};
    };

    for (auto method = begin(methods); method != end(methods); ++method)
    {
        auto const& semantic = method_semantic(method);
        if (semantic)
        {
            write_method_semantic(w, semantic, method);
        }
        else
        {
            write_method(w, method);
        }
    }
}

void write_interface(writer& w, TypeDef const& type)
{
    auto guard = w.push_generic_params(type.GenericParam());
    writer::indent_guard _{ w };

    w.write("%\ninterface %%\n{%\n};\n",
        bind_each<write_custom_attribute>(type.CustomAttribute()),
        bind<write_type_name>(type.TypeName()),
        bind<write_required>("requires", type),
        bind<write_interface_methods>(type));
}

void write_class(writer& w, TypeDef const& type)
{
    auto guard = w.push_generic_params(type.GenericParam());
    writer::indent_guard _{ w };

    w.write("%\nruntimeclass %%\n{\n};\n",
        bind_each<write_custom_attribute>(type.CustomAttribute()),
        bind<write_type_name>(type.TypeName()),
        bind<write_required>(":", type));
}

auto get_out(reader const& args)
{
    auto out{ absolute(args.value("output", "idl")) };
    create_directories(out);
    out += path::preferred_separator;
    return out.string();
}

void print_usage()
{
    puts("Usage...");
}

int main(int const argc, char** argv)
{
    writer w;

    try
    {
        auto start = high_resolution_clock::now();

        static constexpr cmd::option options[]
        {
            // name, min, max
            { "input", 1 },
            { "output", 0, 1 },
            { "include", 0 },
            { "exclude", 0 },
            { "verbose", 0, 0 },
        };

        reader args{ argc, argv, options };

        if (!args)
        {
            print_usage();
            return 0;
        }

        cache c{ args.values("input") };
        auto const out = get_out(args);
        bool const verbose = args.exists("verbose");

        filter f{ args.values("include"), args.values("exclude") };

        if (verbose)
        {
            std::for_each(c.databases().begin(), c.databases().end(), [&](auto&& db)
            {
                w.write("in: %\n", db.path());
            });

            w.write("out: %\n", out);
        }

        w.flush_to_console();
        task_group group;

        for (auto const& [ns, members] : c.namespaces())
        {
            group.add([&, &ns = ns, &members = members]
            {
                if (!f.includes(members))
                {
                    return;
                }

                writer w;
                w.current = ns;

                w.write("\nnamespace %\n{%%%%%}\n",
                    ns,
                    f.bind_each<write_enum>(members.enums),
                    f.bind_each<write_struct>(members.structs),
                    f.bind_each<write_delegate>(members.delegates),
                    f.bind_each<write_interface>(members.interfaces),
                    f.bind_each<write_class>(members.classes));

                auto filename{ out };
                filename += ns;
                filename += ".idl";
                w.flush_to_file(filename);
            });
        }

        group.get();

        if (verbose)
        {
            w.write("time: %ms\n", duration_cast<duration<int64_t, std::milli>>(high_resolution_clock::now() - start).count());
        }
    }
    catch (std::exception const& e)
    {
        w.write("%\n", e.what());
    }

    w.flush_to_console();
}
