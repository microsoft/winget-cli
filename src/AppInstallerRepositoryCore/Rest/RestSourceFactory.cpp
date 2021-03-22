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
        // The base class for data that comes from a rest based source.
        struct RestSourceFactoryBase : public ISourceFactory
        {
            std::shared_ptr<ISource> Create(const SourceDetails& details, IProgressCallback&) override final
            {
                THROW_HR_IF(E_INVALIDARG, !Utility::CaseInsensitiveEquals(details.Type, RestSourceFactory::Type()));

                RestClient restClient = RestClient::Create(details.Arg);

                // TODO: Change identifier if required.
                return std::make_shared<RestSource>(details, details.Arg, std::move(restClient));
            }

            void Add(SourceDetails& details, IProgressCallback&) override final
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
            }

            void Update(const SourceDetails& details, IProgressCallback&) override final
            {
                THROW_HR_IF(E_INVALIDARG, !Utility::CaseInsensitiveEquals(details.Type, RestSourceFactory::Type()));
            }

            void Remove(const SourceDetails& details, IProgressCallback&) override final
            {
                THROW_HR_IF(E_INVALIDARG, !Utility::CaseInsensitiveEquals(details.Type, RestSourceFactory::Type()));
            }
        };
    }

    std::unique_ptr<ISourceFactory> RestSourceFactory::Create()
    {
        return std::make_unique<RestSourceFactoryBase>();
    }
}
