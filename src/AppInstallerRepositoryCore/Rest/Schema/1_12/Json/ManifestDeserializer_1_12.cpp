// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ManifestDeserializer.h"
#include "Rest/Schema/AuthenticationInfoParser.h"
#include <winget/JsonUtil.h>

using namespace AppInstaller::Manifest;

namespace AppInstaller::Repository::Rest::Schema::V1_12::Json
{
    namespace
    {
        // Uninstaller switches
        constexpr std::string_view UninstallerSwitches = "UninstallerSwitches"sv;
        constexpr std::string_view Silent = "Silent"sv;
        constexpr std::string_view SilentWithProgress = "SilentWithProgress"sv;
        constexpr std::string_view Interactive = "Interactive"sv;
        constexpr std::string_view Log = "Log"sv;
        constexpr std::string_view Custom = "Custom"sv;

        // Uninstaller Success Codes
        constexpr std::string_view UninstallerSuccessCodes = "UninstallerSuccessCodes"sv;

        void TryParseUninstallerSwitchField(
            std::map<UninstallerSwitchType, Utility::NormalizedString>& uninstallerSwitches,
            UninstallerSwitchType switchType,
            const web::json::value& switchesJsonObject,
            std::string_view switchJsonFieldName)
        {
            auto value = JSON::GetRawStringValueFromJsonNode(switchesJsonObject, JSON::GetUtilityString(switchJsonFieldName));

            if (JSON::IsValidNonEmptyStringValue(value))
            {
                uninstallerSwitches[switchType] = value.value();
            }
        }
    }

    std::map<Manifest::UninstallerSwitchType, Manifest::string_t> ManifestDeserializer::DeserializeUninstallerSwitches(const web::json::value& uninstallerSwitchesJsonObject) const
    {
        std::map<Manifest::UninstallerSwitchType, Manifest::string_t> uninstallerSwitches;

        TryParseUninstallerSwitchField(uninstallerSwitches, UninstallerSwitchType::Silent, uninstallerSwitchesJsonObject, Silent);
        TryParseUninstallerSwitchField(uninstallerSwitches, UninstallerSwitchType::SilentWithProgress, uninstallerSwitchesJsonObject, SilentWithProgress);
        TryParseUninstallerSwitchField(uninstallerSwitches, UninstallerSwitchType::Interactive, uninstallerSwitchesJsonObject, Interactive);
        TryParseUninstallerSwitchField(uninstallerSwitches, UninstallerSwitchType::Log, uninstallerSwitchesJsonObject, Log);
        TryParseUninstallerSwitchField(uninstallerSwitches, UninstallerSwitchType::Custom, uninstallerSwitchesJsonObject, Custom);

        return uninstallerSwitches;
    }

    std::optional<Manifest::ManifestInstaller> ManifestDeserializer::DeserializeInstaller(const web::json::value& installerJsonObject) const
    {
        auto result = V1_10::Json::ManifestDeserializer::DeserializeInstaller(installerJsonObject);

        if (result)
        {
            auto& installer = result.value();

            // Uninstaller Switches
            std::optional<std::reference_wrapper<const web::json::value>> uninstallerSwitches =
                JSON::GetJsonValueFromNode(installerJsonObject, JSON::GetUtilityString(UninstallerSwitches));
            if (uninstallerSwitches)
            {
                installer.UninstallerSwitches = DeserializeUninstallerSwitches(uninstallerSwitches.value().get());
            }

            // Uninstaller SuccessCodes
            std::optional<std::reference_wrapper<const web::json::array>> uninstallSuccessCodes = JSON::GetRawJsonArrayFromJsonNode(installerJsonObject, JSON::GetUtilityString(UninstallerSuccessCodes));
            if (uninstallSuccessCodes)
            {
                for (auto& code : uninstallSuccessCodes.value().get())
                {
                    std::optional<int> codeValue = JSON::GetRawIntValueFromJsonValue(code);
                    if (codeValue)
                    {
                        installer.UninstallerSuccessCodes.emplace_back(std::move(codeValue.value()));
                    }
                }
            }
        }

        return result;
    }

    Manifest::InstallerTypeEnum ManifestDeserializer::ConvertToInstallerType(std::string_view in) const
    {
        std::string inStrLower = Utility::ToLower(in);

        if (inStrLower == "font")
        {
            return InstallerTypeEnum::Font;
        }

        return V1_10::Json::ManifestDeserializer::ConvertToInstallerType(inStrLower);
    }

    Manifest::ManifestVer ManifestDeserializer::GetManifestVersion() const
    {
        return Manifest::s_ManifestVersionV1_12;
    }
}
