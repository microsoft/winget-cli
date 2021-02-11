// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
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
			std::shared_ptr<ISource> Create(const SourceDetails& details, IProgressCallback& progress) override final
			{
				THROW_HR_IF(E_INVALIDARG, details.Type != RestSourceFactory::Type());

				return CreateInternal(details, progress);
			}

			virtual std::shared_ptr<ISource> CreateInternal(const SourceDetails& details, IProgressCallback& progress) = 0;

			void Add(SourceDetails& details, IProgressCallback& progress) override final
			{
				UNREFERENCED_PARAMETER(progress);

				if (details.Type.empty())
				{
					// With more than one source implementation, we will probably need to probe first
					details.Type = RestSourceFactory::Type();
					AICLI_LOG(Repo, Info, << "Initializing source type: " << details.Name << " => " << details.Type);
				}
				else
				{
					THROW_HR_IF(E_INVALIDARG, details.Type != RestSourceFactory::Type());
				}

				// Check if URL is remtoe and secure
				THROW_HR_IF(APPINSTALLER_CLI_ERROR_SOURCE_NOT_SECURE, Utility::IsUrlRemote(details.Arg) && !Utility::IsUrlSecure(details.Arg));

				AICLI_LOG(Repo, Info, << "Initializing source from: " << details.Name << " => " << details.Arg);
			}

			void Update(const SourceDetails& details, IProgressCallback& progress) override final
			{
				UNREFERENCED_PARAMETER(progress);
				THROW_HR_IF(E_INVALIDARG, details.Type != RestSourceFactory::Type());
				THROW_HR(E_NOTIMPL);
			}

			void Remove(const SourceDetails& details, IProgressCallback& progress) override final
			{
				UNREFERENCED_PARAMETER(progress);
				THROW_HR_IF(E_INVALIDARG, details.Type != RestSourceFactory::Type());
				THROW_HR(E_NOTIMPL);
			}
		};

		// Source factory
		struct RestSourcePackageFactory : public RestSourceFactoryBase
		{
			std::shared_ptr<ISource> CreateInternal(
				const SourceDetails& details, IProgressCallback& progress) override
			{
				UNREFERENCED_PARAMETER(progress);

				RestClient restClient = RestClient::RestClient();
				// TODO: Change identifier if required.
				return std::make_shared<RestSource>(details, details.Arg, std::move(restClient));
			}
		};
	}

	std::unique_ptr<ISourceFactory> RestSourceFactory::Create()
	{
		return std::make_unique<RestSourcePackageFactory>();
	}
}
