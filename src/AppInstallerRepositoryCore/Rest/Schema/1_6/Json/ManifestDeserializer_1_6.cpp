// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ManifestDeserializer.h"
#include <winget/JsonUtil.h>

using namespace AppInstaller::Manifest;

namespace AppInstaller::Repository::Rest::Schema::V1_6::Json
{
    namespace
    {
        // Installer
        constexpr std::string_view DownloadCommandProhibited = "DownloadCommandProhibited"sv;

        // Locale
        constexpr std::string_view DonationUrl = "DonationUrl"sv;
    }

    std::optional<Manifest::ManifestInstaller> ManifestDeserializer::DeserializeInstaller(const web::json::value& installerJsonObject) const
    {
        auto result = V1_5::Json::ManifestDeserializer::DeserializeInstaller(installerJsonObject);

        if (result)
        {
            auto& installer = result.value();

            installer.DownloadCommandProhibited = JSON::GetRawBoolValueFromJsonNode(installerJsonObject, JSON::GetUtilityString(DownloadCommandProhibited)).value_or(false);
        }

        return result;
    }

    Manifest::UpdateBehaviorEnum ManifestDeserializer::ConvertToUpdateBehavior(std::string_view in) const
    {
        std::string inStrLower = Utility::ToLower(in);

        if (inStrLower == "deny")
        {
            return UpdateBehaviorEnum::Deny;
        }

        return V1_5::Json::ManifestDeserializer::ConvertToUpdateBehavior(inStrLower);
    }

    std::optional<Manifest::ManifestLocalization> ManifestDeserializer::DeserializeLocale(const web::json::value& localeJsonObject) const
    {
        auto result = V1_1::Json::ManifestDeserializer::DeserializeLocale(localeJsonObject);

        if (result)
        {
            auto& locale = result.value();

            TryParseStringLocaleField<Manifest::Localization::DonationUrl>(locale, localeJsonObject, DonationUrl);
        }

        return result;
    }

    Manifest::ManifestVer ManifestDeserializer::GetManifestVersion() const
    {
        return Manifest::s_ManifestVersionV1_6;
    }
}
