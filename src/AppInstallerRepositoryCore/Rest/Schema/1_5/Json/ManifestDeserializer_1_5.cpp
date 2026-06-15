// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ManifestDeserializer.h"
#include <winget/JsonUtil.h>

using namespace AppInstaller::Manifest;

namespace AppInstaller::Repository::Rest::Schema::V1_5::Json
{
    namespace
    {
        // Locale
        constexpr std::string_view Icons = "Icons"sv;
        constexpr std::string_view IconUrl = "IconUrl"sv;
        constexpr std::string_view IconFileType = "IconFileType"sv;
        constexpr std::string_view IconResolution = "IconResolution"sv;
        constexpr std::string_view IconTheme = "IconTheme"sv;
        constexpr std::string_view IconSha256 = "IconSha256"sv;
    }

    std::optional<Manifest::ManifestLocalization> ManifestDeserializer::DeserializeLocale(const web::json::value& localeJsonObject) const
    {
        auto result = V1_4::Json::ManifestDeserializer::DeserializeLocale(localeJsonObject);

        if (result)
        {
            auto& locale = result.value();

            // Icons
            auto iconsNode = JSON::GetRawJsonArrayFromJsonNode(localeJsonObject, JSON::GetUtilityString(Icons));
            if (iconsNode)
            {
                std::vector<Manifest::Icon> icons;
                for (auto const& iconNode : iconsNode->get())
                {
                    Manifest::Icon iconEntry;

                    iconEntry.Url = JSON::GetRawStringValueFromJsonNode(iconNode, JSON::GetUtilityString(IconUrl)).value_or("");
                    if (iconEntry.Url.empty())
                    {
                        continue;
                    }

                    auto fileType = JSON::GetRawStringValueFromJsonNode(iconNode, JSON::GetUtilityString(IconFileType)).value_or("");
                    if (fileType.empty())
                    {
                        continue;
                    }

                    iconEntry.FileType = Manifest::ConvertToIconFileTypeEnum(fileType);
                    iconEntry.Resolution = Manifest::ConvertToIconResolutionEnum(JSON::GetRawStringValueFromJsonNode(iconNode, JSON::GetUtilityString(IconResolution)).value_or(""));
                    iconEntry.Theme = Manifest::ConvertToIconThemeEnum(JSON::GetRawStringValueFromJsonNode(iconNode, JSON::GetUtilityString(IconTheme)).value_or(""));

                    std::optional<std::string> sha256 = JSON::GetRawStringValueFromJsonNode(iconNode, JSON::GetUtilityString(IconSha256));
                    if (JSON::IsValidNonEmptyStringValue(sha256))
                    {
                        iconEntry.Sha256 = Utility::SHA256::ConvertToBytes(*sha256);
                    }

                    icons.emplace_back(std::move(iconEntry));
                }

                if (!icons.empty())
                {
                    locale.Add<Manifest::Localization::Icons>(std::move(icons));
                }
            }
        }

        return result;
    }

    Manifest::ManifestVer ManifestDeserializer::GetManifestVersion() const
    {
        return Manifest::s_ManifestVersionV1_5;
    }
}
