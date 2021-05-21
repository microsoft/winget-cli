#include "pch.h"
#include <AppInstallerRepositorySource.h>
#include "PackageVersionInfo.h"
#include "PackageVersionInfo.g.cpp"
#include "PackageCatalogInfo.h"
#include "PackageCatalogReference.h"
#include <wil\cppwinrt_wrl.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    void PackageVersionInfo::Initialize(std::shared_ptr<::AppInstaller::Repository::IPackageVersion> packageVersion)
    {
        m_packageVersion = packageVersion;
    }
    hstring PackageVersionInfo::GetMetadata(winrt::Microsoft::Management::Deployment::PackageVersionMetadataField const& metadataField)
    {
        ::AppInstaller::Repository::IPackageVersion::Metadata metadata = m_packageVersion->GetMetadata();
        ::AppInstaller::Repository::PackageVersionMetadata metadataKey = ::AppInstaller::Repository::PackageVersionMetadata::InstalledLocation;
        switch (metadataField)
        {
        case Microsoft::Management::Deployment::PackageVersionMetadataField::InstalledLocation :
            metadataKey = ::AppInstaller::Repository::PackageVersionMetadata::InstalledLocation;
            break;
        case Microsoft::Management::Deployment::PackageVersionMetadataField::InstalledScope:
            metadataKey = ::AppInstaller::Repository::PackageVersionMetadata::InstalledScope;
            break;
        case Microsoft::Management::Deployment::PackageVersionMetadataField::InstallerType:
            metadataKey = ::AppInstaller::Repository::PackageVersionMetadata::InstalledType;
            break;
        case Microsoft::Management::Deployment::PackageVersionMetadataField::Locale:
            metadataKey = ::AppInstaller::Repository::PackageVersionMetadata::InstalledLocale;
            break;
        case Microsoft::Management::Deployment::PackageVersionMetadataField::PublisherDisplayName:
            metadataKey = ::AppInstaller::Repository::PackageVersionMetadata::Publisher;
            break;
        case Microsoft::Management::Deployment::PackageVersionMetadataField::SilentUninstallCommand:
            metadataKey = ::AppInstaller::Repository::PackageVersionMetadata::SilentUninstallCommand;
            break;
        case Microsoft::Management::Deployment::PackageVersionMetadataField::StandardUninstallCommand:
            metadataKey = ::AppInstaller::Repository::PackageVersionMetadata::StandardUninstallCommand;
            break;
        }
        auto result = metadata.find(metadataKey);
        return winrt::to_hstring(result->second);
    }
    hstring PackageVersionInfo::Id()
    {
        return winrt::to_hstring(m_packageVersion->GetProperty(::AppInstaller::Repository::PackageVersionProperty::Id).get());
    }
    hstring PackageVersionInfo::DisplayName()
    {
        return winrt::to_hstring(m_packageVersion->GetProperty(::AppInstaller::Repository::PackageVersionProperty::Name).get());
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
        if (m_packageFamilyNames.Size() == 0)
        {
            auto packageFamilyNameVector = m_packageVersion->GetMultiProperty(::AppInstaller::Repository::PackageVersionMultiProperty::PackageFamilyName);
            for (int i = 0; i < packageFamilyNameVector.size(); ++i)
            {
                m_packageFamilyNames.Append(winrt::to_hstring(packageFamilyNameVector.at(i)));
            }
        }

        return m_packageFamilyNames.GetView();
    }
    winrt::Windows::Foundation::Collections::IVectorView<hstring> PackageVersionInfo::ProductCodes()
    {
        if (m_productCodes.Size() == 0)
        {
            auto productCodeVector = m_packageVersion->GetMultiProperty(::AppInstaller::Repository::PackageVersionMultiProperty::ProductCode);
            for (int i = 0; i < productCodeVector.size(); i++)
            {
                m_productCodes.Append(winrt::to_hstring(productCodeVector.at(i)));
            }
        }

        return m_productCodes.GetView();
    }
    winrt::Microsoft::Management::Deployment::PackageCatalogReference PackageVersionInfo::PackageCatalogReference()
    {
        if (!m_packageCatalogReference)
        {
            auto packageCatalogInfo = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::PackageCatalogInfo>>();
            packageCatalogInfo->Initialize(m_packageVersion->GetSource().get()->GetDetails());
            auto packageCatalogRefImpl = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::PackageCatalogReference>>();
            packageCatalogRefImpl->Initialize(*packageCatalogInfo);
            m_packageCatalogReference = *packageCatalogRefImpl;
        }
        return m_packageCatalogReference;
    }
}
