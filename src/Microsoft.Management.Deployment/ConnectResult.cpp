#include "pch.h"
#include "ConnectResult.h"
#include "ConnectResult.g.cpp"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    void ConnectResult::Initialize(winrt::hresult errorCode, winrt::Microsoft::Management::Deployment::AppCatalog appCatalog)
    {
        m_errorCode = errorCode;
        m_appCatalog = appCatalog;
    }
    winrt::hresult ConnectResult::ErrorCode()
    {
        return m_errorCode;
    }
    winrt::Microsoft::Management::Deployment::AppCatalog ConnectResult::AppCatalog()
    {
        return m_appCatalog;
    }
}
