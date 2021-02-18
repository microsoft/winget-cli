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

using namespace AppInstaller::Manifest;

namespace AppInstaller::Repository::Rest::Schema
{
	// The common interface used to interact with RestAPI responses.
	struct IRestClient
	{
		virtual ~IRestClient() = default;

		struct Package
		{
			// TODO: Change type based on discussion.
			Manifest::Manifest manifest;

			Package(Manifest::Manifest manifest) : manifest(manifest) {}
		};

		struct SearchResult
		{
			std::vector<Package> Matches;
			bool Truncated = false;
		};

		// Performs a search based on the given criteria.
		virtual SearchResult Search(std::string m_restApiUri, const SearchRequest& request) const = 0;

		// Gets all versions and channels for the given id.
		virtual std::vector<Utility::VersionAndChannel> GetVersionKeysFromPackage(const Manifest::Manifest& manifest) const = 0;

		// Gets the manifest id for the given { id, version, channel }, if present.
	   // If version is empty, gets the value for the 'latest' version.
		virtual std::optional<std::string> GetVersionFromPackage(const Manifest::Manifest& manifest, std::string_view version, std::string_view channel) const = 0;

		// Gets the string for the given property and manifest id, if present.
		virtual std::optional<std::string> GetPropertyFromVersion(const std::string& manifest, PackageVersionProperty property) const = 0;

		// Gets the string values for the given property and manifest id, if present.
		virtual std::vector<std::string> GetMultiPropertyFromVersion(const std::string& manifest, PackageVersionMultiProperty property) const = 0;
	};
}