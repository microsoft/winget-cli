#include "pch.h"

#include "abi_writer.h"
#include "code_writers.h"
#include "types.h"
#include "type_banners.h"

using namespace std::literals;
using namespace xlang::meta::reader;
using namespace xlang::text;

template <typename T>
static std::size_t push_type_contract_guards(writer& w, T const& type)
{
    if (auto vers = get_contract_history(type))
    {
        w.push_contract_guard(*vers);
        return 1;
    }

    return 0;
}

template <typename T>
static std::size_t begin_type_definition(writer& w, T const& type)
{
    write_type_banner(w, type);
    if (type.is_experimental())
    {
        w.write("#if defined(ENABLE_WINRT_EXPERIMENTAL_TYPES)\n");
    }

    return type.push_contract_guards(w);
}

template <typename T>
static void end_type_definition(writer& w, T const& type, std::size_t contractGuardDepth)
{
    w.pop_contract_guards(contractGuardDepth);
    if (type.is_experimental())
    {
        w.write("#endif // defined(ENABLE_WINRT_EXPERIMENTAL_TYPES)\n");
    }

    w.write('\n');
}

typedef_base::typedef_base(TypeDef const& type) :
    m_type(type),
    m_clrFullName(::clr_full_name(type)),
    m_mangledName(::mangled_name<false>(type)),
    m_genericParamMangledName(::mangled_name<true>(type)),
    m_contractHistory(get_contract_history(type))
{
    for_each_attribute(type, metadata_namespace, "VersionAttribute"sv, [&](bool, CustomAttribute const& attr)
    {
        m_platformVersions.push_back(decode_platform_version(attr));
    });
}

std::size_t typedef_base::push_contract_guards(writer& w) const
{
    XLANG_ASSERT(!is_generic());

    if (m_contractHistory)
    {
        w.push_contract_guard(*m_contractHistory);
        return 1;
    }

    return 0;
}

void typedef_base::write_cpp_abi_name(writer& w) const
{
    write_cpp_fully_qualified_type(w, clr_abi_namespace(), cpp_abi_name());
}

static std::string_view enum_string(writer& w)
{
    return w.config().enum_class ? "MIDL_ENUM"sv : "enum"sv;
}

void enum_type::write_cpp_forward_declaration(writer& w) const
{
    if (!w.should_forward_declare(m_mangledName))
    {
        return;
    }

    w.push_namespace(clr_abi_namespace());
    auto typeStr = underlying_type() == ElementType::I4 ? "int"sv : "unsigned int"sv;
    w.write("%typedef % % : % %;\n", indent{}, enum_string(w), cpp_abi_name(), typeStr, cpp_abi_name());
    w.pop_namespace();
    w.write('\n');
}

void enum_type::write_cpp_generic_param_abi_type(writer& w) const
{
    w.write("% ", enum_string(w));
    write_cpp_abi_param(w);
}

void enum_type::write_cpp_abi_param(writer& w) const
{
    // Enums are passed by value
    write_cpp_abi_name(w);
}

void enum_type::write_c_forward_declaration(writer& w) const
{
    if (!w.should_forward_declare(m_mangledName))
    {
        return;
    }

    w.write("typedef enum % %;\n\n", bind_c_type_name(*this), bind_c_type_name(*this));
}

void enum_type::write_c_abi_param(writer& w) const
{
    w.write("enum %", bind_c_type_name(*this));
}

void enum_type::write_cpp_definition(writer& w) const
{
    auto name = cpp_abi_name();
    auto contractDepth = begin_type_definition(w, *this);
    w.push_namespace(clr_abi_namespace());

    w.write("%%", indent{}, enum_string(w));
    if (auto info = is_deprecated(); info && w.config().enable_header_deprecation)
    {
        w.write("\n");
        write_deprecation_message(w, *info);
        w.write("%", indent{});
    }
    else
    {
        w.write(' ');
    }

    auto typeStr = underlying_type() == ElementType::I4 ? "int"sv : "unsigned int"sv;
    w.write(R"^-^(% : %
%{
)^-^", name, typeStr, indent{});

    for (auto const& field : m_type.FieldList())
    {
        if (auto value = field.Constant())
        {
            auto isExperimental = ::is_experimental(field);
            if (isExperimental)
            {
                w.write("#if defined(ENABLE_WINRT_EXPERIMENTAL_TYPES)\n");
            }

            auto fieldContractDepth = push_type_contract_guards(w, field);

            w.write("%", indent{ 1 });
            if (!w.config().enum_class)
            {
                w.write("%_", name);
            }
            w.write(field.Name());

            if (auto info = ::is_deprecated(field); info && w.config().enable_header_deprecation)
            {
                w.write("\n");
                write_deprecation_message(w, *info, 1, "DEPRECATEDENUMERATOR");
                w.write("%", indent{ 1 });
            }
            else
            {
                w.write(' ');
            }

            w.write("= %,\n", value);
            w.pop_contract_guards(fieldContractDepth);
            if (isExperimental)
            {
                w.write("#endif // defined(ENABLE_WINRT_EXPERIMENTAL_TYPES)\n");
            }
        }
    }

    w.write("%};\n", indent{});

    if (is_flags_enum(m_type))
    {
        w.write("\n%DEFINE_ENUM_FLAG_OPERATORS(%)\n", indent{}, name);
    }

    if (w.config().enum_class)
    {
        w.write('\n');
        for (auto const& field : m_type.FieldList())
        {
            if (field.Constant())
            {
                w.write("%const % %_% = %::%;\n", indent{}, name, name, field.Name(), name, field.Name());
            }
        }
    }

    w.pop_namespace();
    end_type_definition(w, *this, contractDepth);
}

void enum_type::write_c_definition(writer& w) const
{
    auto contractDepth = begin_type_definition(w, *this);

    w.write("enum");
    if (auto info = is_deprecated(); info && w.config().enable_header_deprecation)
    {
        w.write("\n");
        write_deprecation_message(w, *info);
    }
    else
    {
        w.write(' ');
    }

    w.write(R"^-^(%
{
)^-^", bind_c_type_name(*this));

    for (auto const& field : m_type.FieldList())
    {
        if (auto value = field.Constant())
        {
            auto isExperimental = ::is_experimental(field);
            if (isExperimental)
            {
                w.write("#if defined(ENABLE_WINRT_EXPERIMENTAL_TYPES)\n");
            }

            auto fieldContractDepth = push_type_contract_guards(w, field);

            w.write("    %_%", cpp_abi_name(), field.Name());
            if (auto info = ::is_deprecated(field); info && w.config().enable_header_deprecation)
            {
                w.write("\n");
                write_deprecation_message(w, *info, 1, "DEPRECATEDENUMERATOR");
                w.write("   ");
            }

            w.write(" = %,\n", value);
            w.pop_contract_guards(fieldContractDepth);
            if (isExperimental)
            {
                w.write("#endif // defined(ENABLE_WINRT_EXPERIMENTAL_TYPES)\n");
            }
        }
    }

    w.write("};\n");
    end_type_definition(w, *this, contractDepth);
}

void struct_type::write_cpp_forward_declaration(writer& w) const
{
    if (!w.should_forward_declare(m_mangledName))
    {
        return;
    }

    w.push_namespace(clr_abi_namespace());
    w.write("%typedef struct % %;\n", indent{}, cpp_abi_name(), cpp_abi_name());
    w.pop_namespace();
    w.write('\n');
}

void struct_type::write_cpp_generic_param_abi_type(writer& w) const
{
    w.write("struct ");
    write_cpp_abi_param(w);
}

void struct_type::write_cpp_abi_param(writer& w) const
{
    // Structs are passed by value
    write_cpp_abi_name(w);
}

void struct_type::write_c_forward_declaration(writer& w) const
{
    if (!w.should_forward_declare(m_mangledName))
    {
        return;
    }

    w.write("typedef struct % %;\n\n", bind_c_type_name(*this), bind_c_type_name(*this));
}

void struct_type::write_c_abi_param(writer& w) const
{
    w.write("struct %", bind_c_type_name(*this));
}

void struct_type::write_cpp_definition(writer& w) const
{
    auto contractDepth = begin_type_definition(w, *this);
    w.push_namespace(clr_abi_namespace());

    w.write("%struct", indent{});
    if (auto info = is_deprecated(); info && w.config().enable_header_deprecation)
    {
        w.write('\n');
        write_deprecation_message(w, *info);
        w.write("%", indent{});
    }
    else
    {
        w.write(' ');
    }

    w.write(R"^-^(%
%{
)^-^", cpp_abi_name(), indent{});

    for (auto const& member : members)
    {
        if (auto info = ::is_deprecated(member.field); info && w.config().enable_header_deprecation)
        {
            write_deprecation_message(w, *info, 1);
        }

        w.write("%% %;\n", indent{ 1 }, [&](writer& w) { member.type->write_cpp_abi_param(w); }, member.field.Name());
    }

    w.write("%};\n", indent{});

    w.pop_namespace();
    end_type_definition(w, *this, contractDepth);
}

void struct_type::write_c_definition(writer& w) const
{
    auto contractDepth = begin_type_definition(w, *this);

    w.write("struct");
    if (auto info = is_deprecated(); info && w.config().enable_header_deprecation)
    {
        w.write('\n');
        write_deprecation_message(w, *info);
    }
    else
    {
        w.write(' ');
    }

    w.write(R"^-^(%
{
)^-^", bind_c_type_name(*this));

    for (auto const& member : members)
    {
        if (auto info = ::is_deprecated(member.field); info && w.config().enable_header_deprecation)
        {
            write_deprecation_message(w, *info, 1);
        }

        w.write("    % %;\n", [&](writer& w) { member.type->write_c_abi_param(w); }, member.field.Name());
    }

    w.write("};\n");

    end_type_definition(w, *this, contractDepth);
}

void delegate_type::write_cpp_forward_declaration(writer& w) const
{
    if (!w.should_forward_declare(m_mangledName))
    {
        return;
    }

    w.write(R"^-^(#ifndef __%_FWD_DEFINED__
#define __%_FWD_DEFINED__
)^-^", bind_mangled_name_macro(*this), bind_mangled_name_macro(*this));

    w.push_namespace(clr_abi_namespace());
    w.write("%interface %;\n", indent{}, m_abiName);
    w.pop_namespace();

    w.write(R"^-^(#define % %

#endif // __%_FWD_DEFINED__

)^-^",
        bind_mangled_name_macro(*this),
        bind_cpp_fully_qualified_type(clr_abi_namespace(), m_abiName),
        bind_mangled_name_macro(*this));
}

void delegate_type::write_cpp_generic_param_abi_type(writer& w) const
{
    write_cpp_abi_param(w);
}

void delegate_type::write_cpp_abi_param(writer& w) const
{
    write_cpp_abi_name(w);
    w.write('*');
}

static std::string_view function_name(MethodDef const& def)
{
    // If this is an overload, use the unique name
    auto fnName = def.Name();
    if (auto overloadAttr = get_attribute(def, metadata_namespace, "OverloadAttribute"sv))
    {
        auto sig = overloadAttr.Value();
        auto const& fixedArgs = sig.FixedArgs();
        XLANG_ASSERT(fixedArgs.size() == 1);
        fnName = std::get<std::string_view>(std::get<ElemSig>(fixedArgs[0].value).value);
    }

    return fnName;
}

static void write_cpp_function_declaration(writer& w, function_def const& func)
{
    if (auto info = is_deprecated(func.def); info && w.config().enable_header_deprecation)
    {
        write_deprecation_message(w, *info, 1);
    }

    w.write("%virtual HRESULT STDMETHODCALLTYPE %(", indent{ 1 }, function_name(func.def));

    std::string_view prefix = "\n"sv;
    for (auto const& param : func.params)
    {
        auto refMod = param.signature.ByRef() ? "*"sv : ""sv;
        if (param.signature.Type().is_szarray())
        {
            w.write("%%UINT32% %Length", prefix, indent{ 2 }, refMod, param.name);
            refMod = param.signature.ByRef() ? "**"sv : "*"sv;
            prefix = ",\n";
        }

        auto constMod = is_const(param.signature) ? "const "sv : ""sv;
        w.write("%%%%% %",
            prefix,
            indent{ 2 },
            constMod,
            [&](writer& w) { param.type->write_cpp_abi_param(w); },
            refMod,
            param.name);
        prefix = ",\n";
    }

    if (func.return_type)
    {
        auto refMod = "*"sv;
        if (func.return_type->signature.Type().is_szarray())
        {
            w.write("%%UINT32* %Length", prefix, indent{ 2 }, func.return_type->name);
            refMod = "**"sv;
            prefix = ",\n";
        }

        w.write("%%%% %",
            prefix,
            indent{ 2 },
            [&](writer& w) { func.return_type->type->write_cpp_abi_param(w); },
            refMod,
            func.return_type->name);
    }

    if (func.params.empty() && !func.return_type)
    {
        w.write("void) = 0;\n");
    }
    else
    {
        w.write("\n%) = 0;\n", indent{ 2 });
    }
}

template <typename T>
static void write_cpp_interface_definition(writer& w, T const& type)
{
    constexpr bool is_delegate = std::is_same_v<T, delegate_type>;
    constexpr bool is_interface = std::is_same_v<T, interface_type>;
    static_assert(is_delegate || is_interface);

    w.push_namespace(type.clr_abi_namespace());

    w.write(R"^-^(%MIDL_INTERFACE("%")
)^-^", indent{}, bind_uuid(type));

    if (auto info = is_deprecated(type.type()); info && w.config().enable_header_deprecation)
    {
        write_deprecation_message(w, *info);
    }

    w.write("%% : public ", indent{}, type.cpp_abi_name());

    if constexpr (is_delegate)
    {
        w.write("IUnknown");
    }
    else if constexpr (is_interface)
    {
        w.write("IInspectable");
    }

    w.write(R"^-^(
%{
%public:
)^-^", indent{}, indent{});

    for (auto const& func : type.functions)
    {
        write_cpp_function_declaration(w, func);
    }

    if constexpr (is_interface)
    {
        if (type.fast_class)
        {
            w.write("\n%// Supplemental functions added by use of the fast ABI attribute\n", indent{});

            auto fastAttr = get_attribute(type.fast_class->type(), metadata_namespace, "FastAbiAttribute"sv);
            std::size_t fastContractDepth = w.push_contract_guard(version_from_attribute(fastAttr)) ? 1 : 0;

            // If the class "derives" from any other class, the fast pointer-to-base functions come first
            std::vector<class_type const*> baseClasses;
            auto base = type.fast_class->base_class;
            while (base)
            {
                baseClasses.push_back(base);
                base = base->base_class;
            }

            std::for_each(baseClasses.rbegin(), baseClasses.rend(), [&](class_type const* baseClass)
            {
                w.write("%virtual % base_%() = 0;\n", indent{}, [&](writer& w) { baseClass->write_cpp_abi_param(w); }, baseClass->cpp_logical_name());
            });

            for (auto [iface, ver] : type.fast_class->supplemental_fast_interfaces)
            {
                w.write("\n%// Supplemental functions added for the % interface\n", indent{}, iface->clr_full_name());
                fastContractDepth += w.push_contract_guard(ver) ? 1 : 0;

                for (auto const& func : iface->functions)
                {
                    write_cpp_function_declaration(w, func);
                }
            }

            w.pop_contract_guards(fastContractDepth);
        }
    }

    w.write(R"^-^(%};

%MIDL_CONST_ID IID& IID_% = __uuidof(%);
)^-^", indent{}, indent{}, type.cpp_abi_name(), type.cpp_abi_name());

    w.pop_namespace();
}

template <typename T>
static void write_c_iunknown_interface(writer& w, T const& type)
{
    w.write(R"^-^(    HRESULT (STDMETHODCALLTYPE* QueryInterface)(%* This,
        REFIID riid,
        void** ppvObject);
    ULONG (STDMETHODCALLTYPE* AddRef)(%* This);
    ULONG (STDMETHODCALLTYPE* Release)(%* This);
)^-^", bind_c_type_name(type), bind_c_type_name(type), bind_c_type_name(type));
}

template <typename T>
static void write_c_iunknown_interface_macros(writer& w, T const& type)
{
    w.write(R"^-^(#define %_QueryInterface(This, riid, ppvObject) \
    ((This)->lpVtbl->QueryInterface(This, riid, ppvObject))

#define %_AddRef(This) \
    ((This)->lpVtbl->AddRef(This))

#define %_Release(This) \
    ((This)->lpVtbl->Release(This))

)^-^", bind_mangled_name_macro(type), bind_mangled_name_macro(type), bind_mangled_name_macro(type));
}

template <typename T>
static void write_c_iinspectable_interface(writer& w, T const& type)
{
    write_c_iunknown_interface(w, type);
    w.write(R"^-^(    HRESULT (STDMETHODCALLTYPE* GetIids)(%* This,
        ULONG* iidCount,
        IID** iids);
    HRESULT (STDMETHODCALLTYPE* GetRuntimeClassName)(%* This,
        HSTRING* className);
    HRESULT (STDMETHODCALLTYPE* GetTrustLevel)(%* This,
        TrustLevel* trustLevel);
)^-^", bind_c_type_name(type), bind_c_type_name(type), bind_c_type_name(type));
}

template <typename T>
static void write_c_iinspectable_interface_macros(writer& w, T const& type)
{
    write_c_iunknown_interface_macros(w, type);
    w.write(R"^-^(#define %_GetIids(This, iidCount, iids) \
    ((This)->lpVtbl->GetIids(This, iidCount, iids))

#define %_GetRuntimeClassName(This, className) \
    ((This)->lpVtbl->GetRuntimeClassName(This, className))

#define %_GetTrustLevel(This, trustLevel) \
    ((This)->lpVtbl->GetTrustLevel(This, trustLevel))

)^-^", bind_mangled_name_macro(type), bind_mangled_name_macro(type), bind_mangled_name_macro(type));
}

template <typename TypeName>
static void write_c_function_declaration(writer& w, TypeName&& typeName, function_def const& func)
{
    if (auto info = is_deprecated(func.def); info && w.config().enable_header_deprecation)
    {
        write_deprecation_message(w, *info, 1);
    }

    w.write("    HRESULT (STDMETHODCALLTYPE* %)(%* This", function_name(func.def), typeName);

    for (auto const& param : func.params)
    {
        auto refMod = param.signature.ByRef() ? "*"sv : ""sv;
        if (param.signature.Type().is_szarray())
        {
            w.write(",\n        UINT32% %Length", refMod, param.name);
            refMod = param.signature.ByRef() ? "**"sv : "*"sv;
        }

        auto constMod = is_const(param.signature) ? "const "sv : ""sv;
        w.write(",\n        %%% %",
            constMod,
            [&](writer& w) { param.type->write_c_abi_param(w); },
            refMod,
            param.name);
    }

    if (func.return_type)
    {
        auto refMod = "*"sv;
        if (func.return_type->signature.Type().is_szarray())
        {
            w.write(",\n        UINT32* %Length", func.return_type->name);
            refMod = "**"sv;
        }

        w.write(",\n        %% %",
            [&](writer& w) { func.return_type->type->write_c_abi_param(w); },
            refMod,
            func.return_type->name);
    }

    w.write(");\n");
}

template <typename T>
static void write_c_function_declaration_macro(writer& w, T const& type, function_def const& func)
{
    if (auto info = is_deprecated(func.def); info && w.config().enable_header_deprecation)
    {
        write_deprecation_message(w, *info, 1);
    }

    auto fnName = function_name(func.def);
    w.write("#define %_%(This", bind_mangled_name_macro(type), fnName);

    for (auto const& param : func.params)
    {
        if (param.signature.Type().is_szarray())
        {
            w.write(", %Length", param.name);
        }

        w.write(", %", param.name);
    }

    if (func.return_type)
    {
        if (func.return_type->signature.Type().is_szarray())
        {
            w.write(", %Length", func.return_type->name);
        }

        w.write(", %", func.return_type->name);
    }

    w.write(R"^-^() \
    ((This)->lpVtbl->%(This)^-^", fnName);

    for (auto const& param : func.params)
    {
        if (param.signature.Type().is_szarray())
        {
            w.write(", %Length", param.name);
        }

        w.write(", %", param.name);
    }

    if (func.return_type)
    {
        if (func.return_type->signature.Type().is_szarray())
        {
            w.write(", %Length", func.return_type->name);
        }

        w.write(", %", func.return_type->name);
    }

    w.write("))\n\n");
}

template <typename T>
static void write_c_interface_definition(writer& w, T const& type)
{
    constexpr bool is_interface = std::is_same_v<T, interface_type>;
    constexpr bool is_delegate = std::is_same_v<T, delegate_type>;
    constexpr bool is_generic = std::is_same_v<T, generic_inst>;
    static_assert(is_interface || is_delegate | is_generic);

    w.write("typedef struct");
    if (auto info = type.is_deprecated(); info && w.config().enable_header_deprecation)
    {
        w.write('\n');
        write_deprecation_message(w, *info);
    }
    else
    {
        w.write(' ');
    }

    w.write(R"^-^(%
{
    BEGIN_INTERFACE

)^-^", bind_c_type_name(type, "Vtbl"));

    bool isDelegate = type.category() == category::delegate_type;
    if (isDelegate)
    {
        write_c_iunknown_interface(w, type);
    }
    else
    {
        write_c_iinspectable_interface(w, type);
    }

    for (auto const& func : type.functions)
    {
        write_c_function_declaration(w, bind_c_type_name(type), func);
    }

    if constexpr (is_interface)
    {
        if (type.fast_class)
        {
            w.write("\n// Supplemental functions added by use of the fast ABI attribute\n");

            auto fastAttr = get_attribute(type.fast_class->type(), metadata_namespace, "FastAbiAttribute"sv);
            std::size_t fastContractDepth = w.push_contract_guard(version_from_attribute(fastAttr)) ? 1 : 0;

            // If the class "derives" from any other class, the fast pointer-to-base functions come first
            std::vector<class_type const*> baseClasses;
            auto base = type.fast_class->base_class;
            while (base)
            {
                baseClasses.push_back(base);
                base = base->base_class;
            }

            std::for_each(baseClasses.rbegin(), baseClasses.rend(), [&](class_type const* baseClass)
            {
                w.write("    % (STDMETHODCALLTYPE* base_%)(%* This);\n",
                    [&](writer& w) { baseClass->write_c_abi_param(w); },
                    baseClass->cpp_logical_name(),
                    bind_c_type_name(type));
            });

            for (auto [iface, ver] : type.fast_class->supplemental_fast_interfaces)
            {
                w.write("\n    // Supplemental functions added for the % interface\n", iface->clr_full_name());
                fastContractDepth += w.push_contract_guard(ver) ? 1 : 0;

                for (auto const& func : iface->functions)
                {
                    write_c_function_declaration(w, bind_c_type_name(type), func);
                }
            }

            w.pop_contract_guards(fastContractDepth);
        }
    }

    w.write(R"^-^(
    END_INTERFACE
} %;

interface %
{
    CONST_VTBL struct %* lpVtbl;
};

#ifdef COBJMACROS

)^-^", bind_c_type_name(type, "Vtbl"), bind_c_type_name(type), bind_c_type_name(type, "Vtbl"));

    if (isDelegate)
    {
        write_c_iunknown_interface_macros(w, type);
    }
    else
    {
        write_c_iinspectable_interface_macros(w, type);
    }

    for (auto const& func : type.functions)
    {
        write_c_function_declaration_macro(w, type, func);
    }

    if constexpr (is_interface)
    {
        if (type.fast_class)
        {
            w.write("// Supplemental functions added by use of the fast ABI attribute\n");

            auto fastAttr = get_attribute(type.fast_class->type(), metadata_namespace, "FastAbiAttribute"sv);
            std::size_t fastContractDepth = w.push_contract_guard(version_from_attribute(fastAttr)) ? 1 : 0;
            w.write("\n");

            // If the class "derives" from any other class, the fast pointer-to-base functions come first
            std::vector<class_type const*> baseClasses;
            auto base = type.fast_class->base_class;
            while (base)
            {
                baseClasses.push_back(base);
                base = base->base_class;
            }

            std::for_each(baseClasses.rbegin(), baseClasses.rend(), [&](class_type const* baseClass)
            {
                w.write(R"^-^(#define %_base_%(This) \
    ((This)->lpVtbl->base_%(This))

)^-^", bind_mangled_name_macro(type), baseClass->cpp_logical_name(), baseClass->cpp_logical_name());
            });

            for (auto [iface, ver] : type.fast_class->supplemental_fast_interfaces)
            {
                w.write("// Supplemental functions added for the % interface\n", iface->clr_full_name());
                fastContractDepth += w.push_contract_guard(ver) ? 1 : 0;
                w.write("\n");

                for (auto const& func : iface->functions)
                {
                    write_c_function_declaration_macro(w, type, func);
                }
            }

            w.pop_contract_guards(fastContractDepth);
            if (fastContractDepth > 0)
            {
                w.write("\n");
            }
        }
    }

    w.write(R"^-^(#endif /* COBJMACROS */
)^-^");
}

void delegate_type::write_c_forward_declaration(writer& w) const
{
    if (!w.should_forward_declare(m_mangledName))
    {
        return;
    }

    w.write(R"^-^(#ifndef __%_FWD_DEFINED__
#define __%_FWD_DEFINED__
)^-^", bind_mangled_name_macro(*this), bind_mangled_name_macro(*this));

    w.write(R"^-^(typedef interface % %;

#endif // __%_FWD_DEFINED__

)^-^",
        bind_c_type_name(*this),
        bind_c_type_name(*this),
        bind_mangled_name_macro(*this));
}

void delegate_type::write_c_abi_param(writer& w) const
{
    w.write("%*", bind_c_type_name(*this));
}

static void write_delegate_definition(writer& w, delegate_type const& type, void (*func)(writer&, delegate_type const&))
{
    // Generics don't get generated definitions
    if (type.is_generic())
    {
        return;
    }

    auto contractDepth = begin_type_definition(w, type);

    w.write(R"^-^(#if !defined(__%_INTERFACE_DEFINED__)
#define __%_INTERFACE_DEFINED__
)^-^", bind_mangled_name_macro(type), bind_mangled_name_macro(type));

    func(w, type);

    w.write(R"^-^(
EXTERN_C const IID %;
#endif /* !defined(__%_INTERFACE_DEFINED__) */
)^-^", bind_iid_name(type), bind_mangled_name_macro(type));

    end_type_definition(w, type, contractDepth);
}

void delegate_type::write_cpp_definition(writer& w) const
{
    write_delegate_definition(w, *this, &write_cpp_interface_definition<delegate_type>);
}

void delegate_type::write_c_definition(writer& w) const
{
    write_delegate_definition(w, *this, &write_c_interface_definition<delegate_type>);
}

void interface_type::write_cpp_forward_declaration(writer& w) const
{
    if (!w.should_forward_declare(m_mangledName))
    {
        return;
    }

    w.write(R"^-^(#ifndef __%_FWD_DEFINED__
#define __%_FWD_DEFINED__
)^-^", bind_mangled_name_macro(*this), bind_mangled_name_macro(*this));

    w.push_namespace(clr_abi_namespace());
    w.write("%interface %;\n", indent{}, m_type.TypeName());
    w.pop_namespace();

    w.write(R"^-^(#define % %

#endif // __%_FWD_DEFINED__

)^-^",
        bind_mangled_name_macro(*this),
        bind_cpp_fully_qualified_type(clr_abi_namespace(), m_type.TypeName()),
        bind_mangled_name_macro(*this));
}

void interface_type::write_cpp_generic_param_abi_type(writer& w) const
{
    write_cpp_abi_param(w);
}

void interface_type::write_cpp_abi_param(writer& w) const
{
    write_cpp_abi_name(w);
    w.write('*');
}

void interface_type::write_c_forward_declaration(writer& w) const
{
    if (!w.should_forward_declare(m_mangledName))
    {
        return;
    }

    w.write(R"^-^(#ifndef __%_FWD_DEFINED__
#define __%_FWD_DEFINED__
typedef interface % %;

#endif // __%_FWD_DEFINED__

)^-^",
        bind_mangled_name_macro(*this),
        bind_mangled_name_macro(*this),
        bind_c_type_name(*this),
        bind_c_type_name(*this),
        bind_mangled_name_macro(*this));
}

void interface_type::write_c_abi_param(writer& w) const
{
    w.write("%*", bind_c_type_name(*this));
}

static void write_interface_definition(writer& w, interface_type const& type, void (*func)(writer&, interface_type const&))
{
    // Generics don't get generated definitions
    if (type.is_generic())
    {
        return;
    }

    auto contractDepth = begin_type_definition(w, type);

    w.write(R"^-^(#if !defined(__%_INTERFACE_DEFINED__)
#define __%_INTERFACE_DEFINED__
extern const __declspec(selectany) _Null_terminated_ WCHAR InterfaceName_%_%[] = L"%";
)^-^",
        bind_mangled_name_macro(type),
        bind_mangled_name_macro(type),
        bind_list("_", namespace_range{ type.clr_abi_namespace() }),
        type.cpp_abi_name(),
        type.clr_full_name());

    func(w, type);

    w.write(R"^-^(
EXTERN_C const IID %;
#endif /* !defined(__%_INTERFACE_DEFINED__) */
)^-^", bind_iid_name(type), bind_mangled_name_macro(type));

    end_type_definition(w, type, contractDepth);
}

void interface_type::write_cpp_definition(writer& w) const
{
    write_interface_definition(w, *this, &write_cpp_interface_definition<interface_type>);
}

void interface_type::write_c_definition(writer& w) const
{
    write_interface_definition(w, *this, &write_c_interface_definition<interface_type>);
}

void class_type::write_cpp_forward_declaration(writer& w) const
{
    if (!default_interface)
    {
        XLANG_ASSERT(false);
        xlang::throw_invalid("Cannot forward declare class '", m_clrFullName, "' since it has no default interface");
    }

    if (!w.should_forward_declare(m_mangledName))
    {
        return;
    }

    // We need to declare both the class as well as the default interface
    w.push_namespace(clr_logical_namespace());
    w.write("%class %;\n", indent{}, cpp_logical_name());
    w.pop_namespace();
    w.write('\n');

    default_interface->write_cpp_forward_declaration(w);
}

void class_type::write_cpp_generic_param_logical_type(writer& w) const
{
    w.write("%*", bind_cpp_fully_qualified_type(clr_logical_namespace(), cpp_logical_name()));
}

void class_type::write_cpp_generic_param_abi_type(writer& w) const
{
    if (!default_interface)
    {
        XLANG_ASSERT(false);
        xlang::throw_invalid("Class '", m_clrFullName, "' cannot be used as a generic parameter since it has no "
            "default interface");
    }

    // The second template argument may look a bit odd, but it is correct. The default interface must be an interface,
    // which won't be aggregated and this is the simplest way to write generics correctly
    w.write("%<%*, %>",
        bind_cpp_fully_qualified_type("Windows.Foundation.Internal"sv, "AggregateType"sv),
        bind_cpp_fully_qualified_type(clr_logical_namespace(), cpp_logical_name()),
        [&](writer& w) { default_interface->write_cpp_generic_param_abi_type(w); });
}

void class_type::write_cpp_abi_param(writer& w) const
{
    if (!default_interface)
    {
        XLANG_ASSERT(false);
        xlang::throw_invalid("Class '", m_clrFullName, "' cannot be used as a function argument since it has no "
            "default interface");
    }

    default_interface->write_cpp_abi_param(w);
}

void class_type::write_c_forward_declaration(writer& w) const
{
    if (!default_interface)
    {
        XLANG_ASSERT(false);
        xlang::throw_invalid("Cannot forward declare class '", m_clrFullName, "' since it has no default interface");
    }

    default_interface->write_c_forward_declaration(w);
}

void class_type::write_c_abi_param(writer& w) const
{
    if (!default_interface)
    {
        XLANG_ASSERT(false);
        xlang::throw_invalid("Class '", m_clrFullName, "' cannot be used as a function argument since it has no "
            "default interface");
    }

    default_interface->write_c_abi_param(w);
}

void class_type::write_cpp_definition(writer& w) const
{
    auto contractDepth = begin_type_definition(w, *this);

    w.write(R"^-^(#ifndef RUNTIMECLASS_%_%_DEFINED
#define RUNTIMECLASS_%_%_DEFINED
)^-^",
        bind_list("_", namespace_range{ clr_logical_namespace() }),
        cpp_logical_name(),
        bind_list("_", namespace_range{ clr_logical_namespace() }),
        cpp_logical_name());

    if (auto info = is_deprecated(); info && w.config().enable_header_deprecation)
    {
        write_deprecation_message(w, *info);
    }

    w.write(R"^-^(extern const __declspec(selectany) _Null_terminated_ WCHAR RuntimeClass_%_%[] = L"%";
#endif
)^-^",
        bind_list("_", namespace_range{ clr_logical_namespace() }),
        cpp_logical_name(),
        clr_full_name());

    end_type_definition(w, *this, contractDepth);
}

void class_type::write_c_definition(writer& w) const
{
    write_cpp_definition(w);
}

std::size_t generic_inst::push_contract_guards(writer& w) const
{
    // Follow MIDLRT's lead and only write contract guards for the generic parameters
    std::size_t result = 0;
    for (auto param : m_genericParams)
    {
        result += param->push_contract_guards(w);
    }

    return result;
}

void generic_inst::write_cpp_forward_declaration(writer& w) const
{
    if (!w.begin_declaration(m_mangledName))
    {
        return;
    }

    // First make sure that any generic requried interface/function argument/return types are declared
    for (auto dep : dependencies)
    {
        dep->write_cpp_forward_declaration(w);
    }

    // Also need to make sure that all generic parameters are declared
    for (auto param : m_genericParams)
    {
        param->write_cpp_forward_declaration(w);
    }

    auto isExperimental = is_experimental();
    if (isExperimental)
    {
        w.write("#if defined(ENABLE_WINRT_EXPERIMENTAL_TYPES)\n");
    }

    auto contractDepth = push_contract_guards(w);
    w.write('\n');

    w.write(R"^-^(#ifndef DEF_%_USE
#define DEF_%_USE
#if !defined(RO_NO_TEMPLATE_NAME)
)^-^", m_mangledName, m_mangledName);

    w.push_inline_namespace(clr_abi_namespace());

    auto const cppName = generic_type_abi_name();
    auto write_cpp_name = [&](writer& w)
    {
        w.write(cppName);
        w.write('<');

        std::string_view prefix;
        for (auto param : m_genericParams)
        {
            w.write("%", prefix);
            param->write_cpp_generic_param_logical_type(w);
            prefix = ", "sv;
        }

        w.write('>');
    };

    w.write(R"^-^(template <>
struct __declspec(uuid("%"))
% : %_impl<)^-^", bind_uuid(*this), write_cpp_name, cppName);

    std::string_view prefix;
    for (auto param : m_genericParams)
    {
        w.write("%", prefix);
        param->write_cpp_generic_param_abi_type(w);
        prefix = ", "sv;
    }

    w.write(R"^-^(>
{
    static const wchar_t* z_get_rc_name_impl()
    {
        return L"%";
    }
};
// Define a typedef for the parameterized interface specialization's mangled name.
// This allows code which uses the mangled name for the parameterized interface to access the
// correct parameterized interface specialization.
typedef % %_t;
)^-^", m_clrFullName, write_cpp_name, m_mangledName);

    if (w.config().ns_prefix_state == ns_prefix::optional)
    {
        w.write(R"^-^(#if defined(MIDL_NS_PREFIX)
#define % ABI::@::%_t
#else
#define % @::%_t
#endif // MIDL_NS_PREFIX
)^-^", m_mangledName, clr_abi_namespace(), m_mangledName, m_mangledName, clr_abi_namespace(), m_mangledName);
    }
    else
    {
        auto nsPrefix = (w.config().ns_prefix_state == ns_prefix::always) ? "ABI::"sv : "";
        w.write(R"^-^(#define % %@::%_t
)^-^", m_mangledName, nsPrefix, clr_abi_namespace(), m_mangledName);
    }

    w.pop_inline_namespace();

    w.write(R"^-^(
#endif // !defined(RO_NO_TEMPLATE_NAME)
#endif /* DEF_%_USE */

)^-^", m_mangledName);

    w.pop_contract_guards(contractDepth);
    if (isExperimental)
    {
        w.write("#endif // defined(ENABLE_WINRT_EXPERIMENTAL_TYPES)\n");
    }

    w.write('\n');
    w.end_declaration(m_mangledName);
}

void generic_inst::write_cpp_generic_param_logical_type(writer& w) const
{
    // For generic instantiations, logical name == abi name
    write_cpp_generic_param_abi_type(w);
}

void generic_inst::write_cpp_generic_param_abi_type(writer& w) const
{
    write_cpp_abi_param(w);
}

void generic_inst::write_cpp_abi_name(writer& w) const
{
    w.write(m_mangledName);
}

void generic_inst::write_cpp_abi_param(writer& w) const
{
    w.write("%*", m_mangledName);
}

void generic_inst::write_c_forward_declaration(writer& w) const
{
    if (!w.begin_declaration(m_mangledName))
    {
        if (w.should_forward_declare(m_mangledName))
        {
            w.write("typedef interface % %;\n\n", m_mangledName, m_mangledName);
        }

        return;
    }

    // Also need to make sure that all generic parameters are declared
    for (auto param : m_genericParams)
    {
        param->write_c_forward_declaration(w);
    }

    // First make sure that any generic requried interface/function argument/return types are declared
    for (auto dep : dependencies)
    {
        dep->write_c_forward_declaration(w);
    }

    auto isExperimental = is_experimental();
    if (isExperimental)
    {
        w.write("#if defined(ENABLE_WINRT_EXPERIMENTAL_TYPES)\n");
    }

    auto contractDepth = push_contract_guards(w);

    w.write(R"^-^(#if !defined(__%_INTERFACE_DEFINED__)
#define __%_INTERFACE_DEFINED__

typedef interface % %;

//  Declare the parameterized interface IID.
EXTERN_C const IID IID_%;

)^-^", m_mangledName, m_mangledName, m_mangledName, m_mangledName, m_mangledName);

    write_c_interface_definition(w, *this);

    w.write(R"^-^(
#endif // __%_INTERFACE_DEFINED__
)^-^", m_mangledName);

    w.pop_contract_guards(contractDepth);
    if (isExperimental)
    {
        w.write("#endif // defined(ENABLE_WINRT_EXPERIMENTAL_TYPES)\n");
    }

    w.write('\n');
    w.end_declaration(m_mangledName);
}

void generic_inst::write_c_abi_param(writer& w) const
{
    w.write("%*", m_mangledName);
}

element_type const& element_type::from_type(xlang::meta::reader::ElementType type)
{
    static element_type const boolean_type{ "Boolean"sv, "bool"sv, "boolean"sv, "boolean"sv, "boolean"sv, "b1"sv };
    static element_type const char_type{ "Char16"sv, "wchar_t"sv, "wchar_t"sv, "WCHAR"sv, "wchar__zt"sv, "c2"sv };
    static element_type const u1_type{ "UInt8"sv, "::byte"sv, "::byte"sv, "BYTE"sv, "byte"sv, "u1"sv };
    static element_type const i2_type{ "Int16"sv, "short"sv, "short"sv, "INT16"sv, "short"sv, "i2"sv };
    static element_type const u2_type{ "UInt16"sv, "UINT16"sv, "UINT16"sv, "UINT16"sv, "UINT16"sv, "u2"sv };
    static element_type const i4_type{ "Int32"sv, "int"sv, "int"sv, "INT32"sv, "int"sv, "i4"sv };
    static element_type const u4_type{ "UInt32"sv, "UINT32"sv, "UINT32"sv, "UINT32"sv, "UINT32"sv, "u4"sv };
    static element_type const i8_type{ "Int64"sv, "__int64"sv, "__int64"sv, "INT64"sv, "__z__zint64"sv, "i8"sv };
    static element_type const u8_type{ "UInt64"sv, "UINT64"sv, "UINT64"sv, "UINT64"sv, "UINT64"sv, "u8"sv };
    static element_type const r4_type{ "Single"sv, "float"sv, "float"sv, "FLOAT"sv, "float"sv, "f4"sv };
    static element_type const r8_type{ "Double"sv, "double"sv, "double"sv, "DOUBLE"sv, "double"sv, "f8"sv };
    static element_type const string_type{ "String"sv, "HSTRING"sv, "HSTRING"sv, "HSTRING"sv, "HSTRING"sv, "string"sv };
    static element_type const object_type{ "Object"sv, "IInspectable*"sv, "IInspectable*"sv, "IInspectable*"sv, "IInspectable"sv, "cinterface(IInspectable)"sv };

    switch (type)
    {
    case ElementType::Boolean: return boolean_type;
    case ElementType::Char: return char_type;
    case ElementType::U1: return u1_type;
    case ElementType::I2: return i2_type;
    case ElementType::U2: return u2_type;
    case ElementType::I4: return i4_type;
    case ElementType::U4: return u4_type;
    case ElementType::I8: return i8_type;
    case ElementType::U8: return u8_type;
    case ElementType::R4: return r4_type;
    case ElementType::R8: return r8_type;
    case ElementType::String: return string_type;
    case ElementType::Object: return object_type;
    default: xlang::throw_invalid("Unrecognized ElementType: ", std::to_string(static_cast<int>(type)));
    }
}

void element_type::write_cpp_generic_param_logical_type(writer& w) const
{
    w.write(m_logicalName);
}

void element_type::write_cpp_generic_param_abi_type(writer& w) const
{
    if (m_logicalName != m_abiName)
    {
        w.write("%<%, %>",
            bind_cpp_fully_qualified_type("Windows.Foundation.Internal"sv, "AggregateType"sv),
            m_logicalName,
            m_abiName);
    }
    else
    {
        w.write(m_abiName);
    }
}

void element_type::write_cpp_abi_name(writer& w) const
{
    w.write(m_cppName);
}

void element_type::write_c_abi_param(writer& w) const
{
    w.write(m_cppName);
}

system_type const& system_type::from_name(std::string_view typeName)
{
    if (typeName == "Guid"sv)
    {
        static system_type const guid_type{ "Guid"sv, "GUID"sv, "g16"sv };
        return guid_type;
    }

    XLANG_ASSERT(false);
    xlang::throw_invalid("Unknown type '", typeName, "' in System namespace");
}

void system_type::write_cpp_generic_param_logical_type(writer& w) const
{
    write_cpp_generic_param_abi_type(w);
}

void system_type::write_cpp_generic_param_abi_type(writer& w) const
{
    w.write(m_cppName);
}

void system_type::write_cpp_abi_name(writer& w) const
{
    w.write(m_cppName);
}

void system_type::write_c_abi_param(writer& w) const
{
    w.write(m_cppName);
}

mapped_type const* mapped_type::from_typedef(xlang::meta::reader::TypeDef const& type)
{
    if (type.TypeNamespace() == foundation_namespace)
    {
        if (type.TypeName() == "HResult"sv)
        {
            static mapped_type const hresult_type{ type, "HRESULT"sv, "HRESULT"sv, "struct(Windows.Foundation.HResult;i4)"sv };
            return &hresult_type;
        }
        else if (type.TypeName() == "EventRegistrationToken"sv)
        {
            static mapped_type event_token_type{ type, "EventRegistrationToken"sv, "EventRegistrationToken"sv, "struct(Windows.Foundation.EventRegistrationToken;i8)"sv };
            return &event_token_type;
        }
        else if (type.TypeName() == "AsyncStatus"sv)
        {
            static mapped_type const async_status_type{ type, "AsyncStatus"sv, "AsyncStatus"sv, "enum(Windows.Foundation.AsyncStatus;i4)"sv };
            return &async_status_type;
        }
        else if (type.TypeName() == "IAsyncInfo"sv)
        {
            static mapped_type const async_info_type{ type, "IAsyncInfo"sv, "IAsyncInfo"sv, "{00000036-0000-0000-c000-000000000046}"sv };
            return &async_info_type;
        }
    }

    return nullptr;
}

void mapped_type::write_cpp_generic_param_logical_type(writer& w) const
{
    write_cpp_generic_param_abi_type(w);
}

void mapped_type::write_cpp_generic_param_abi_type(writer& w) const
{
    switch (get_category(m_type))
    {
    case category::enum_type:
        w.write ("% ", enum_string(w));
        break;

    case category::struct_type:
        w.write("struct ");
        break;

    default:
        break; // Others don't get prefixes
    }

    write_cpp_abi_param(w);
}

void mapped_type::write_cpp_abi_name(writer& w) const
{
    w.write(m_cppName);
}

void mapped_type::write_cpp_abi_param(writer& w) const
{
    // Currently all mapped types are mapped because the underlying type is a C type
    write_c_abi_param(w);
}

void mapped_type::write_c_abi_param(writer& w) const
{
    w.write(m_cppName);

    auto typeCategory = get_category(m_type);
    if ((typeCategory == category::delegate_type) ||
        (typeCategory == category::interface_type) ||
        (typeCategory == category::class_type))
    {
        w.write('*');
    }
}
