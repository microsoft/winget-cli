// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/winget/ManifestJSONParser.h"
#include "Rest/Schema/1_0/Json/ManifestDeserializer.h"
#include "Rest/Schema/1_1/Json/ManifestDeserializer.h"
#include "Rest/Schema/1_4/Json/ManifestDeserializer.h"
#include "Rest/Schema/1_5/Json/ManifestDeserializer.h"
#include "Rest/Schema/1_6/Json/ManifestDeserializer.h"
#include "Rest/Schema/1_7/Json/ManifestDeserializer.h"
#include "Rest/Schema/1_9/Json/ManifestDeserializer.h"
#include "Rest/Schema/1_10/Json/ManifestDeserializer.h"

namespace AppInstaller::Repository::JSON
{
    struct ManifestJSONParser::impl
    {
        // The deserializer.  We only have one lineage (1.0+) right now.
        std::unique_ptr<Rest::Schema::V1_0::Json::ManifestDeserializer> m_deserializer;
    };

    ManifestJSONParser::ManifestJSONParser(const Utility::Version& responseSchemaVersion)
    {
        const auto& parts = responseSchemaVersion.GetParts();
        THROW_HR_IF(E_INVALIDARG, parts.empty());

        m_pImpl = std::make_unique<impl>();

        if (parts[0].Integer == 1)
        {
            if (parts.size() == 1 || parts[1].Integer == 0)
            {
                m_pImpl->m_deserializer = std::make_unique<Rest::Schema::V1_0::Json::ManifestDeserializer>();
            }
            else if (parts.size() > 1 && parts[1].Integer < 4)
            {
                m_pImpl->m_deserializer = std::make_unique<Rest::Schema::V1_1::Json::ManifestDeserializer>();
            }
            else if (parts.size() > 1 && parts[1].Integer < 5)
            {
                m_pImpl->m_deserializer = std::make_unique<Rest::Schema::V1_4::Json::ManifestDeserializer>();
            }
            else if (parts.size() > 1 && parts[1].Integer < 6)
            {
                m_pImpl->m_deserializer = std::make_unique<Rest::Schema::V1_5::Json::ManifestDeserializer>();
            }
            else if (parts.size() > 1 && parts[1].Integer < 7)
            {
                m_pImpl->m_deserializer = std::make_unique<Rest::Schema::V1_6::Json::ManifestDeserializer>();
            }
            else if (parts.size() > 1 && parts[1].Integer < 9)
            {
                m_pImpl->m_deserializer = std::make_unique<Rest::Schema::V1_7::Json::ManifestDeserializer>();
            }
            else if (parts.size() > 1 && parts[1].Integer < 10)
            {
                m_pImpl->m_deserializer = std::make_unique<Rest::Schema::V1_9::Json::ManifestDeserializer>();
            }
            else
            {
                m_pImpl->m_deserializer = std::make_unique<Rest::Schema::V1_10::Json::ManifestDeserializer>();
            }
        }
        else
        {
            THROW_HR(HRESULT_FROM_WIN32(ERROR_UNSUPPORTED_TYPE));
        }
    }

    ManifestJSONParser::ManifestJSONParser(ManifestJSONParser&&) noexcept = default;
    ManifestJSONParser& ManifestJSONParser::operator=(ManifestJSONParser&&) noexcept = default;

    ManifestJSONParser::~ManifestJSONParser() = default;

    std::vector<Manifest::Manifest> ManifestJSONParser::Deserialize(const web::json::value& response) const
    {
        return m_pImpl->m_deserializer->Deserialize(response);
    }

    std::vector<Manifest::Manifest> ManifestJSONParser::DeserializeData(const web::json::value& data) const
    {
        return m_pImpl->m_deserializer->DeserializeData(data);
    }

    std::vector<Manifest::AppsAndFeaturesEntry> ManifestJSONParser::DeserializeAppsAndFeaturesEntries(const web::json::array& data) const
    {
        return m_pImpl->m_deserializer->DeserializeAppsAndFeaturesEntries(data);
    }

    std::optional<Manifest::ManifestLocalization> ManifestJSONParser::DeserializeLocale(const web::json::value& locale) const
    {
        return m_pImpl->m_deserializer->DeserializeLocale(locale);
    }

    std::optional<Manifest::InstallationMetadataInfo> ManifestJSONParser::DeserializeInstallationMetadata(const web::json::value& installationMetadata) const
    {
        return m_pImpl->m_deserializer->DeserializeInstallationMetadata(installationMetadata);
    }
}
