// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "RestSource.h"
#include "RestSourceFactory.h"
#include "cpprest/http_client.h"
#include <winget/ManifestYamlParser.h>

using namespace AppInstaller::Manifest;
using namespace AppInstaller::Repository::Rest::Schema;

namespace AppInstaller::Repository::Rest
{
	namespace
	{
		using namespace AppInstaller::Repository::Rest::Schema;

		// The base for the package objects.
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
			PackageVersion(const std::shared_ptr<const RestSource>& source, std::string manifest) : SourceReference(source), m_manifest(manifest) {}

			// Inherited via IPackageVersion
			Utility::LocIndString GetProperty(PackageVersionProperty property) const override
			{
				UNREFERENCED_PARAMETER(property);
				// TODO: Call manifest helper to get property details
				//switch (property)
				//{
				//case PackageVersionProperty::SourceIdentifier:
				//	return Utility::LocIndString{ GetReferenceSource()->GetIdentifier() };
				//case PackageVersionProperty::SourceName:
				//	return Utility::LocIndString { GetReferenceSource()->GetDetails().Name };
				//default:
				//	// Values coming from the index will always be localized/independent.
				//	return Utility::LocIndString{ GetReferenceSource()->GetRestClient().GetPropertyFromVersion(m_manifest, property).value() };
				//}

				return {};
			}

			// TODO
			std::vector<Utility::LocIndString> GetMultiProperty(PackageVersionMultiProperty property) const override
			{
				UNREFERENCED_PARAMETER(property);

				// TODO: Call manifest helper to get property details
				std::vector<Utility::LocIndString> result;

				std::string value = "Foo";
				result.emplace_back(Utility::LocIndString{ value });

				return result;
			}

			// TODO
			Manifest::Manifest GetManifest() const override
			{
				// TODO: Get manifest with this package version.

				/* std::shared_ptr<const SQLiteIndexSource> source = GetReferenceSource();
				std::optional<std::string> relativePathOpt = source->GetIndex().GetPropertyByManifestId(m_manifestId, PackageVersionProperty::RelativePath);
				THROW_HR_IF(E_NOT_SET, !relativePathOpt);
				return GetManifestFromArgAndRelativePath(source->GetDetails().Arg, relativePathOpt.value()); */
				return Manifest::YamlParser::Create(m_manifest);
			}

			std::shared_ptr<const ISource> GetSource() const override
			{
				return GetReferenceSource();
			}

			// TODO
			IPackageVersion::Metadata GetMetadata() const override
			{
				return {};
			}

		private:
			std::string m_manifest;
		};

		// The base for IPackage implementations here.
		struct PackageBase : public SourceReference
		{
			PackageBase(const std::shared_ptr<const RestSource>& source, const IRestClient::Package package) :
				SourceReference(source), m_package(package) {}

			Utility::LocIndString GetProperty(PackageProperty property) const
			{
				Utility::LocIndString result;

				std::shared_ptr<IPackageVersion> truth = GetLatestVersionInternal();
				if (truth)
				{
					switch (property)
					{
					case PackageProperty::Id:
						return truth->GetProperty(PackageVersionProperty::Id);
					case PackageProperty::Name:
						return truth->GetProperty(PackageVersionProperty::Name);
					default:
						THROW_HR(E_UNEXPECTED);
					}
				}

				return result;
			}

		protected:
			std::shared_ptr<IPackageVersion> GetLatestVersionInternal() const
			{
				std::shared_ptr<const RestSource> source = GetReferenceSource();
				std::optional<std::string> manifest = source->GetRestClient().GetVersionFromPackage(m_package.manifest, {}, {});
				if (manifest)
				{
					return std::make_shared<PackageVersion>(source, manifest.value());
				}

				return {};
			}

			IRestClient::Package m_package;
		};

		// The IPackage impl for RestSource of Available packages.
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
				// TODO: Call Manifest helper to get available version keys
				std::shared_ptr<const RestSource> source = GetReferenceSource();
				std::vector<Utility::VersionAndChannel> versions = source->GetRestClient().GetVersionKeysFromPackage(m_package.manifest);

				std::vector<PackageVersionKey> result;
				for (const auto& vac : versions)
				{
					result.emplace_back(source->GetIdentifier(), vac.GetVersion().ToString(), vac.GetChannel().ToString());
				}

				return {};
			}

			std::shared_ptr<IPackageVersion> GetLatestAvailableVersion() const override
			{
				return GetLatestVersionInternal();
			}

			std::shared_ptr<IPackageVersion> GetAvailableVersion(const PackageVersionKey& versionKey) const override
			{
				std::shared_ptr<const RestSource> source = GetReferenceSource();

				// Ensure that this key targets this (or any) source
				if (!versionKey.SourceId.empty() && versionKey.SourceId != source->GetIdentifier())
				{
					return {};
				}

				std::optional<std::string> manifestId = source->GetRestClient().GetVersionFromPackage(
					m_package.manifest, versionKey.Version, versionKey.Channel);

				if (manifestId)
				{
					return std::make_shared<PackageVersion>(source, manifestId.value());
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
		SearchResult returnVal;

		std::shared_ptr<const RestSource> sharedThis = shared_from_this();
		for (auto& result : results.Matches)
		{
			// TODO: Check if package is available or installed.
			auto package = AvailablePackage(sharedThis, result);
		}

		return returnVal;
	}
}
