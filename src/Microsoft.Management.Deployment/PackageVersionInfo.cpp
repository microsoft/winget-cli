// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <mutex>
#include <winget/RepositorySource.h>
#include "PackageVersionInfo.h"
#include "PackageVersionInfo.g.cpp"
#include "PackageCatalogInfo.h"
#include "PackageCatalog.h"
#include "CatalogPackage.h"
#include "ComContext.h"
#include "Workflows/WorkflowBase.h"
#include "Workflows/ManifestComparator.h"
#include "winget/RepositorySearch.h"
#include "AppInstallerVersions.h"
#include "Converters.h"
#pragma warning( push )
#pragma warning ( disable : 4467 )
// 4467 Allow use of uuid attribute for com object creation.
#include "PackageManager.h"
#pragma warning( pop )
#include <wil\cppwinrt_wrl.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    void PackageVersionInfo::Initialize(std::shared_ptr<::AppInstaller::Repository::IPackageVersion> packageVersion)
    {
        m_packageVersion = std::move(packageVersion);
    }
    hstring PackageVersionInfo::GetMetadata(winrt::Microsoft::Management::Deployment::PackageVersionMetadataField const& metadataField)
    {
        ::AppInstaller::Repository::PackageVersionMetadata metadataKey = GetRepositoryPackageVersionMetadata(metadataField);
        ::AppInstaller::Repository::IPackageVersion::Metadata metadata = m_packageVersion->GetMetadata();
        auto result = metadata.find(metadataKey);
        hstring resultString = winrt::to_hstring(result->second);
        // The api uses "System" rather than "Machine" for install scope.
        if (metadataField == PackageVersionMetadataField::InstalledScope && resultString == L"Machine")
        {
            return winrt::to_hstring(L"System");
        }
        return resultString;
    }
    hstring PackageVersionInfo::Id()
    {
        return winrt::to_hstring(m_packageVersion->GetProperty(::AppInstaller::Repository::PackageVersionProperty::Id).get());
    }
    hstring PackageVersionInfo::DisplayName()
    {
        return winrt::to_hstring(m_packageVersion->GetProperty(::AppInstaller::Repository::PackageVersionProperty::Name).get());
    }
    hstring PackageVersionInfo::Publisher()
    {
        return winrt::to_hstring(m_packageVersion->GetProperty(::AppInstaller::Repository::PackageVersionProperty::Publisher).get());
    }
    hstring PackageVersionInfo::Version()
    {
        return winrt::to_hstring(m_packageVersion->GetProperty(::AppInstaller::Repository::PackageVersionProperty::Version).get());
    }
    hstring PackageVersionInfo::Channel()
    {
        return winrt::to_hstring(m_packageVersion->GetProperty(::AppInstaller::Repository::PackageVersionProperty::Channel).get());
    }
    winrt::Windows::Foundation::Collections::IVectorView<hstring> PackageVersionInfo::PackageFamilyNames()
    {
        if (!m_packageFamilyNames)
        {
            // Vector hasn't been created yet, create and populate it.
            auto packageFamilyNames = winrt::single_threaded_vector<hstring>(); 
            for (auto&& string : m_packageVersion->GetMultiProperty(::AppInstaller::Repository::PackageVersionMultiProperty::PackageFamilyName))
            {
                packageFamilyNames.Append(winrt::to_hstring(string));
            }
            m_packageFamilyNames = packageFamilyNames;
        }
        return m_packageFamilyNames.GetView();
    }
    winrt::Windows::Foundation::Collections::IVectorView<hstring> PackageVersionInfo::ProductCodes()
    {
        if (!m_productCodes)
        {
            // Vector hasn't been created yet, create and populate it.
            auto productCodes = winrt::single_threaded_vector<hstring>();
            for (auto&& string : m_packageVersion->GetMultiProperty(::AppInstaller::Repository::PackageVersionMultiProperty::ProductCode))
            {
                productCodes.Append(winrt::to_hstring(string));
            }
            m_productCodes = productCodes;
        }
        return m_productCodes.GetView();
    }
    winrt::Microsoft::Management::Deployment::PackageCatalog PackageVersionInfo::PackageCatalog()
    {
        if (!m_packageCatalog)
        {
            auto packageCatalogInfo = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::PackageCatalogInfo>>();
            packageCatalogInfo->Initialize(m_packageVersion->GetSource().GetDetails());
            auto packageCatalog = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::PackageCatalog>>();
            packageCatalog->Initialize(*packageCatalogInfo, m_packageVersion->GetSource(), false);
            m_packageCatalog = *packageCatalog;
        }
        return m_packageCatalog;
    }

    winrt::Microsoft::Management::Deployment::CompareResult PackageVersionInfo::CompareToVersion(hstring versionString)
    {
        if (versionString.empty())
        {
            return CompareResult::Unknown;
        }

        AppInstaller::Utility::Version thisVersion{ m_packageVersion->GetProperty(::AppInstaller::Repository::PackageVersionProperty::Version).get() };
        AppInstaller::Utility::Version otherVersion{ AppInstaller::Utility::ConvertToUTF8(versionString) };

        if (thisVersion < otherVersion)
        {
            return CompareResult::Lesser;
        }
        else if (otherVersion < thisVersion)
        {
            return CompareResult::Greater;
        }
        else
        {
            return CompareResult::Equal;
        }
    }
    bool PackageVersionInfo::HasApplicableInstaller(InstallOptions options)
    {
        AppInstaller::CLI::Execution::COMContext context;
        PopulateContextFromInstallOptions(&context, options);
        AppInstaller::Repository::IPackageVersion::Metadata installationMetadata;
        AppInstaller::CLI::Workflow::ManifestComparator manifestComparator{ context, installationMetadata };
        AppInstaller::Manifest::Manifest manifest = m_packageVersion->GetManifest();
        auto result = manifestComparator.GetPreferredInstaller(manifest);
        return result.installer.has_value();
    }
    std::shared_ptr<::AppInstaller::Repository::IPackageVersion> PackageVersionInfo::GetRepositoryPackageVersion()
    { 
        return m_packageVersion; 
    }
}
