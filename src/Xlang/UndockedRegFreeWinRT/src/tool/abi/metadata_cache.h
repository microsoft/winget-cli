#pragma once

#include <map>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "meta_reader.h"
#include "sha1.h"
#include "task_group.h"
#include "types.h"
#include "type_names.h"

struct api_contract
{
    xlang::meta::reader::TypeDef type;
    std::uint32_t current_version;
};

struct category_compare
{
    bool operator()(typedef_base const& lhs, typedef_base const& rhs) const
    {
        using namespace xlang::meta::reader;
        auto leftCat = get_category(lhs.type());
        auto rightCat = get_category(rhs.type());
        if (leftCat == rightCat)
        {
            return lhs.clr_full_name() < rhs.clr_full_name();
        }

        auto category_power = [](category cat)
        {
            switch (cat)
            {
            case category::enum_type: return 0;
            case category::struct_type: return 1;
            case category::delegate_type: return 2;
            case category::interface_type: return 3;
            case category::class_type: return 4;
            default: return 100;
            }
        };
        return category_power(leftCat) < category_power(rightCat);
    }
};

struct metadata_cache;

struct type_cache
{
    metadata_cache const* cache;

    // Definitions
    std::vector<std::reference_wrapper<enum_type const>> enums;
    std::vector<std::reference_wrapper<struct_type const>> structs;
    std::vector<std::reference_wrapper<delegate_type const>> delegates;
    std::vector<std::reference_wrapper<interface_type const>> interfaces;
    std::vector<std::reference_wrapper<class_type const>> classes;

    // Dependencies
    std::set<std::string_view> dependent_namespaces;
    std::map<std::string_view, std::reference_wrapper<generic_inst const>> generic_instantiations;
    std::set<std::reference_wrapper<typedef_base const>> external_dependencies;
    std::set<std::reference_wrapper<typedef_base const>, category_compare> internal_dependencies;
};

struct namespace_cache
{
    // Definitions
    std::vector<enum_type> enums;
    std::vector<struct_type> structs;
    std::vector<delegate_type> delegates;
    std::vector<interface_type> interfaces;
    std::vector<class_type> classes;
    std::vector<api_contract> contracts;

    // Dependencies
    std::set<std::string_view> dependent_namespaces;
    std::map<std::string_view, generic_inst> generic_instantiations;
    std::set<std::reference_wrapper<typedef_base const>> type_dependencies;
};

struct metadata_cache
{
    std::map<std::string_view, namespace_cache> namespaces;

    metadata_cache(xlang::meta::reader::cache const& c);

    type_cache compile_namespaces(std::initializer_list<std::string_view> targetNamespaces);

    metadata_type const* try_find(std::string_view typeNamespace, std::string_view typeName) const
    {
        if (typeNamespace == system_namespace)
        {
            return &system_type::from_name(typeName);
        }

        auto nsItr = m_typeTable.find(typeNamespace);
        if (nsItr != m_typeTable.end())
        {
            auto nameItr = nsItr->second.find(typeName);
            if (nameItr != nsItr->second.end())
            {
                return &nameItr->second;
            }
        }

        return nullptr;
    }

    metadata_type const& find(std::string_view typeNamespace, std::string_view typeName) const
    {
        if (auto ptr = try_find(typeNamespace, typeName))
        {
            return *ptr;
        }

        xlang::throw_invalid("Could not find type '", typeName, "' in namespace '", typeNamespace, "'");
    }

private:

    void process_namespace_types(
        xlang::meta::reader::cache::namespace_members const& members,
        namespace_cache& target,
        std::map<std::string_view, metadata_type const&>& table);

    struct init_state
    {
        namespace_cache* target;
        generic_inst const* parent_generic_inst = nullptr;
    };

    void process_namespace_dependencies(namespace_cache& target);
    void process_enum_dependencies(init_state& state, enum_type& type);
    void process_struct_dependencies(init_state& state, struct_type& type);
    void process_delegate_dependencies(init_state& state, delegate_type& type);
    void process_interface_dependencies(init_state& state, interface_type& type);
    void process_class_dependencies(init_state& state, class_type& type);

    using relative_version = std::pair<std::size_t, std::uint32_t>;
    using relative_version_map = std::unordered_map<interface_type const*, relative_version>;
    void process_fastabi_required_interfaces(init_state& state, interface_type const* currentInterface, relative_version rank, relative_version_map& interfaceMap);

    function_def process_function(init_state& state, xlang::meta::reader::MethodDef const& def);

    metadata_type const& find_dependent_type(init_state& state, xlang::meta::reader::TypeSig const& type);
    metadata_type const& find_dependent_type(init_state& state, xlang::meta::reader::coded_index<xlang::meta::reader::TypeDefOrRef> const& type);
    metadata_type const& find_dependent_type(init_state& state, xlang::meta::reader::GenericTypeInstSig const& type);

    std::map<std::string_view, std::map<std::string_view, metadata_type const&>> m_typeTable;
};
