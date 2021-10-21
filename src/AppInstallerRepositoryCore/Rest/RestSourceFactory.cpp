// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "RestSourceFactory.h"
#include "RestClient.h"
#include "RestSource.h"

using namespace std::string_literals;
using namespace std::string_view_literals;

namespace AppInstaller::Repository::Rest
{
    namespace
    {
        struct RestSourceReference : public ISourceReference
        {
            RestSourceReference(const SourceDetails& details) : m_details(details) {}

            SourceDetails& GetDetails() override { return m_details; };

            std::string GetIdentifier() override
            {
                if (!m_restClient)
                {
                    Initialize();
                }
                return m_identifier;
            }

            SourceInformation GetInformation() override
            {
                if (!m_restClient)
                {
                    Initialize();
                }
                return m_information;
            }

            // Set custom header. Returns false if custom header is not supported.
            bool SetCustomHeader(std::optional<std::string> header) override
            {
                m_customHeader = header;
                return true;
            }

            std::shared_ptr<ISource> Open(IProgressCallback&) override
            {
                if (!m_restClient)
                {
                    Initialize();
                }

                RestClient restClient = RestClient::Create(m_details.Arg, m_customHeader);
                return std::make_shared<RestSource>(m_details, m_identifier, m_information, std::move(restClient));
            }

        private:
            void Initialize()
            {
                std::call_once(m_initializeFlag,
                    [&]()
                    {
                        m_restClient = RestClient::Create(m_details.Arg, m_customHeader);

                        m_identifier = m_restClient->GetSourceIdentifier();

                        const auto& sourceInformation = m_restClient->GetSourceInformation();
                        m_information.UnsupportedPackageMatchFields = sourceInformation.UnsupportedPackageMatchFields;
                        m_information.RequiredPackageMatchFields = sourceInformation.RequiredPackageMatchFields;
                        m_information.UnsupportedQueryParameters = sourceInformation.UnsupportedQueryParameters;
                        m_information.RequiredQueryParameters = sourceInformation.RequiredQueryParameters;

                        m_information.SourceAgreementsIdentifier = sourceInformation.SourceAgreementsIdentifier;
                        for (auto const& agreement : sourceInformation.SourceAgreements)
                        {
                            m_information.SourceAgreements.emplace_back(agreement.Label, agreement.Text, agreement.Url);
                        }
                    });
            }

            std::string m_identifier;
            SourceDetails m_details;
            SourceInformation m_information;
            std::optional<std::string> m_customHeader;
            std::optional<RestClient> m_restClient;
            std::once_flag m_initializeFlag;
        };

        // The base class for data that comes from a rest based source.
        struct RestSourceFactoryImpl : public ISourceFactory
        {
            std::shared_ptr<ISourceReference> Create(const SourceDetails& details) override final
            {
                THROW_HR_IF(E_INVALIDARG, !Utility::CaseInsensitiveEquals(details.Type, RestSourceFactory::Type()));

                return std::make_shared<RestSourceReference>(details);
            }

            bool Add(SourceDetails& details, IProgressCallback&) override final
            {
                if (details.Type.empty())
                {
                    details.Type = RestSourceFactory::Type();
                }
                else
                {
                    THROW_HR_IF(E_INVALIDARG, !Utility::CaseInsensitiveEquals(details.Type, RestSourceFactory::Type()));
                }

                // Check if URL is remote and secure
                THROW_HR_IF(APPINSTALLER_CLI_ERROR_SOURCE_NOT_REMOTE, !Utility::IsUrlRemote(details.Arg));
                THROW_HR_IF(APPINSTALLER_CLI_ERROR_SOURCE_NOT_SECURE, !Utility::IsUrlSecure(details.Arg));

                return true;
            }

            bool Update(const SourceDetails& details, IProgressCallback&) override final
            {
                THROW_HR_IF(E_INVALIDARG, !Utility::CaseInsensitiveEquals(details.Type, RestSourceFactory::Type()));
                return true;
            }

            bool Remove(const SourceDetails& details, IProgressCallback&) override final
            {
                THROW_HR_IF(E_INVALIDARG, !Utility::CaseInsensitiveEquals(details.Type, RestSourceFactory::Type()));
                return true;
            }
        };
    }

    std::unique_ptr<ISourceFactory> RestSourceFactory::Create()
    {
        return std::make_unique<RestSourceFactoryImpl>();
    }
}
