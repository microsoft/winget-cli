#include "pch.h"
#include <AppInstallerRepositorySource.h>
#include "PackageVersionInfo.h"
#include "PackageVersionInfo.g.cpp"
#include "AppCatalog.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    PackageVersionInfo::PackageVersionInfo(std::shared_ptr<::AppInstaller::Repository::IPackageVersion> packageVersion)
    {
        m_packageVersion = packageVersion;
    }
    hstring PackageVersionInfo::GetMetadata(Microsoft::Management::Deployment::PackageVersionMetadata const& metadataType)
    {
        ::AppInstaller::Repository::IPackageVersion::Metadata metadata = m_packageVersion->GetMetadata();
        ::AppInstaller::Repository::PackageVersionMetadata metadataKey = ::AppInstaller::Repository::PackageVersionMetadata::InstalledLocation;
        switch (metadataType)
        {
        case Microsoft::Management::Deployment::PackageVersionMetadata::InstalledLocation :
            metadataKey = ::AppInstaller::Repository::PackageVersionMetadata::InstalledLocation;
            break;
        case Microsoft::Management::Deployment::PackageVersionMetadata::InstalledScope:
            metadataKey = ::AppInstaller::Repository::PackageVersionMetadata::InstalledScope;
            break;
        case Microsoft::Management::Deployment::PackageVersionMetadata::InstalledType:
            metadataKey = ::AppInstaller::Repository::PackageVersionMetadata::InstalledType;
            break;
        case Microsoft::Management::Deployment::PackageVersionMetadata::Locale:
            metadataKey = ::AppInstaller::Repository::PackageVersionMetadata::Locale;
            break;
        case Microsoft::Management::Deployment::PackageVersionMetadata::Publisher:
            metadataKey = ::AppInstaller::Repository::PackageVersionMetadata::Publisher;
            break;
        case Microsoft::Management::Deployment::PackageVersionMetadata::SilentUninstallCommand:
            metadataKey = ::AppInstaller::Repository::PackageVersionMetadata::SilentUninstallCommand;
            break;
        case Microsoft::Management::Deployment::PackageVersionMetadata::StandardUninstallCommand:
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
    hstring PackageVersionInfo::Name()
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
    Windows::Foundation::Collections::IVectorView<hstring> PackageVersionInfo::PackageFamilyName()
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
    Windows::Foundation::Collections::IVectorView<hstring> PackageVersionInfo::ProductCode()
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
    Microsoft::Management::Deployment::AppCatalog PackageVersionInfo::AppCatalog()
    {
        if (!m_appCatalog)
        {
            m_appCatalog = winrt::make<winrt::Microsoft::Management::Deployment::implementation::AppCatalog>(winrt::to_hstring(m_packageVersion->GetSource().get()->GetDetails().Name));
        }
        return m_appCatalog;
    }
}
