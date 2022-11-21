#pragma once

#include "impl/base.h"
#include "impl/cmd_reader_windows.h"

namespace xlang::cmd
{
    struct option
    {
        static constexpr uint32_t no_min = 0;
        static constexpr uint32_t no_max = std::numeric_limits<uint32_t>::max();
        
        std::string_view name;
        uint32_t min{ no_min };
        uint32_t max{ no_max };
        std::string_view arg{};
        std::string_view desc{};
    };

    struct reader
    {
        template <typename C, typename V, size_t numOptions>
        reader(C const argc, V const argv, const option(& options)[numOptions])
        {
#ifdef XLANG_DEBUG
            {
                std::set<std::string_view> unique;

                for (auto&& option : options)
                {
                    // If this assertion fails it means there are duplicate options.
                    XLANG_ASSERT(unique.insert(option.name).second);
                }
            }
#endif

            if (argc < 2)
            {
                return;
            }

            auto last{ std::end(options) };

            for (C i = 1; i < argc; ++i)
            {
                extract_option(argv[i], options, last);
            }

            for (auto&& option : options)
            {
                auto args = m_options.find(option.name);
                std::size_t const count = args == m_options.end() ? 0 : args->second.size();

                if (option.min == 0 && option.max == 0 && count > 0)
                {
                    throw_invalid("Option '", option.name, "' does not accept a value");
                }
                else if (option.max == option.min && count != option.max)
                {
                    throw_invalid("Option '", option.name, "' requires exactly ", std::to_string(option.max), " value(s)");
                }
                else if (count < option.min)
                {
                    throw_invalid("Option '", option.name, "' requires at least ", std::to_string(option.min), " value(s)");
                }
                else if (count > option.max)
                {
                    throw_invalid("Option '", option.name, "' accepts at most ", std::to_string(option.max), " value(s)");
                }
            }
        }

        explicit operator bool() const noexcept
        {
            return !m_options.empty();
        }

        bool exists(std::string_view const& name) const noexcept
        {
            return m_options.count(name);
        }

        auto const& values(std::string_view const& name) const noexcept
        {
            auto result = m_options.find(name);

            if (result == m_options.end())
            {
                static std::vector<std::string> empty{};
                return empty;
            }

            return result->second;
        }

        auto value(std::string_view const& name, std::string_view const& default_value = {}) const
        {
            auto result = m_options.find(name);

            if (result == m_options.end() || result->second.empty())
            {
                return std::string{ default_value };
            }

            return result->second.front();
        }

        template <typename F>
        auto files(std::string_view const& name, F directory_filter) const
        {
            std::set<std::string> files;

            auto add_directory = [&](auto&& path)
            {
                for (auto&& file : std::filesystem::directory_iterator(path))
                {
                    if (std::filesystem::is_regular_file(file))
                    {
                        auto filename = file.path().string();

                        if (directory_filter(filename))
                        {
                            files.insert(filename);
                        }
                    }
                }
            };

            for (auto&& path : values(name))
            {
                if (std::filesystem::is_directory(path))
                {
                    add_directory(std::filesystem::canonical(path));
                    continue;
                }

                if (std::filesystem::is_regular_file(path))
                {
                    files.insert(std::filesystem::canonical(path).string());
                    continue;
                }
#if XLANG_PLATFORM_WINDOWS
                if (path == "local")
                {
                    std::array<char, MAX_PATH> local{};
#ifdef _WIN64
                    ExpandEnvironmentStringsA("%windir%\\System32\\WinMetadata", local.data(), static_cast<DWORD>(local.size()));
#else
                    ExpandEnvironmentStringsA("%windir%\\SysNative\\WinMetadata", local.data(), static_cast<DWORD>(local.size()));
#endif
                    add_directory(local.data());
                    continue;
                }

                std::string sdk_version;

                if (path == "sdk" || path == "sdk+")
                {
                    sdk_version = impl::get_sdk_version();
                }
                else
                {
                    std::regex rx(R"(((\d+)\.(\d+)\.(\d+)\.(\d+))\+?)");
                    std::smatch match;

                    if (std::regex_match(path, match, rx))
                    {
                        sdk_version = match[1].str();
                    }
                }

                if (!sdk_version.empty())
                {
                    auto sdk_path = impl::get_sdk_path();
                    auto xml_path = sdk_path;
                    xml_path /= L"Platforms\\UAP";
                    xml_path /= sdk_version;
                    xml_path /= L"Platform.xml";

                    impl::add_files_from_xml(files, sdk_version, xml_path, sdk_path);

                    if (path.back() != '+')
                    {
                        continue;
                    }

                    for (auto&& item : std::filesystem::directory_iterator(sdk_path / L"Extension SDKs"))
                    {
                        xml_path = item.path() / sdk_version;
                        xml_path /= L"SDKManifest.xml";

                        impl::add_files_from_xml(files, sdk_version, xml_path, sdk_path);
                    }

                    continue;
                }
#endif
                throw_invalid("Path '", path, "' is not a file or directory");
            }

            return files;
        }

        auto files(std::string_view const& name) const
        {
            return files(name, [](auto&&) {return true; });
        }

    private:

        template<typename O>
        auto find(O const& options, std::string_view const& arg)
        {
            for (auto current = std::begin(options); current != std::end(options); ++current)
            {
                if (starts_with(current->name, arg))
                {
                    return current;
                }
            }

            return std::end(options);
        }

        std::map<std::string_view, std::vector<std::string>> m_options;

        template<typename O, typename L>
        void extract_option(std::string_view arg, O const& options, L& last)
        {
            if (arg[0] == '-')
            {
                arg.remove_prefix(1);
                last = find(options, arg);

                if (last == std::end(options))
                {
                    throw_invalid("Option '-", arg, "' is not supported");
                }

                m_options.try_emplace(last->name);
            }
            else if (arg[0] == '@')
            {
                arg.remove_prefix(1);
                extract_response_file(arg, options, last);
            }
            else if (last == std::end(options))
            {
                throw_invalid("Value '", arg, "' is not supported");
            }
            else
            {
                m_options[last->name].push_back(std::string{ arg });
            }
        }

        template<typename O, typename L>
        void extract_response_file(std::string_view const& arg, O const& options, L& last)
        {
            std::filesystem::path response_path{ std::string{ arg } };
            std::string extension = response_path.extension().generic_string();
            std::transform(extension.begin(), extension.end(), extension.begin(),
                [](auto c) { return static_cast<unsigned char>(::tolower(c)); });

            // Check if misuse of @ prefix, so if directory or metadata file instead of response file.
            if (is_directory(response_path) || extension == ".winmd")
            {
                throw_invalid("'@' is reserved for response files");
            }
            std::string line_buf;
            std::ifstream response_file(absolute(response_path));
            while (getline(response_file, line_buf))
            {
                size_t argc = 0;
                std::vector<std::string> argv;

                parse_command_line(line_buf.data(), argv, &argc);

                for (size_t i = 0; i < argc; i++)
                {
                    extract_option(argv[i], options, last);
                }
            }
        }

        template <typename Character>
        static void parse_command_line(Character* cmdstart, std::vector<std::string>& argv, size_t* argument_count)
        {

            std::string arg;
            bool copy_character;
            unsigned backslash_count;
            bool in_quotes;
            bool first_arg;

            Character* p = cmdstart;
            in_quotes = false;
            first_arg = true;
            *argument_count = 0;

            for (;;)
            {
                if (*p)
                {
                    while (*p == ' ' || *p == '\t')
                        ++p;
                }

                if (!first_arg)
                {
                    argv.emplace_back(arg);
                    arg.clear();
                    ++*argument_count;
                }

                if (*p == '\0')
                    break;

                for (;;)
                {
                    copy_character = true;

                    // Rules:
                    // 2N     backslashes   + " ==> N backslashes and begin/end quote
                    // 2N + 1 backslashes   + " ==> N backslashes + literal "
                    // N      backslashes       ==> N backslashes
                    backslash_count = 0;

                    while (*p == '\\')
                    {
                        ++p;
                        ++backslash_count;
                    }

                    if (*p == '"')
                    {
                        // if 2N backslashes before, start/end quote, otherwise
                        // copy literally:
                        if (backslash_count % 2 == 0)
                        {
                            if (in_quotes && p[1] == '"')
                            {
                                p++; // Double quote inside quoted string
                            }
                            else
                            {
                                // Skip first quote char and copy second:
                                copy_character = false;
                                in_quotes = !in_quotes;
                            }
                        }

                        backslash_count /= 2;
                    }

                    while (backslash_count--)
                    {
                        arg.push_back('\\');
                    }

                    if (*p == '\0' || (!in_quotes && (*p == ' ' || *p == '\t')))
                        break;

                    if (copy_character)
                    {
                        arg.push_back(*p);
                    }

                    ++p;
                }

                first_arg = false;
            }
        }
    };
}
