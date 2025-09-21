// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ManifestDeserializer.h"
#include "Rest/Schema/AuthenticationInfoParser.h"
#include <winget/JsonUtil.h>

using namespace AppInstaller::Manifest;

namespace AppInstaller::Repository::Rest::Schema::V1_12::Json
{
    constexpr std::string_view UninstallerSuccessCodes = "UninstallerSuccessCodes"sv;

    std::optional<Manifest::ManifestInstaller> ManifestDeserializer::DeserializeInstaller(const web::json::value& installerJsonObject) const
    {
        auto result = V1_10::Json::ManifestDeserializer::DeserializeInstaller(installerJsonObject);

        if (result)
        {
            auto& installer = result.value();

            installer.AuthInfo = ParseAuthenticationInfo(installerJsonObject, ParseAuthenticationInfoType::Installer, GetManifestVersion());

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
