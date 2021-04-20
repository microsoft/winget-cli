// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winrt/Windows.ApplicationModel.Resources.h>

#include <string>

using namespace std::string_view_literals;

namespace AppInstaller::StringResource
{

#define WINGET_WIDE_STRINGIFY_HELP(_id_) L ## _id_
#define WINGET_WIDE_STRINGIFY(_id_) WINGET_WIDE_STRINGIFY_HELP(_id_)
#define WINGET_DEFINE_RESOURCE_STRINGID(_id_) static constexpr AppInstaller::StringResource::StringId _id_ { WINGET_WIDE_STRINGIFY(#_id_) ## sv }

    // A resource identifier
    struct StringId : public std::wstring_view
    {
        explicit constexpr StringId(std::wstring_view id) : std::wstring_view(id) {}
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
