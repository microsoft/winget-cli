#pragma once

namespace pywinrt
{
    struct settings_type
    {
        std::set<std::string> input;

        std::filesystem::path output_folder;
        std::string module{ "pyrt" };
        bool verbose{};

        std::set<std::string> include;
        std::set<std::string> exclude;
        xlang::meta::reader::filter filter;
    };

    extern settings_type settings;
}
