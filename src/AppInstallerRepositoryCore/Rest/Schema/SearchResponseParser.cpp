// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "SearchResponseParser.h"
#include "Rest/Schema/1_0/Json/SearchResponseDeserializer.h"
#include "Rest/Schema/1_4/Json/SearchResponseDeserializer.h"

namespace AppInstaller::Repository::Rest::Schema
{
    struct SearchResponseParser::impl
    {
        // The deserializer.  We only have one lineage (1.0+) right now.
        std::unique_ptr<Rest::Schema::V1_0::Json::SearchResponseDeserializer> m_deserializer;
    };

    SearchResponseParser::SearchResponseParser(SearchResponseParser&&) noexcept = default;
    SearchResponseParser& SearchResponseParser::operator=(SearchResponseParser&&) noexcept = default;

    SearchResponseParser::~SearchResponseParser() = default;

    SearchResponseParser::SearchResponseParser(const Utility::Version& schemaVersion)
    {
        const auto& parts = schemaVersion.GetParts();
        THROW_HR_IF(E_INVALIDARG, parts.empty());

        m_pImpl = std::make_unique<impl>();

        if (parts[0].Integer == 1)
        {
            if (parts.size() == 1 || parts[1].Integer < 4)
            {
                m_pImpl->m_deserializer = std::make_unique<Rest::Schema::V1_0::Json::SearchResponseDeserializer>();
            }
            else
            {
                m_pImpl->m_deserializer = std::make_unique<Rest::Schema::V1_4::Json::SearchResponseDeserializer>();
            }
        }
        else
        {
            THROW_HR(HRESULT_FROM_WIN32(ERROR_UNSUPPORTED_TYPE));
        }
    }

    IRestClient::SearchResult SearchResponseParser::Deserialize(const web::json::value& searchResultJsonObject) const
    {
        return m_pImpl->m_deserializer->Deserialize(searchResultJsonObject);
    }
}
