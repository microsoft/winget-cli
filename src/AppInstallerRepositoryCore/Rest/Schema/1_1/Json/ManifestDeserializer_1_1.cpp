// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ManifestDeserializer.h"
#include "Rest/Schema/JsonHelper.h"

using namespace AppInstaller::Manifest;

namespace AppInstaller::Repository::Rest::Schema::V1_1::Json
{
    namespace
    {
        // Installer
        constexpr std::string_view MSStoreProductIdentifier = "MSStoreProductIdentifier"sv;
    }

    Manifest::InstallerTypeEnum ManifestDeserializer::ConvertToInstallerType(std::string_view in) const
    {
        std::string inStrLower = Utility::ToLower(in);

        if (inStrLower == "msstore")
        {
            return InstallerTypeEnum::MSStore;
        }

        return V1_0::Json::ManifestDeserializer::ConvertToInstallerType(in);
    }

    std::optional<Manifest::ManifestInstaller> ManifestDeserializer::DeserializeInstaller(const web::json::value& installerJsonObject) const
    {
        auto result = V1_0::Json::ManifestDeserializer::DeserializeInstaller(installerJsonObject);

        if (result)
        {
            auto& installer = result.value();

            installer.ProductId = JsonHelper::GetRawStringValueFromJsonNode(installerJsonObject, JsonHelper::GetUtilityString(MSStoreProductIdentifier)).value_or("");
        }

        return result;
    }
}
