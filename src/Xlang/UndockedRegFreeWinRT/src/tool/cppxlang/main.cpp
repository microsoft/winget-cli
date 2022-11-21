#include "pch.h"
#include <time.h>
#include "strings.h"
#include "settings.h"
#include "type_writers.h"
#include "helpers.h"
#include "code_writers.h"
#include "component_writers.h"
#include "file_writers.h"
#include "type_writers.h"

namespace xlang
{
    settings_type settings;

    struct usage_exception {};

    static constexpr cmd::option options[]
    {
        { "input", 0, cmd::option::no_max, "<spec>", "Metadata to include in projection" },
        { "reference", 0, cmd::option::no_max, "<spec>", "Metadata to reference from projection" },
        { "output", 0, 1, "<path>", "Location of generated projection and component templates" },
        { "component", 0, 1, "[<path>]", "Generate component templates, and optional implementation" },
        { "name", 0, 1, "<name>", "Specify explicit name for component files" },
        { "verbose", 0, 0, {}, "Show detailed progress information" },
        { "overwrite", 0, 0, {}, "Overwrite generated component files" },
        { "prefix", 0, 0, {}, "Use dotted namespace convention for component files (defaults to folders)" },
        { "pch", 0, 1, "<name>", "Specify name of precompiled header file (defaults to pch.h)" },
        { "include", 0, cmd::option::no_max, "<prefix>", "One or more prefixes to include in input" },
        { "exclude", 0, cmd::option::no_max, "<prefix>", "One or more prefixes to exclude from input" },
        { "base", 0, 0, {}, "Generate base.h unconditionally" },
        { "optimize", 0, 0, {}, "Generate component projection with unified construction support" },
        { "help", 0, cmd::option::no_max, {}, "Show detailed help with examples" },
        { "library", 0, 1, "<prefix>", "Specify library prefix (defaults to winrt)" },
        { "filter" }, // One or more prefixes to include in input (same as -include)
        { "license", 0, 0 }, // Generate license comment
        { "brackets", 0, 0 }, // Use angle brackets for #includes (defaults to quotes)
    };

    static void print_usage(writer& w)
    {
        static auto printColumns = [](writer& w, std::string_view const& col1, std::string_view const& col2)
        {
            w.write_printf("  %-20s%s\n", col1.data(), col2.data());
        };

        static auto printOption = [](writer& w, cmd::option const& opt)
        {
            if(opt.desc.empty())
            {
                return;
            }
            printColumns(w, w.write_temp("-% %", opt.name, opt.arg), opt.desc);
        };

        auto format = R"(
cppxlang v%
Copyright (c) Microsoft Corporation. All rights reserved.

  cppxlang.exe [options...]

Options:

%  ^@<path>             Response file containing command line options

Where <spec> is one or more of:

  path                Path to winmd file or recursively scanned folder
  local               Local ^%WinDir^%\System32\WinMetadata folder
  sdk[+]              Current version of Windows SDK [with extensions]
  10.0.12345.0[+]     Specific version of Windows SDK [with extensions]
)";
        w.write(format, XLANG_VERSION_STRING, bind_each(printOption, options));
    }

    static void process_args(int const argc, char** argv)
    {
        cmd::reader args{ argc, argv, options };

        if (!args || args.exists("help"))
        {
            throw usage_exception{};
        }

        settings.verbose = args.exists("verbose");

        settings.input = args.files("input", database::is_database);
        settings.reference = args.files("reference", database::is_database);

        settings.component = args.exists("component");
        settings.base = args.exists("base");

        settings.license = args.exists("license");
        settings.brackets = args.exists("brackets");

        auto output_folder = canonical(args.value("output"));
        create_directories(output_folder / "xlang/impl");
        output_folder += '/';
        settings.output_folder = output_folder.string();

        for (auto && include : args.values("include"))
        {
            settings.include.insert(include);
        }

        for (auto && include : args.values("filter"))
        {
            settings.include.insert(include);
        }

        for (auto && exclude : args.values("exclude"))
        {
            settings.exclude.insert(exclude);
        }

        if (settings.component)
        {
            settings.component_overwrite = args.exists("overwrite");
            settings.component_name = args.value("name");

            if (settings.component_name.empty())
            {
                // For compatibility with C++/WinRT 1.0, the component_name defaults to the *first*
                // input, hence the use of values() here that will return the args in input order.

                auto& values = args.values("input");

                if (!values.empty())
                {
                    settings.component_name = path(values[0]).filename().replace_extension().string();
                }
            }

            settings.component_pch = args.value("pch", "pch.h");
            settings.component_prefix = args.exists("prefix");
            settings.component_lib = args.value("lib", "xlang");
            settings.component_opt = args.exists("opt");

            if (settings.component_pch == ".")
            {
                settings.component_pch.clear();
            }

            auto component = args.value("component");

            if (!component.empty())
            {
                auto component_folder = canonical(component);
                create_directories(component_folder);
                component_folder += '/';
                settings.component_folder = component_folder.string();
            }
        }
    }

    static auto get_files_to_cache()
    {
        std::vector<std::string> files;
        files.insert(files.end(), settings.input.begin(), settings.input.end());
        files.insert(files.end(), settings.reference.begin(), settings.reference.end());
        return files;
    }

    static void build_filters(cache const& c)
    {
        if (settings.reference.empty())
        {
            return;
        }

        std::set<std::string> include;

        for (auto file : settings.input)
        {
            auto db = std::find_if(c.databases().begin(), c.databases().end(), [&](auto&& db)
            {
                return db.path() == file;
            });

            for (auto&& type : db->TypeDef)
            {
                if (!type.Flags().WindowsRuntime())
                {
                    continue;
                }

                std::string full_name{ type.TypeNamespace() };
                full_name += '.';
                full_name += type.TypeName();
                include.insert(full_name);
            }
        }

        settings.projection_filter = { include, {} };

        if (settings.include.empty() && settings.exclude.empty())
        {
            settings.component_filter = { include, {} };
        }
        else
        {
            settings.component_filter = { settings.include, settings.exclude };
        }

    }

    static void remove_foundation_types(cache& c)
    {
        c.remove_type("Foundation", "DateTime");
        c.remove_type("Foundation", "EventRegistrationToken");
        c.remove_type("Foundation", "TimeSpan");
    }

    static int run(int const argc, char** argv)
    {
        int result{};
        writer w;

        try
        {
            auto start = get_start_time();
            process_args(argc, argv);
            cache c{ get_files_to_cache() };
            remove_foundation_types(c);
            build_filters(c);
            settings.base = settings.base || (!settings.component && settings.projection_filter.empty());

            if (settings.verbose)
            {
                w.write(" tool:  %\n", canonical(argv[0]).string());
                w.write(" ver:   %\n", XLANG_VERSION_STRING);

                for (auto&& file : settings.input)
                {
                    w.write(" in:    %\n", file);
                }

                for (auto&& file : settings.reference)
                {
                    w.write(" ref:   %\n", file);
                }

                w.write(" out:   %\n", settings.output_folder);

                if (!settings.component_folder.empty())
                {
                    w.write(" cout:  %\n", settings.component_folder);
                }
            }

            w.flush_to_console();
            task_group group;

            for (auto&&[ns, members] : c.namespaces())
            {
                group.add([&, &ns = ns, &members = members]
                {
                    if (!has_projected_types(members) || !settings.projection_filter.includes(members))
                    {
                        return;
                    }

                    write_namespace_0_h(ns, members);
                    write_namespace_1_h(ns, members);
                    write_namespace_2_h(ns, members, c);
                    write_namespace_h(c, ns, members);
                });
            }

            group.add([&]
            {
                if (settings.base)
                {
                    write_base_h();
                    write_coroutine_h();
                }

                if (settings.component)
                {
                    std::vector<TypeDef> classes;

                    for (auto&&[ns, members] : c.namespaces())
                    {
                        for (auto&& type : members.classes)
                        {
                            if (settings.component_filter.includes(type))
                            {
                                classes.push_back(type);
                            }
                        }
                    }

                    if (!classes.empty())
                    {
                        write_module_g_cpp(classes);

                        for (auto&& type : classes)
                        {
                            write_component_g_h(type);
                            write_component_g_cpp(type);
                            write_component_h(type);
                            write_component_cpp(type);
                        }
                    }
                }
            });

            group.get();

            if (settings.verbose)
            {
                w.write(" time:  %ms\n", get_elapsed_time(start));
            }
        }
        catch (usage_exception const&)
        {
            print_usage(w);
        }
        catch (std::exception const& e)
        {
            w.write(" error: %\n", e.what());
            result = 1;
        }

        w.flush_to_console();
        return result;
    }
}

int main(int const argc, char** argv)
{
    return xlang::run(argc, argv);
}
