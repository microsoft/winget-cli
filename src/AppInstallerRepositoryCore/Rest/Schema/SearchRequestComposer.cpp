// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "SearchRequestComposer.h"
#include "Rest/Schema/1_0/Json/SearchRequestSerializer.h"
#include "Rest/Schema/1_1/Json/SearchRequestSerializer.h"

namespace AppInstaller::Repository::Rest::Schema
{
    struct SearchRequestComposer::impl
    {
        // The serializer.  We only have one lineage (1.0+) right now.
        std::unique_ptr<Rest::Schema::V1_0::Json::SearchRequestSerializer> m_serializer;
    };

    SearchRequestComposer::SearchRequestComposer(SearchRequestComposer&&) noexcept = default;
    SearchRequestComposer& SearchRequestComposer::operator=(SearchRequestComposer&&) noexcept = default;

    SearchRequestComposer::~SearchRequestComposer() = default;

    SearchRequestComposer::SearchRequestComposer(const Utility::Version& schemaVersion)
    {
        const auto& parts = schemaVersion.GetParts();
        THROW_HR_IF(E_INVALIDARG, parts.empty());

        m_pImpl = std::make_unique<impl>();

        if (parts[0].Integer == 1)
        {
            if (parts.size() == 1 || parts[1].Integer == 0)
            {
                m_pImpl->m_serializer = std::make_unique<Rest::Schema::V1_0::Json::SearchRequestSerializer>();
            }
            else
            {
                m_pImpl->m_serializer = std::make_unique<Rest::Schema::V1_1::Json::SearchRequestSerializer>();
            }
        }
        else
        {
            THROW_HR(HRESULT_FROM_WIN32(ERROR_UNSUPPORTED_TYPE));
        }
    }

    web::json::value SearchRequestComposer::Serialize(const AppInstaller::Repository::SearchRequest& searchRequest) const
    {
        return m_pImpl->m_serializer->Serialize(searchRequest);
    }
}
