// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "pch.h"
#include "RestPackageSourceFactory.h"
#include "RestSource.h"
#include "Microsoft/SQLiteIndex.h"
#include "Microsoft/SQLiteIndexSource.h"

using namespace std::string_literals;
using namespace std::string_view_literals;

namespace AppInstaller::Repository::Rest
{
	namespace
	{
		static constexpr std::string_view s_PreIndexedPackageSourceFactory_PackageFileName = "source.msix"sv;
		static constexpr std::string_view s_PreIndexedPackageSourceFactory_AppxManifestFileName = "AppxManifest.xml"sv;
		static constexpr std::string_view s_PreIndexedPackageSourceFactory_IndexFileName = "index.db"sv;
		// TODO: This being hard coded to force using the Public directory name is not ideal.
		static constexpr std::string_view s_PreIndexedPackageSourceFactory_IndexFilePath = "Public\\index.db"sv;

		// Construct the package location from the given details.
		// Currently expects that the arg is an https uri pointing to the root of the data.
		std::string GetPackageLocation(const SourceDetails& details)
		{
			THROW_HR_IF(E_INVALIDARG, details.Arg.empty());
			std::string result = details.Arg;
			if (result.back() != '/')
			{
				result += '/';
			}
			return result;
		}

		// The base class for a package that comes from a Rest packaged source.
		struct RestFactoryBase : public ISourceFactory
		{
			std::shared_ptr<ISource> Create(const SourceDetails& details, IProgressCallback& progress) override final
			{
				UNREFERENCED_PARAMETER(progress);

				THROW_HR_IF(E_INVALIDARG, details.Type != RestPackageSourceFactory::Type());

				return CreateInternal(details, "exampleidentifier");
			}

			virtual std::shared_ptr<ISource> CreateInternal(const SourceDetails& details, std::string identifier) = 0;

			void Add(SourceDetails& details, IProgressCallback& progress) override final
			{
				UNREFERENCED_PARAMETER(progress);

				if (details.Type.empty())
				{
					// With more than one source implementation, we will probably need to probe first
					details.Type = RestPackageSourceFactory::Type();
					AICLI_LOG(Repo, Info, << "Initializing source type: " << details.Name << " => " << details.Type);
				}
				else
				{
					THROW_HR_IF(E_INVALIDARG, details.Type != RestPackageSourceFactory::Type());
				}

				std::string packageLocation = GetPackageLocation(details);
			}

			void Update(const SourceDetails& details, IProgressCallback& progress) override final
			{
				UNREFERENCED_PARAMETER(progress);

				THROW_HR_IF(E_INVALIDARG, details.Type != RestPackageSourceFactory::Type());
			}

			void Remove(const SourceDetails& details, IProgressCallback& progress) override final
			{
				UNREFERENCED_PARAMETER(progress);

				THROW_HR_IF(E_INVALIDARG, details.Type != RestPackageSourceFactory::Type());
			}

			//void Add(SourceDetails& details, IProgressCallback& progress) override final
			//{
			//	if (details.Type.empty())
			//	{
			//		// With more than one source implementation, we will probably need to probe first
			//		details.Type = RestPackageSourceFactory::Type();
			//		AICLI_LOG(Repo, Info, << "Initializing source type: " << details.Name << " => " << details.Type);
			//	}
			//	else
			//	{
			//		THROW_HR_IF(E_INVALIDARG, details.Type != RestPackageSourceFactory::Type());
			//	}

			//	std::string packageLocation = GetPackageLocation(details);

			//	THROW_HR_IF(APPINSTALLER_CLI_ERROR_SOURCE_NOT_SECURE, Utility::IsUrlRemote(packageLocation) && !Utility::IsUrlSecure(packageLocation));

			//	AICLI_LOG(Repo, Info, << "Initializing source from: " << details.Name << " => " << packageLocation);
			//}

			/*void Remove(const SourceDetails& details, IProgressCallback& progress) override final
			{
				THROW_HR_IF(E_INVALIDARG, details.Type != RestPackageSourceFactory::Type());

				RemoveInternal(details, progress);
			}*/
		};

		// Source factory for running within a packaged context
		struct RestContextFactory : public RestFactoryBase
		{
			std::shared_ptr<ISource> CreateInternal(
				const SourceDetails& details, std::string identifier) 
			{
				return std::make_shared<RestSource>(details, identifier);
			}

			//void RemoveInternal(const SourceDetails& details, IProgressCallback& callback) override
			//{
			//	// AICLI_LOG(Repo, Info, << "Removing package: " << *fullName);
			//	// Deployment::RemovePackage(*fullName, callback);
			//}
		};
	}

	std::unique_ptr<ISourceFactory> RestPackageSourceFactory::Create()
	{
		return std::make_unique<RestContextFactory>();
	}
}
