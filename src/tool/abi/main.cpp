#include "pch.h"

#include "abi_writer.h"
#include "common.h"
#include "metadata_cache.h"
#include "strings.h"

using namespace std::chrono;
using namespace std::filesystem;
using namespace std::literals;
using namespace xlang;
using namespace xlang::meta::reader;
using namespace xlang::text;
using namespace xlang::cmd;

auto output_directory(reader const& args)
{
    auto out{ absolute(args.value("output", "abi")) };
    create_directories(out);
    out += path::preferred_separator;
    return out.string();
}

struct usage_exception {};

static constexpr option options[]
{
    { "input", 0, option::no_max, "<spec>", "Windows metadata to include in generation" },
    { "reference", 0, option::no_max, "<spec>", "Windows metadata to reference from generation" },
    { "output", 0, 1, "<path>", "Location of generated headers" },
    { "include", 0, option::no_max, "<prefix>", "One or more prefixes to include in input" },
    { "exclude", 0, option::no_max, "<prefix>", "One or more prefixes to exclude from input" },
    { "verbose", 0, 0, {}, "Show detailed progress information" },
    { "ns-prefix", 0, 1, "<always|optional|never>", "Sets policy for prefixing type names with 'ABI' namespace (default: never)" },
    { "enum-class", 0, 0, {}, "Use 'MIDL_ENUM', rather than 'enum'" },
    { "lowercase-include-guard", 0, 0, {}, "Generate lowercase include guards for compatibility with Windows SDK headers" },
    { "enable-header-deprecation", 0, 0, {}, "Generate support for [[deprecated(...)]] attribute" },
    { "help", 0, option::no_max, {}, "Show detailed help with examples" },
};

static void print_usage(basic_writer& w)
{
    static auto printColumns = [](basic_writer& w, std::string_view const& col1, std::string_view const& col2)
    {
        w.write_printf("  %-40s%s\n", col1.data(), col2.data());
    };

    static auto printOption = [](basic_writer& w, option const& opt)
    {
        if(opt.desc.empty())
        {
            return;
        }
        printColumns(w, w.write_temp("-% %", opt.name, opt.arg), opt.desc);
    };

    auto format = R"(
WinRT ABI Header Generation Tool v%
Copyright (c) Microsoft Corporation. All rights reserved.

  abi.exe [options...]

Options:

%  ^@<path>                                 Response file containing command line options

Where <spec> is one or more of:

  path                                    Path to winmd file or recursively scanned folder
  local                                   Local ^%WinDir^%\System32\WinMetadata folder
  sdk[+]                                  Current version of Windows SDK [with extensions]
  10.0.12345.0[+]                         Specific version of Windows SDK [with extensions]
)";
    w.write(format, ABIWINRT_VERSION_STRING, bind_each(printOption, options));
}

int main(int const argc, char** argv)
{
    int exitCode = 0;
    basic_writer w;

    try
    {
        auto const start = high_resolution_clock::now();

        reader args{ argc, argv, options };
        
        if (!args || args.exists("help"))
        {
            throw usage_exception{};
        }
        
        abi_configuration config;
        config.verbose = args.exists("verbose");
        config.output_directory = output_directory(args);
        config.enum_class = args.exists("enum-class");
        config.lowercase_include_guard = args.exists("lowercase-include-guard");
        config.enable_header_deprecation = args.exists("enable-header-deprecation");

        if (args.exists("ns-prefix"))
        {
            auto const& values = args.values("ns-prefix");
            if (values.empty())
            {
                config.ns_prefix_state = ns_prefix::always;
            }
            else
            {
                auto const& value = values[0];
                if (value == "always")
                {
                    config.ns_prefix_state = ns_prefix::always;
                }
                else if (value == "never")
                {
                    config.ns_prefix_state = ns_prefix::never;
                }
                else if (value == "optional")
                {
                    config.ns_prefix_state = ns_prefix::optional;
                }
                else
                {
                    throw_invalid("'" + value + "' is not a valid argument for 'ns-prefix'");
                }
            }
        }
        else
        {
            config.ns_prefix_state = ns_prefix::never;
        }

        auto inputFiles = args.files("input");
        auto referenceFiles = args.files("reference");

        if (config.verbose)
        {
            for (auto const& in : inputFiles)
            {
                w.write("in: %\n", in);
            }

            for (auto const& ref : referenceFiles)
            {
                w.write("ref: %\n", ref);
            }

            w.write("out: %\n", config.output_directory);
        }
        w.flush_to_console();

        std::vector<std::string> filesToRead;
        filesToRead.reserve(inputFiles.size() + referenceFiles.size());
        filesToRead.insert(filesToRead.end(), inputFiles.begin(), inputFiles.end());
        filesToRead.insert(filesToRead.end(), referenceFiles.begin(), referenceFiles.end());

        cache c{ filesToRead };
        metadata_cache mdCache{ c };

        auto include = args.values("include");
        if (include.empty() && !referenceFiles.empty())
        {
            for (auto const& db : c.databases())
            {
                if (std::find_if(inputFiles.begin(), inputFiles.end(), [&](auto const& file)
                    {
                        return db.path() == file;
                    }) != inputFiles.end())
                {
                    for (auto const& type : db.TypeDef)
                    {
                        if (!type.Flags().WindowsRuntime())
                        {
                            continue;
                        }

                        include.push_back(std::string{ type.TypeNamespace() });
                    }
                }
            }
        }

        filter f{ include, args.values("exclude") };
        task_group group;
        auto filter_includes = [&](namespace_cache const& types)
        {
            auto includes = [&](auto const& vector)
            {
                return std::find_if(vector.begin(), vector.end(), [&](auto const& t)
                    {
                        return f.includes(t.type());
                    }) != vector.end();
            };
            if (includes(types.enums) || includes(types.structs) || includes(types.delegates) ||
                includes(types.interfaces) || includes(types.classes))
            {
                return true;
            }

            for (auto const& contract : types.contracts)
            {
                if (f.includes(contract.type))
                {
                    return true;
                }
            }
            return false;
        };

        bool foundationDependency = false;
        for (auto const& [ns, nsTypes] : mdCache.namespaces)
        {
            // Headers are all or nothing. If the consumer is wanting one type in a namespace, they get everything
            if (filter_includes(nsTypes))
            {
                if ((ns == foundation_namespace) || (ns == collections_namespace))
                {
                    foundationDependency = true;
                }
                else
                {
                    group.add([&, ns = ns]()
                    {
                        write_abi_header(ns, config, mdCache.compile_namespaces({ ns }));
                    });
                }
            }
        }

        if (foundationDependency)
        {
            group.add([&]()
            {
                // Write the 'Windows.Foundation.h' header. This is a merge of the 'Windows.Foundation' and the
                // 'Windows.Foundation.Collections' namespacess
                auto foundationItr = mdCache.namespaces.find(foundation_namespace);
                auto collectionsItr = mdCache.namespaces.find(collections_namespace);
                if (foundationItr == mdCache.namespaces.end())
                {
                    XLANG_ASSERT(false);
                    w.write("WARNING: Dependency on the 'Windows.Foundation.Collections' identified, but the "
                        "'Windows.Foundation' namespace could not be found. Skipping...");
                    w.flush_to_console();
                }
                else if (collectionsItr == mdCache.namespaces.end())
                {
                    XLANG_ASSERT(false);
                    w.write("WARNING: Dependency on the 'Windows.Foundation' identified, but the "
                        "'Windows.Foundation.Collections' namespace could not be found. Skipping...");
                    w.flush_to_console();
                }
                else
                {
                    auto types = mdCache.compile_namespaces({ foundation_namespace, collections_namespace });
                    write_abi_header(foundation_namespace, config, types);
                }
            });
        }

        group.get();

        if (config.verbose)
        {
            w.write("time: %ms\n", static_cast<std::int64_t>(duration_cast<milliseconds>((high_resolution_clock::now() - start)).count()));
        }
    }
    catch (usage_exception const&)
    {
        print_usage(w);
        exitCode = 1;
    }
    catch (std::exception const& e)
    {
        exitCode = 1;
        w.write("%\n", e.what());
    }

    w.flush_to_console();
    return exitCode;
}
