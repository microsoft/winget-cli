// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/RepositorySearch.h>


namespace AppInstaller::Repository
{
    // Gets an IPackage that represents the available package versions for the installed version..
    std::shared_ptr<IPackageVersionCollection> GetAvailableVersionsForInstalledVersion(const std::shared_ptr<ICompositePackage>& composite);
}
