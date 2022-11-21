
namespace xlang::meta::reader
{
    struct cache
    {
        cache() = default;
        cache(cache const&) = delete;
        cache& operator=(cache const&) = delete;

        template<typename C, typename T = typename C::value_type>
        explicit cache(C const& files)
        {
            for (auto&& file : files)
            {
                auto& db = m_databases.emplace_back(file, this);

                for (auto&& type : db.TypeDef)
                {
                    if (!type.Flags().WindowsRuntime())
                    {
                        continue;
                    }

                    auto& ns = m_namespaces[type.TypeNamespace()];
                    ns.types.try_emplace(type.TypeName(), type);
                }
            }

            for (auto&&[namespace_name, members] : m_namespaces)
            {
                for (auto&&[name, type] : members.types)
                {
                    switch (get_category(type))
                    {
                    case category::interface_type:
                        members.interfaces.push_back(type);
                        continue;
                    case category::class_type:
                        if (extends_type(type, "System"sv, "Attribute"sv))
                        {
                            members.attributes.push_back(type);
                            continue;
                        }
                        members.classes.push_back(type);
                        continue;
                    case category::enum_type:
                        members.enums.push_back(type);
                        continue;
                    case category::struct_type:
                        if (get_attribute(type, "Windows.Foundation.Metadata"sv, "ApiContractAttribute"sv))
                        {
                            members.contracts.push_back(type);
                            continue;
                        }
                        members.structs.push_back(type);
                        continue;
                    case category::delegate_type:
                        members.delegates.push_back(type);
                        continue;
                    }
                }
            }
        }

        explicit cache(std::string const& file) : cache{ std::vector<std::string>{ file } }
        {
        }

        TypeDef find(std::string_view const& type_namespace, std::string_view const& type_name) const noexcept
        {
            auto ns = m_namespaces.find(type_namespace);

            if (ns == m_namespaces.end())
            {
                return {};
            }

            auto type = ns->second.types.find(type_name);

            if (type == ns->second.types.end())
            {
                return {};
            }

            return type->second;
        }

        TypeDef find(std::string_view const& type_string) const
        {
            auto pos = type_string.rfind('.');

            if (pos == std::string_view::npos)
            {
                throw_invalid("Type '", type_string, "' is missing a namespace qualifier");
            }

            return find(type_string.substr(0, pos), type_string.substr(pos + 1, type_string.size()));
        }

        TypeDef find_required(std::string_view const& type_namespace, std::string_view const& type_name) const
        {
            auto definition = find(type_namespace, type_name);
            
            if (!definition)
            {
                throw_invalid("Type '", type_namespace, ".", type_name, "' could not be found");
            }

            return definition;
        }

        TypeDef find_required(std::string_view const& type_string) const
        {
            auto pos = type_string.rfind('.');

            if (pos == std::string_view::npos)
            {
                throw_invalid("Type '", type_string, "' is missing a namespace qualifier");
            }

            return find_required(type_string.substr(0, pos), type_string.substr(pos + 1, type_string.size()));
        }

        auto const& databases() const noexcept
        {
            return m_databases;
        }

        auto const& namespaces() const noexcept
        {
            return m_namespaces;
        }

        void remove_type(std::string_view const& ns, std::string_view const& name)
        {
            auto m = m_namespaces.find(ns);
            if (m == m_namespaces.end())
            {
                return;
            }
            auto& members = m->second;

            auto remove = [&](auto&& collection, auto&& name)
            {
                auto pos = std::find_if(collection.begin(), collection.end(), [&](auto&& type)
                    {
                        return type.TypeName() == name;
                    });

                if (pos != collection.end())
                {
                    collection.erase(pos);
                }
            };

            remove(members.interfaces, name);
            remove(members.classes, name);
            remove(members.enums, name);
            remove(members.structs, name);
            remove(members.delegates, name);
        }

        struct namespace_members
        {
            std::map<std::string_view, TypeDef> types;
            std::vector<TypeDef> interfaces;
            std::vector<TypeDef> classes;
            std::vector<TypeDef> enums;
            std::vector<TypeDef> structs;
            std::vector<TypeDef> delegates;
            std::vector<TypeDef> attributes;
            std::vector<TypeDef> contracts;
        };

        using namespace_type = std::pair<std::string_view const, namespace_members> const&;

    private:

        std::list<database> m_databases;
        std::map<std::string_view, namespace_members> m_namespaces;
    };
}