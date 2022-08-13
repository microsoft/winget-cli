// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winrt/Windows.ApplicationModel.Resources.h>

#include <string>
#include <vector>
#include "LocIndependent.h"
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

            template<typename ...T>
            Utility::LocIndString operator()(T ... args) const;
        private:
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

    namespace Execution::details
    {
        // List of approved types for output, others are potentially not localized.
        template <typename T>
        struct IsApprovedForOutput : std::false_type {};

#define WINGET_CREATE_ISAPPROVEDFOROUTPUT_SPECIALIZATION(_t_) template <> struct IsApprovedForOutput<_t_> : std::true_type {};
#define WINGET_CREATE_ISAPPROVEDFOROUTPUT_TYPE(_t_) " '" #_t_ "'"
#define WINGET_CREATE_ISAPPROVEDFOROUTPUT_LIST(F) \
        /* It is assumed that single char values need not be localized, as they are matched
           ordinally or they are punctuation / other. */ \
        F(char) \
        /* Localized strings (and from an Id for one for convenience).*/ \
        F(AppInstaller::StringResource::StringId) \
        F(AppInstaller::Resource::LocString) \
        /* Strings explicitly declared as localization independent.*/ \
        F(AppInstaller::Utility::LocIndView) \
        F(AppInstaller::Utility::LocIndString) \
        /* Normalized strings come from user dataand should therefore already by localized
           by how they are chosen (or there is no localized version).*/ \
        F(AppInstaller::Utility::NormalizedString)

#define WINGET_ISAPPROVEDFOROUTPUT_ERROR  \
        "Only the following types are approved for output:" \
        WINGET_CREATE_ISAPPROVEDFOROUTPUT_LIST(WINGET_CREATE_ISAPPROVEDFOROUTPUT_TYPE)

        WINGET_CREATE_ISAPPROVEDFOROUTPUT_LIST(WINGET_CREATE_ISAPPROVEDFOROUTPUT_SPECIALIZATION)
    }

    template<typename ... T>
    Utility::LocIndString StringResource::StringId::operator()(T ... args) const
    {
        static_assert((Execution::details::IsApprovedForOutput<T>::value && ...), WINGET_ISAPPROVEDFOROUTPUT_ERROR);
        return Utility::LocIndString{ Utility::Format(Resolve(), std::forward<T>(args)...) };
    }
}

inline std::ostream& operator<<(std::ostream& out, AppInstaller::StringResource::StringId si)
{
    return (out << AppInstaller::Resource::LocString{ si });
}
