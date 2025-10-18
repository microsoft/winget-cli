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
                Initialize();
                return m_details.Identifier;
            }

            SourceInformation GetInformation() override
            {
                Initialize();
                return m_information;
            }

            // Set custom header. Returns false if custom header is not supported.
            bool SetCustomHeader(std::optional<std::string> header) override
            {
                m_customHeader = header;
                return true;
            }

            void SetCaller(std::string caller) override
            {
                m_caller = std::move(caller);
            }

            void SetAuthenticationArguments(Authentication::AuthenticationArguments authArgs) override
            {
                m_authArgs = std::move(authArgs);
            }

            std::shared_ptr<ISource> Open(IProgressCallback&) override
            {
                Initialize();
                RestClient restClient = RestClient::Create(m_details.Arg, m_customHeader, m_caller, m_httpClientHelper, m_restClientInformation, m_authArgs);
                return std::make_shared<RestSource>(m_details, m_information, std::move(restClient));
            }

            void SetThreadGlobals(const std::shared_ptr<ThreadLocalStorage::ThreadGlobals>& threadGlobals) override
            {
                m_threadGlobals = threadGlobals;
            }

        private:
            void Initialize()
            {
                std::call_once(m_initializeFlag,
                    [&]()
                    {
                        m_httpClientHelper.SetPinningConfiguration(m_details.CertificatePinningConfiguration, m_threadGlobals);
                        m_restClientInformation = RestClient::GetInformation(m_details.Arg, m_customHeader, m_caller, m_httpClientHelper);

                        m_details.Identifier = m_restClientInformation.SourceIdentifier;

                        m_information.UnsupportedPackageMatchFields = m_restClientInformation.UnsupportedPackageMatchFields;
                        m_information.RequiredPackageMatchFields = m_restClientInformation.RequiredPackageMatchFields;
                        m_information.UnsupportedQueryParameters = m_restClientInformation.UnsupportedQueryParameters;
                        m_information.RequiredQueryParameters = m_restClientInformation.RequiredQueryParameters;

                        m_information.SourceAgreementsIdentifier = m_restClientInformation.SourceAgreementsIdentifier;
                        for (auto const& agreement : m_restClientInformation.SourceAgreements)
                        {
                            m_information.SourceAgreements.emplace_back(agreement.Label, agreement.Text, agreement.Url);
                        }

                        m_information.Authentication = m_restClientInformation.Authentication;
                    });
            }

            SourceDetails m_details;
            Http::HttpClientHelper m_httpClientHelper;
            SourceInformation m_information;
            Schema::IRestClient::Information m_restClientInformation;
            std::optional<std::string> m_customHeader;
            std::string m_caller;
            Authentication::AuthenticationArguments m_authArgs;
            std::once_flag m_initializeFlag;
            std::shared_ptr<ThreadLocalStorage::ThreadGlobals> m_threadGlobals;
        };

        // The base class for data that comes from a rest based source.
        struct RestSourceFactoryImpl : public ISourceFactory
        {
            std::string_view TypeName() const override final
            {
                return RestSourceFactory::Type();
            }

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
