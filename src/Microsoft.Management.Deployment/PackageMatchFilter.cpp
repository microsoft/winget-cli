#include "pch.h"
#include <AppInstallerRepositorySource.h>
#include <AppInstallerRepositorySearch.h>
#include "Converters.h"
#include "PackageMatchFilter.h"
#include "PackageMatchFilter.g.cpp"
#include <wil\cppwinrt_wrl.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    void PackageMatchFilter::Initialize(::AppInstaller::Repository::PackageMatchFilter matchFilter)
    {
        m_value = winrt::to_hstring(matchFilter.Value);
        m_matchField = GetDeploymentMatchField(matchFilter.Field);
        m_packageFieldMatchOption = GetDeploymentMatchOption(matchFilter.Type);
    }
    winrt::Microsoft::Management::Deployment::PackageFieldMatchOption PackageMatchFilter::Option()
    {
        return m_packageFieldMatchOption;
    }
    void PackageMatchFilter::Option(winrt::Microsoft::Management::Deployment::PackageFieldMatchOption const& value)
    {
        m_packageFieldMatchOption = value;
    }
    winrt::Microsoft::Management::Deployment::PackageMatchField PackageMatchFilter::Field()
    {
        return m_matchField;
    }
    void PackageMatchFilter::Field(winrt::Microsoft::Management::Deployment::PackageMatchField const& value)
    {
        m_matchField = value;
    }
    hstring PackageMatchFilter::Value()
    {
        return hstring(m_value);
    }
    void PackageMatchFilter::Value(hstring const& value)
    {
        m_value = value;
    }
    CoCreatableCppWinRtClass(PackageMatchFilter);
}
