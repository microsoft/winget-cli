#include "pch.h"

#include <cctype>
#include <cstring>

#include "abi_writer.h"
#include "code_writers.h"
#include "common.h"
#include "strings.h"

using namespace std::literals;
using namespace xlang;
using namespace xlang::meta::reader;
using namespace xlang::text;

void writer::push_namespace(std::string_view ns)
{
    XLANG_ASSERT(m_namespaceStack.empty());

    if (m_config.ns_prefix_state == ns_prefix::always)
    {
        write("namespace ABI {\n");
        ++m_indentation;
    }
    else if (m_config.ns_prefix_state == ns_prefix::optional)
    {
        write("ABI_NAMESPACE_BEGIN\n");
    }

    for (auto nsPart : namespace_range{ ns })
    {
        write("%namespace % {\n", indent{}, nsPart);
        m_namespaceStack.emplace_back(nsPart);
        ++m_indentation;
    }
}

void writer::push_inline_namespace(std::string_view ns)
{
    XLANG_ASSERT(m_namespaceStack.empty());

    std::string_view prefix;
    if (m_config.ns_prefix_state == ns_prefix::always)
    {
        write("namespace ABI {");
        prefix = " "sv;
    }
    else if (m_config.ns_prefix_state == ns_prefix::optional)
    {
        write("ABI_NAMESPACE_BEGIN");
        prefix = " "sv;
    }

    for (auto nsPart : namespace_range{ ns })
    {
        write("%namespace % {", prefix, nsPart);
        m_namespaceStack.emplace_back(nsPart);
        prefix = " "sv;
    }

    write('\n');
}

void writer::pop_namespace()
{
    XLANG_ASSERT(!m_namespaceStack.empty());
    XLANG_ASSERT(m_indentation >= m_namespaceStack.size()); // Otherwise mixing inline and non-inline
    while (!m_namespaceStack.empty())
    {
        --m_indentation;
        write("%} /* % */\n", indent{}, m_namespaceStack.back());
        m_namespaceStack.pop_back();
    }

    if (m_config.ns_prefix_state == ns_prefix::always)
    {
        write("} /* ABI */\n");
        --m_indentation;
    }
    else if (m_config.ns_prefix_state == ns_prefix::optional)
    {
        write("ABI_NAMESPACE_END\n");
    }

    XLANG_ASSERT(m_indentation == 0);
}

void writer::pop_inline_namespace()
{
    XLANG_ASSERT(!m_namespaceStack.empty());
    XLANG_ASSERT(m_indentation == 0); // Otherwise mixing inline and non-inline

    std::string_view prefix;
    while (!m_namespaceStack.empty())
    {
        write("%/* % */ }", prefix, m_namespaceStack.back());
        m_namespaceStack.pop_back();
        prefix = " "sv;
    }

    if (m_config.ns_prefix_state == ns_prefix::always)
    {
        write(" /* ABI */ }\n");
    }
    else if (m_config.ns_prefix_state == ns_prefix::optional)
    {
        write(" ABI_NAMESPACE_END\n");
    }
    else
    {
        write('\n');
    }
}

bool writer::push_contract_guard(version ver)
{
    if (std::holds_alternative<contract_version>(ver))
    {
        push_contract_guard(std::get<contract_version>(ver));
        return true;
    }

    return false;
}

void writer::push_contract_guard(contract_version ver)
{
    contract_history history;
    history.current_contract = ver;
    push_contract_guard(std::move(history));
}

void writer::push_contract_guard(contract_history vers)
{
    auto [ns, name] = decompose_type(vers.current_contract.type_name);
    write("#if % >= %", bind<write_contract_macro>(ns, name), format_hex{ vers.current_contract.version });
    for (auto const& prev : vers.previous_contracts)
    {
        auto [prevNs, prevName] = decompose_type(prev.from_contract);
        write(" || \\\n    % >= % && % < %",
            bind<write_contract_macro>(prevNs, prevName), format_hex{ prev.version_introduced },
            bind<write_contract_macro>(prevNs, prevName), format_hex{ prev.version_removed });
    }
    write('\n');

    m_contractGuardStack.emplace_back(std::move(vers));
}

void writer::pop_contract_guards(std::size_t count)
{
    while (count--)
    {
        auto const& vers = m_contractGuardStack.back();
        auto [ns, name] = decompose_type(vers.current_contract.type_name);
        write("#endif // % >= %", bind<write_contract_macro>(ns, name), format_hex{ vers.current_contract.version });
        for (auto const& prev : vers.previous_contracts)
        {
            auto [prevNs, prevName] = decompose_type(prev.from_contract);
            write(" || \\\n       // % >= % && % < %",
                bind<write_contract_macro>(prevNs, prevName), format_hex{ prev.version_introduced },
                bind<write_contract_macro>(prevNs, prevName), format_hex{ prev.version_removed });
        }
        write('\n');
        m_contractGuardStack.pop_back();
    }
}

static void write_include_guard(writer& w, std::string_view ns)
{
    if (w.config().lowercase_include_guard)
    {
        xlang::text::bind_list<writer::write_lowercase>("2E", namespace_range{ ns })(w);
    }
    else
    {
        xlang::text::bind_list("2E", namespace_range{ ns })(w);
    }
}

static void write_api_contract_definitions(writer& w, type_cache const& types)
{
    w.write(R"^-^(
//  API Contract Inclusion Definitions
#if !defined(SPECIFIC_API_CONTRACT_DEFINITIONS)
)^-^"sv);

    for (auto ns : types.dependent_namespaces)
    {
        auto itr = types.cache->namespaces.find(ns);
        if (itr != types.cache->namespaces.end())
        {
            for (auto const& contract : itr->second.contracts)
            {
                auto contractNamespace = contract.type.TypeNamespace();
                auto contractName = contract.type.TypeName();
                w.write(R"^-^(#if !defined(%)
#define % %
#endif // defined(%)

)^-^"sv,
                    bind<write_contract_macro>(contractNamespace, contractName),
                    bind<write_contract_macro>(contractNamespace, contractName), format_hex{ contract.current_version },
                    bind<write_contract_macro>(contractNamespace, contractName));
            }
        }
    }

    w.write(R"^-^(#endif // defined(SPECIFIC_API_CONTRACT_DEFINITIONS)


)^-^"sv);
}

static void write_includes(writer& w, type_cache const& types, std::string_view fileName)
{
    // Forced dependencies
    w.write(R"^-^(// Header files for imported files
#include "inspectable.h"
#include "AsyncInfo.h"
#include "EventToken.h"
#include "windowscontracts.h"
)^-^");

    if (fileName != foundation_namespace)
    {
        w.write(R"^-^(#include "Windows.Foundation.h"
)^-^");
    }
    else
    {
        w.write(R"^-^(#include "IVectorChangedEventArgs.h"
)^-^");
    }

    bool hasCollectionsDependency = false;
    for (auto ns : types.dependent_namespaces)
    {
        if (ns == collections_namespace)
        {
            // The collections header is hand-rolled
            hasCollectionsDependency = true;
        }
        else if (ns == foundation_namespace)
        {
            // This is a forced dependency
        }
        else if (ns == system_namespace)
        {
            // The "System" namespace a lie
        }
        else if (ns == fileName)
        {
            // Don't include ourself
        }
        else
        {
            w.write(R"^-^(#include "%.h"
)^-^", ns);
        }
    }

    if (hasCollectionsDependency)
    {
        w.write(R"^-^(// Importing Collections header
#include <windows.foundation.collections.h>
)^-^");
    }

    w.write("\n");
}

static void write_cpp_interface_forward_declarations(writer& w, type_cache const& types)
{
    w.write("/* Forward Declarations */\n");

    for (auto const& type : types.delegates)
    {
        if (!type.get().is_generic())
        {
            type.get().write_cpp_forward_declaration(w);
        }
    }

    for (auto const& type : types.interfaces)
    {
        if (!type.get().is_generic())
        {
            type.get().write_cpp_forward_declaration(w);
        }
    }
}

static void write_cpp_generic_definitions(writer& w, type_cache const& types)
{
    w.write(R"^-^(// Parameterized interface forward declarations (C++)

// Collection interface definitions
)^-^");

    for (auto const& [name, inst] : types.generic_instantiations)
    {
        inst.get().write_cpp_forward_declaration(w);
    }
}

static void write_cpp_dependency_forward_declarations(writer& w, type_cache const& types)
{
    for (auto const& type : types.external_dependencies)
    {
        type.get().write_cpp_forward_declaration(w);
    }

    for (auto const& type : types.internal_dependencies)
    {
        type.get().write_cpp_forward_declaration(w);
    }
}

static void write_cpp_type_definitions(writer& w, type_cache const& types)
{
    for (auto const& enumType : types.enums)
    {
        enumType.get().write_cpp_definition(w);
    }

    for (auto const& structType : types.structs)
    {
        structType.get().write_cpp_definition(w);
    }

    for (auto const& delegateType : types.delegates)
    {
        delegateType.get().write_cpp_definition(w);
    }

    for (auto const& interfaceType : types.interfaces)
    {
        interfaceType.get().write_cpp_definition(w);
    }

    for (auto const& classType : types.classes)
    {
        classType.get().write_cpp_definition(w);
    }
}

static void write_c_interface_forward_declarations(writer& w, type_cache const& types)
{
    w.write("/* Forward Declarations */\n");

    for (auto const& type : types.delegates)
    {
        if (!type.get().is_generic())
        {
            type.get().write_c_forward_declaration(w);
        }
    }

    for (auto const& type : types.interfaces)
    {
        if (!type.get().is_generic())
        {
            type.get().write_c_forward_declaration(w);
        }
    }
}

static void write_c_generic_definitions(writer& w, type_cache const& types)
{
    w.write(R"^-^(// Parameterized interface forward declarations (C)

// Collection interface definitions

)^-^");

    for (auto const& [name, inst] : types.generic_instantiations)
    {
        inst.get().write_c_forward_declaration(w);
    }
}

static void write_c_dependency_forward_declarations(writer& w, type_cache const& types)
{
    for (auto const& type : types.external_dependencies)
    {
        type.get().write_c_forward_declaration(w);
    }

    for (auto const& type : types.internal_dependencies)
    {
        type.get().write_c_forward_declaration(w);
    }
}

static void write_c_type_definitions(writer& w, type_cache const& types)
{
    for (auto const& enumType : types.enums)
    {
        enumType.get().write_c_definition(w);
    }

    for (auto const& structType : types.structs)
    {
        structType.get().write_c_definition(w);
    }

    for (auto const& delegateType : types.delegates)
    {
        delegateType.get().write_c_definition(w);
    }

    for (auto const& interfaceType : types.interfaces)
    {
        interfaceType.get().write_c_definition(w);
    }

    for (auto const& classType : types.classes)
    {
        classType.get().write_c_definition(w);
    }
}

void write_abi_header(std::string_view fileName, abi_configuration const& config, type_cache const& types)
{
    writer w{ config };

    // All headers begin with a bit of boilerplate
    w.write(strings::file_header);
    w.write(strings::include_guard_start,
        bind<write_include_guard>(fileName),
        bind<write_include_guard>(fileName),
        bind<write_include_guard>(fileName),
        bind<write_include_guard>(fileName));
    if (config.enable_header_deprecation)
    {
        w.write(strings::deprecated_header_start);
    }
    w.write(strings::ns_prefix_definitions,
        (config.ns_prefix_state == ns_prefix::always) ? strings::ns_prefix_always :
        (config.ns_prefix_state == ns_prefix::optional) ? strings::ns_prefix_optional : strings::ns_prefix_never);
    if (config.ns_prefix_state == ns_prefix::optional)
    {
        w.write(strings::optional_ns_prefix_definitions);
    }
    w.write(strings::constexpr_definitions);

    write_api_contract_definitions(w, types);
    write_includes(w, types, fileName);

    // C++ interface
    w.write("#if defined(__cplusplus) && !defined(CINTERFACE)\n");
    if (config.enum_class)
    {
        w.write(strings::enum_class);
    }

    write_cpp_interface_forward_declarations(w, types);
    write_cpp_generic_definitions(w, types);
    write_cpp_dependency_forward_declarations(w, types);
    write_cpp_type_definitions(w, types);

    // C interface
    w.write("#else // !defined(__cplusplus)\n");
    w.begin_c_interface();

    write_c_interface_forward_declarations(w, types);
    write_c_generic_definitions(w, types);
    write_c_dependency_forward_declarations(w, types);
    write_c_type_definitions(w, types);

    w.write("#endif // defined(__cplusplus)");

    w.write(strings::constexpr_end_definitions);
    if (config.ns_prefix_state == ns_prefix::optional)
    {
        w.write(strings::optional_ns_prefix_end_definitions);
    }
    if (config.enable_header_deprecation)
    {
        w.write(strings::deprecated_header_end);
    }
    w.write(strings::include_guard_end, bind<write_include_guard>(fileName), bind<write_include_guard>(fileName));

    auto filename{ config.output_directory };
    filename += fileName;
    filename += ".h";
    w.flush_to_file(filename);
}
