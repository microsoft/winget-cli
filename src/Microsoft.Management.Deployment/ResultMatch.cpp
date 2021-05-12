#include "pch.h"
#include <AppInstallerRepositorySource.h>
#include <AppInstallerRepositorySearch.h>
#include "ResultMatch.h"
#include "ResultMatch.g.cpp"
#include "CatalogPackage.h"
#include <wil\cppwinrt_wrl.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    ResultMatch::ResultMatch(Microsoft::Management::Deployment::CatalogPackage package, Microsoft::Management::Deployment::PackageMatchFilter matchCriteria)
    {
        m_catalogPackage = package;
        m_matchCriteria = matchCriteria;
    }
    void ResultMatch::Initialize(Microsoft::Management::Deployment::CatalogPackage package, Microsoft::Management::Deployment::PackageMatchFilter matchCriteria)
    {
        m_catalogPackage = package;
        m_matchCriteria = matchCriteria;
    }
    Microsoft::Management::Deployment::CatalogPackage ResultMatch::CatalogPackage()
    {
        return m_catalogPackage;
    }
    Microsoft::Management::Deployment::PackageMatchFilter ResultMatch::MatchCriteria()
    {
        return m_matchCriteria;
    }
    CoCreatableCppWinRtClass(ResultMatch);
}
