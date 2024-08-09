// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winrt/Windows.ApplicationModel.Resources.h>

#include <string>
#include <optional>
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
            // Example: out << myStringId(placeholderVal1, placeholderVal2, ...)
            template<typename ...T>
            Utility::LocIndString operator()(T ... args) const;

            // Creates a StringId that represents an empty resource string
            static StringId Empty();

        private:
            // Resolve the string ID to its corresponding localized string
            // without replacing placeholders.
            std::string Resolve() const;
        };

        inline StringId StringId::Empty() { return StringId{ {} }; }

        // Output resource identifier as localized string.
        std::ostream& operator<<(std::ostream& out, StringId si);

        // Resource string identifiers.
        struct String
        {
            WINGET_DEFINE_RESOURCE_STRINGID(PolicyEnableWinGet);
            WINGET_DEFINE_RESOURCE_STRINGID(PolicyEnableWingetSettings);
            WINGET_DEFINE_RESOURCE_STRINGID(PolicyEnableExperimentalFeatures);
            WINGET_DEFINE_RESOURCE_STRINGID(PolicyEnableLocalManifests);
            WINGET_DEFINE_RESOURCE_STRINGID(PolicyEnableHashOverride);
            WINGET_DEFINE_RESOURCE_STRINGID(PolicyEnableLocalArchiveMalwareScanOverride);
            WINGET_DEFINE_RESOURCE_STRINGID(PolicyEnableDefaultSource);
            WINGET_DEFINE_RESOURCE_STRINGID(PolicyEnableMSStoreSource);
            WINGET_DEFINE_RESOURCE_STRINGID(PolicyAdditionalSources);
            WINGET_DEFINE_RESOURCE_STRINGID(PolicyAllowedSources);
            WINGET_DEFINE_RESOURCE_STRINGID(PolicySourceAutoUpdateInterval);
            WINGET_DEFINE_RESOURCE_STRINGID(PolicyEnableBypassCertificatePinningForMicrosoftStore);
            WINGET_DEFINE_RESOURCE_STRINGID(PolicyEnableWindowsPackageManagerCommandLineInterfaces);
            WINGET_DEFINE_RESOURCE_STRINGID(PolicyEnableWinGetConfiguration);
            WINGET_DEFINE_RESOURCE_STRINGID(PolicyEnableProxyCommandLineOptions);

            WINGET_DEFINE_RESOURCE_STRINGID(SettingsWarningInvalidFieldFormat);
            WINGET_DEFINE_RESOURCE_STRINGID(SettingsWarningInvalidFieldValue);
            WINGET_DEFINE_RESOURCE_STRINGID(SettingsWarningInvalidValueFromPolicy);
            WINGET_DEFINE_RESOURCE_STRINGID(SettingsWarningLoadedBackupSettings);
            WINGET_DEFINE_RESOURCE_STRINGID(SettingsWarningParseError);
            WINGET_DEFINE_RESOURCE_STRINGID(SettingsWarningUsingDefault);

            WINGET_DEFINE_RESOURCE_STRINGID(UnknownErrorCode);
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

    namespace StringResource
    {
        // Tries to resolve a string, returning a nullopt if it cannot.
        std::optional<Resource::LocString> TryResolveString(std::wstring_view resKey);
    }

    namespace details
    {
        // List of approved types for output, others are potentially not localized.
        template <typename T>
        struct IsApprovedForOutput
        {
            static constexpr bool value = std::is_arithmetic<T>::value;
        };

#define WINGET_CREATE_ISAPPROVEDFOROUTPUT_SPECIALIZATION(_t_) \
        template <> \
        struct IsApprovedForOutput<_t_> \
        { \
            static constexpr bool value = true; \
        }

        // It is assumed that single char values need not be localized, as they are matched
        // ordinally or they are punctuation / other.
        WINGET_CREATE_ISAPPROVEDFOROUTPUT_SPECIALIZATION(char);
        // Localized strings (and from an Id for one for convenience).
        WINGET_CREATE_ISAPPROVEDFOROUTPUT_SPECIALIZATION(StringResource::StringId);
        WINGET_CREATE_ISAPPROVEDFOROUTPUT_SPECIALIZATION(Resource::LocString);
        // Strings explicitly declared as localization independent.
        WINGET_CREATE_ISAPPROVEDFOROUTPUT_SPECIALIZATION(Utility::LocIndView);
        WINGET_CREATE_ISAPPROVEDFOROUTPUT_SPECIALIZATION(Utility::LocIndString);
        // Normalized strings come from user data and should therefore already by localized
        // by how they are chosen (or there is no localized version).
        WINGET_CREATE_ISAPPROVEDFOROUTPUT_SPECIALIZATION(Utility::NormalizedString);
    }

    template<typename ... T>
    Utility::LocIndString StringResource::StringId::operator()(T ... args) const
    {
        static_assert((details::IsApprovedForOutput<std::decay_t<T>>::value && ...), "This type may not be localized, see comment for more information");
        return Utility::LocIndString{ Utility::Format(Resolve(), std::forward<T>(args)...) };
    }
}

