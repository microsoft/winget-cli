// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "pch.h"
#include <AppInstallerVersions.h>
#include <winget/Manifest.h>
#include <SQLiteWrapper.h>
#include <PackageDependenciesValidation.h>
#include <Microsoft/Schema/1_4/DependenciesTable.h>

namespace AppInstaller::Repository
{
	using namespace Microsoft::Schema::V1_4;

	bool PackageDependenciesValidation::ValidateManifestDependencies(const std::string& indexPath, const Manifest::Manifest& manifest)
	{
		SQLite::Connection connection(SQLite::Connection::Create(indexPath, SQLite::Connection::OpenDisposition::ReadOnly, SQLite::Connection::OpenFlags::None));
		return DependenciesTable::ValidateDependencies(connection, manifest);
	}

	bool PackageDependenciesValidation::VerifyDependenciesStructureForManifestDelete(const std::string& indexPath, const Manifest::Manifest& manifest)
	{
		SQLite::Connection connection(SQLite::Connection::Create(indexPath, SQLite::Connection::OpenDisposition::ReadOnly, SQLite::Connection::OpenFlags::None));
		return DependenciesTable::VerifyDependenciesStructureForManifestDelete(connection, manifest);
	}
}