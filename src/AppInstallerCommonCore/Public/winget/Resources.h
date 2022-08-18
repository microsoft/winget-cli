// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winrt/Windows.ApplicationModel.Resources.h>

#include <string>
#include <vector>
#include "AppInstallerStrings.h"

using namespace std::string_view_literals;

namespace AppInstaller
{
    namespace StringResource
    {
#define WINGET_WIDE_STRINGIFY_HELP(_id_) L ## _id_
#define WINGET_WIDE_STRINGIFY(_id_) WINGET_WIDE_STRINGIFY_HELP(_id_)
#define WINGET_DEFINE_RESOURCE_STRINGID(_id_) static constexpr AppInstaller::StringResource::StringId _id_ { WINGET_WIDE_STRINGIFY(#_id_) ## sv }

        // A resource identifier
        struct StringId : public std::wstring_view
        {
            explicit constexpr StringId(std::wstring_view id) : std::wstring_view(id) {}

            // Sets the placeholder values in the resolved string id.
            template<typename ...T>
            Utility::LocIndString operator()(T ... args) const;

            friend std::ostream& operator<<(std::ostream& out, StringId si);
        private:

            // Resolve the string id.
            std::string Resolve() const;
        };

        // Resource string identifiers.
        struct String
        {
            WINGET_DEFINE_RESOURCE_STRINGID(PolicyEnableWinGet);
            WINGET_DEFINE_RESOURCE_STRINGID(PolicyEnableWingetSettings);
            WINGET_DEFINE_RESOURCE_STRINGID(PolicyEnableExperimentalFeatures);
            WINGET_DEFINE_RESOURCE_STRINGID(PolicyEnableLocalManifests);
            WINGET_DEFINE_RESOURCE_STRINGID(PolicyEnableHashOverride);
            WINGET_DEFINE_RESOURCE_STRINGID(PolicyEnableDefaultSource);
            WINGET_DEFINE_RESOURCE_STRINGID(PolicyEnableMSStoreSource);
            WINGET_DEFINE_RESOURCE_STRINGID(PolicyAdditionalSources);
            WINGET_DEFINE_RESOURCE_STRINGID(PolicyAllowedSources);
            WINGET_DEFINE_RESOURCE_STRINGID(PolicySourceAutoUpdateInterval);

            WINGET_DEFINE_RESOURCE_STRINGID(SettingsWarningInvalidFieldFormat);
            WINGET_DEFINE_RESOURCE_STRINGID(SettingsWarningInvalidFieldValue);
            WINGET_DEFINE_RESOURCE_STRINGID(SettingsWarningInvalidValueFromPolicy);
            WINGET_DEFINE_RESOURCE_STRINGID(SettingsWarningLoadedBackupSettings);
            WINGET_DEFINE_RESOURCE_STRINGID(SettingsWarningParseError);
        };
    }

    namespace Resource
    {
        // Get an embedded resource from the binary and return as std::string_view.
        // Resource data is valid as long as the binary is loaded.
        std::string_view GetResourceAsString(int resourceName, int resourceType);
        std::string_view GetResourceAsString(PCWSTR resourceName, PCWSTR resourceType);

        // Get an embedded resource from the binary and return as std::pair<BYTE*, size_t>.
        // Resource data is valid as long as the binary is loaded.
        std::pair<const BYTE*, size_t> GetResourceAsBytes(int resourceName, int resourceType);
        std::pair<const BYTE*, size_t> GetResourceAsBytes(PCWSTR resourceName, PCWSTR resourceType);

        struct ResourceOpenException : std::exception
        {
            ResourceOpenException(const winrt::hresult_error& hre);

            const char* what() const noexcept override { return m_message.c_str(); }

        private:
            std::string m_message;
        };

        // TODO https://task.ms/40934960 Consider renaming or removing LocString
        // A localized string
        struct LocString : public Utility::LocIndString
        {
            LocString() = default;

            LocString(StringResource::StringId id) : Utility::LocIndString(id()) {}
            LocString(Utility::LocIndString locIndString) : Utility::LocIndString(std::move(locIndString)) {}

            LocString(const LocString&) = default;
            LocString& operator=(const LocString&) = default;

            LocString(LocString&&) = default;
            LocString& operator=(LocString&&) = default;
        };
    }
}

