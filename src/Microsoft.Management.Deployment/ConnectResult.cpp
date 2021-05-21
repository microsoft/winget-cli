#include "pch.h"
#include "ConnectResult.h"
#include "ConnectResult.g.cpp"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    void ConnectResult::Initialize(winrt::hresult errorCode, winrt::Microsoft::Management::Deployment::PackageCatalog packageCatalog)
    {
        m_errorCode = errorCode;
        m_packageCatalog = packageCatalog;
    }
    winrt::hresult ConnectResult::ErrorCode()
    {
        return m_errorCode;
    }
    winrt::Microsoft::Management::Deployment::PackageCatalog ConnectResult::PackageCatalog()
    {
        return m_packageCatalog;
    }
}
