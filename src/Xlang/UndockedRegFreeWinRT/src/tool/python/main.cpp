#include "pch.h"
#include "helpers.h"

#include "strings.h"
#include "settings.h"
#include "type_writers.h"
#include "code_writers.h"
#include "file_writers.h"

namespace pywinrt
{
    using namespace xlang;

    settings_type settings;

    struct usage_exception {};

    static constexpr cmd::option options[]
    {
        { "input", 0, cmd::option::no_max, "<spec>", "Windows metadata to include in projection" },
        { "output", 0, 1, "<path>", "Location of generated projection" },
        { "include", 0, cmd::option::no_max, "<prefix>", "One or more prefixes to include in projection" },
        { "exclude", 0, cmd::option::no_max, "<prefix>", "One or more prefixes to exclude from projection" },
        { "verbose", 0, 0, {}, "Show detailed progress information" },
        { "module", 0, 1, "<name>", "Name of generated projection. Defaults to winrt."},
        { "help", 0, cmd::option::no_max, {}, "Show detailed help" },
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
Py/WinRT v%
Copyright (c) Microsoft Corporation. All rights reserved.

  pywinrt.exe [options...]

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

    void process_args(int const argc, char** argv)
    {
        cmd::reader args{ argc, argv, options };

        if (!args || args.exists("help"))
        {
            throw usage_exception{};
        }

        settings.verbose = args.exists("verbose");
        settings.module = args.value("module", "winrt");
        settings.input = args.files("input", database::is_database);

        for (auto && include : args.values("include"))
        {
            settings.include.insert(include);
        }

        for (auto && exclude : args.values("exclude"))
        {
            settings.exclude.insert(exclude);
        }

        settings.output_folder = absolute(args.value("output", "output"));
        create_directories(settings.output_folder);
    }

    auto get_files_to_cache()
    {
        std::vector<std::string> files;
        files.insert(files.end(), settings.input.begin(), settings.input.end());
        return files;
    }

    bool has_projected_types(cache::namespace_members const& members)
    {
        return
            !members.interfaces.empty() ||
            !members.classes.empty() ||
            !members.enums.empty() ||
            !members.structs.empty() ||
            !members.delegates.empty();
    }

    int run(int const argc, char** argv)
    {
        int result{};
        writer w;

        try
        {
            auto start = get_start_time();
            process_args(argc, argv);
            cache c{ get_files_to_cache() };
            settings.filter = { settings.include, settings.exclude };

            if (settings.verbose)
            {
                for (auto&& file : settings.input)
                {
                    w.write("input: %\n", file);
                }

                w.write("output: %\n", settings.output_folder.string());
            }

            w.flush_to_console();

            task_group group;

            auto module_dir = settings.output_folder / settings.module;
            auto src_dir = module_dir / "src";
            create_directories(src_dir);

            group.add([&]
            {
                write_pch_h(src_dir);
                write_pch_cpp(src_dir);
                write_pybase_h(src_dir);
                write_package_dunder_init_py(module_dir);
                write_module_cpp(src_dir);
            });

            std::vector<std::string> generated_namespaces{};

            for (auto&&[ns, members] : c.namespaces())
            {
                if (!has_projected_types(members) || !settings.filter.includes(members))
                {
                    continue;
                }

                generated_namespaces.emplace_back(ns);

                auto ns_dir = module_dir;
                for (auto&& ns_segment : get_dotted_name_segments(ns))
                {
                    std::string segment{ ns_segment };
                    std::transform(segment.begin(), segment.end(), segment.begin(), [](char c) {return static_cast<char>(::tolower(c)); });
                    ns_dir /= segment;
                }
                
                create_directories(ns_dir);

                group.add([&src_dir, ns_dir, ns = ns, members = members]
                {
                    auto namespaces = write_namespace_cpp(src_dir, ns, members);
                    write_namespace_h(src_dir, ns, namespaces, members);
                    write_namespace_dunder_init_py(ns_dir, settings.module, namespaces, ns, members);
                });
            }

            group.get();

            write_setup_py(settings.output_folder, generated_namespaces);

            if (settings.verbose)
            {
                w.write("time: %ms\n", get_elapsed_time(start));
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
    return pywinrt::run(argc, argv);
}