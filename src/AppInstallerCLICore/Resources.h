// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <string>
#include <winrt/Windows.ApplicationModel.Resources.h>


namespace AppInstaller::CLI::Resource
{
    using namespace std::string_view_literals;

#define WINGET_WIDE_STRINGIFY_HELP(_id_) L ## _id_
#define WINGET_WIDE_STRINGIFY(_id_) WINGET_WIDE_STRINGIFY_HELP(_id_)
#define WINGET_DEFINE_RESOURCE_ID(_id_) static constexpr std::wstring_view _id_ = WINGET_WIDE_STRINGIFY(#_id_) ## sv

    // Resource string identifiers
    struct String
    {
        static constexpr std::wstring_view SearchCommandDescription = L"SearchCommandDescription"sv;
        WINGET_DEFINE_RESOURCE_ID(HashHelperDescription);
    };

    // A localized string
    struct LocString : public std::string
    {
        LocString(std::wstring_view id);
    };

    // Utility class to load resources
    class Loader
    {
    public:
        // Gets the singleton instance of the resource loader.
        static const Loader& Instance();

        // Gets the the string resource value.
        std::string ResolveString(std::wstring_view resKey) const;

    private:
        winrt::Windows::ApplicationModel::Resources::ResourceLoader m_wingetLoader;

        Loader();
    };
}