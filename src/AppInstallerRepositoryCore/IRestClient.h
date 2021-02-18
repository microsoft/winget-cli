// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "pch.h"
#include "Microsoft/Schema/Version.h"
#include "Public/AppInstallerRepositorySearch.h"
#include <AppInstallerVersions.h>
#include <winget/Manifest.h>
#include <filesystem>
#include <vector>

using namespace AppInstaller::Utility;
using namespace AppInstaller::Manifest;

namespace AppInstaller::Repository::Rest::Schema
{
	// The common interface used to interact with RestAPI responses.
	struct IRestClient
	{
		virtual ~IRestClient() = default;

		struct PackageInfo
		{
			std::string packageIdentifier;
			std::string packageName;
			std::string publisher;

			PackageInfo(std::string packageIdentifier, std::string packageName, std::string publisher) 
				: packageIdentifier(packageIdentifier), packageName(packageName), publisher(publisher) {}
		};

		struct Package
		{
			PackageInfo packageInfo;
			std::vector<VersionAndChannel> versions;

			Package(PackageInfo packageInfo, std::vector<VersionAndChannel> versions)
				: packageInfo(packageInfo), versions(versions) {}
		};

		struct SearchResult
		{
			std::vector<Package> Matches;
			bool Truncated = false;
		};

		// Performs a search based on the given criteria.
		virtual SearchResult Search(std::string restApiUri, const SearchRequest& request) const = 0;

		// Gets the manifest for given version
		virtual std::optional<std::string> GetManifestByVersion(std::string restApiUri, std::string packageId, std::string version) const = 0;

		// Gets all versions and channels for the given id.
		virtual std::vector<Utility::VersionAndChannel> GetVersionKeysFromPackage(const Manifest::Manifest& manifest) const = 0;

		// Gets the manifest id for the given { id, version, channel }, if present.
	   // If version is empty, gets the value for the 'latest' version.
		virtual std::optional<std::string> GetVersionFromPackage(const Manifest::Manifest& manifest, std::string version, std::string channel) const = 0;

		// Gets the string for the given property and manifest id, if present.
		virtual std::optional<std::string> GetPropertyFromVersion(const std::string& manifest, PackageVersionProperty property) const = 0;

		// Gets the string values for the given property and manifest id, if present.
		virtual std::vector<std::string> GetMultiPropertyFromVersion(const std::string& manifest, PackageVersionMultiProperty property) const = 0;
	};
}