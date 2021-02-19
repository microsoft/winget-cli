// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "RestSource.h"

using namespace AppInstaller::Utility;

namespace AppInstaller::Repository::Rest
{
	namespace
	{
		using namespace AppInstaller::Repository::Rest::Schema;

		// The source reference used by package objects.
		struct SourceReference
		{
			SourceReference(const std::shared_ptr<const RestSource>& source) :
				m_source(source) {}

		protected:
			std::shared_ptr<const RestSource> GetReferenceSource() const
			{
				std::shared_ptr<const RestSource> source = m_source.lock();
				THROW_HR_IF(E_NOT_VALID_STATE, !source);
				return source;
			}

		private:
			std::weak_ptr<const RestSource> m_source;
		};

		// The IPackageVersion impl for RestSource.
		struct PackageVersion : public SourceReference, public IPackageVersion
		{
			// TODO: Use Manifest class instead of string.
			PackageVersion(
				const std::shared_ptr<const RestSource>& source, IRestClient::PackageInfo packageInfo, VersionAndChannel versionInfo)
				: SourceReference(source), m_packageInfo(packageInfo), m_versionInfo(versionInfo) {}

			// Inherited via IPackageVersion
			Utility::LocIndString GetProperty(PackageVersionProperty property) const override
			{
				switch (property)
				{
				case PackageVersionProperty::SourceIdentifier:
					return Utility::LocIndString{ GetReferenceSource()->GetIdentifier() };
				case PackageVersionProperty::SourceName:
					return Utility::LocIndString{ GetReferenceSource()->GetDetails().Name };
				case PackageVersionProperty::Id:
					return Utility::LocIndString{ m_packageInfo.packageIdentifier };
				case PackageVersionProperty::Name:
					return Utility::LocIndString{ m_packageInfo.packageName };
				case PackageVersionProperty::Version:
					return Utility::LocIndString{ m_versionInfo.GetVersion().ToString() };
				case PackageVersionProperty::Channel:
					return Utility::LocIndString{ m_versionInfo.GetChannel().ToString() };
				default:
					return Utility::LocIndString{};
				}
			}

			// TODO
			std::vector<Utility::LocIndString> GetMultiProperty(PackageVersionMultiProperty property) const override
			{
				UNREFERENCED_PARAMETER(property);
				std::vector<Utility::LocIndString> result;
				return result;
			}

			// TODO
			Manifest::Manifest GetManifest() const override
			{
				AICLI_LOG(Repo, Info, << "Downloading manifest");
				std::string manifest = GetReferenceSource()->GetRestClient().GetManifestByVersion(
					m_packageInfo.packageIdentifier, m_versionInfo.GetVersion().ToString()).value();

				// TODO: Create Manifest object from result using JSON parser.
				THROW_HR(ERROR_CALL_NOT_IMPLEMENTED);
			}

			std::shared_ptr<const ISource> GetSource() const override
			{
				return GetReferenceSource();
			}

			// TODO
			IPackageVersion::Metadata GetMetadata() const override
			{
				IPackageVersion::Metadata result;
				return result;
			}

		private:
			IRestClient::PackageInfo m_packageInfo;
			Utility::VersionAndChannel m_versionInfo;
		};

		// The base for IPackage implementations here.
		struct PackageBase : public SourceReference
		{
			PackageBase(const std::shared_ptr<const RestSource>& source, const IRestClient::Package& package) :
				SourceReference(source), m_package(package) {}

			Utility::LocIndString GetProperty(PackageProperty property) const
			{
				switch (property)
				{
				case PackageProperty::Id:
					return Utility::LocIndString{ m_package.packageInfo.packageIdentifier };
				case PackageProperty::Name:
					return Utility::LocIndString{ m_package.packageInfo.packageName };
				default:
					THROW_HR(E_UNEXPECTED);
				}
			}

		protected:
			std::shared_ptr<IPackageVersion> GetLatestVersionInternal() const
			{
				VersionAndChannel latestVersion = m_package.versions.back();
				return std::make_shared<PackageVersion>(GetReferenceSource(), m_package.packageInfo, latestVersion);
			}

			IRestClient::Package m_package;
		};

		// The IPackage impl for Available packages from RestSource.
		struct AvailablePackage : public PackageBase, public IPackage
		{
			using PackageBase::PackageBase;

			// Inherited via IPackage
			Utility::LocIndString GetProperty(PackageProperty property) const override
			{
				return PackageBase::GetProperty(property);
			}

			std::shared_ptr<IPackageVersion> GetInstalledVersion() const override
			{
				return {};
			}

			std::vector<PackageVersionKey> GetAvailableVersionKeys() const override
			{
				std::shared_ptr<const RestSource> source = GetReferenceSource();

				std::vector<PackageVersionKey> result;
				for (const auto& versionInfo : m_package.versions)
				{
					result.emplace_back(
						source->GetIdentifier(), versionInfo.GetVersion().ToString(), versionInfo.GetChannel().ToString());
				}

				return result;
			}

			std::shared_ptr<IPackageVersion> GetLatestAvailableVersion() const override
			{
				return GetLatestVersionInternal();
			}

			std::shared_ptr<IPackageVersion> GetAvailableVersion(const PackageVersionKey& versionKey) const override
			{
				std::shared_ptr<const RestSource> source = GetReferenceSource();

				// Ensure that this key targets this (or any) source
				if ((!versionKey.SourceId.empty() && versionKey.SourceId != source->GetIdentifier())
					|| versionKey.Version.empty() && versionKey.Channel.empty())
				{
					return {};
				}

				// TODO: Change filtering logic.
				std::shared_ptr<PackageVersion> packageVersion;
				for (const auto& versionInfo : m_package.versions)
				{
					if (CaseInsensitiveEquals(versionInfo.GetVersion().ToString(), versionKey.Version)
						&& CaseInsensitiveEquals(versionInfo.GetChannel().ToString(), versionKey.Channel))
					{
						return std::make_shared<PackageVersion>(source, m_package.packageInfo, versionInfo);
					}
				}

				return {};
			}

			bool IsUpdateAvailable() const override
			{
				return false;
			}
		};
	}

	RestSource::RestSource(const SourceDetails& details, std::string identifier, RestClient&& restClient)
		: m_details(details), m_identifier(std::move(identifier)), m_restClient(std::move(restClient))
	{
	}

	const SourceDetails& RestSource::GetDetails() const
	{
		return m_details;
	}

	const RestClient& RestSource::GetRestClient() const
	{
		return m_restClient;
	}

	const std::string& RestSource::GetIdentifier() const
	{
		return m_identifier;
	}

	SearchResult RestSource::Search(const SearchRequest& request) const
	{
		// Note: Basic search functionality to fetch everything.
		RestClient::SearchResult results = m_restClient.Search(request);
		SearchResult searchResult;

		std::shared_ptr<const RestSource> sharedThis = shared_from_this();
		for (auto& result : results.Matches)
		{
			// TODO: Check if package is available or installed.
			std::unique_ptr<IPackage> package = std::make_unique<AvailablePackage>(sharedThis, result);
			PackageMatchFilter packageFilter({}, {}, {});

			// Sort the versions
			std::sort(result.versions.begin(), result.versions.end(),
				[](const VersionAndChannel& a, const VersionAndChannel& b)
				{
					return a.GetVersion() < b.GetVersion();
				});

			searchResult.Matches.emplace_back(std::move(package), std::move(packageFilter));
		}

		return searchResult;
	}
}
