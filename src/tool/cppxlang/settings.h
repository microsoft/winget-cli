#pragma once

namespace xlang
{
    struct settings_type
    {
        std::set<std::string> input;
        std::set<std::string> reference;

        std::string output_folder;
        bool base{};
        bool license{};
        bool brackets{};

        bool component{};
        std::string component_folder;
        std::string component_name;
        std::string component_pch;
        bool component_prefix{};
        bool component_overwrite{};
        std::string component_lib;
        bool component_opt{};

        bool verbose{};

        std::set<std::string> include;
        std::set<std::string> exclude;

        meta::reader::filter projection_filter;
        meta::reader::filter component_filter;
    };

    extern settings_type settings;
}
