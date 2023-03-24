// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/WindowsFeature.h"
#include "Public/AppInstallerLogging.h"
#include "Public/AppInstallerStrings.h"

namespace AppInstaller::WindowsFeature
{
#ifndef AICLI_DISABLE_TEST_HOOKS
    static bool s_MockDismHelper_Override = false;

    void TestHook_MockDismHelper_Override(bool status)
    {
        s_MockDismHelper_Override = status;
    }
#endif

    DismHelper::DismHelper()
    {
#ifndef AICLI_DISABLE_TEST_HOOKS
        // The entire DismHelper class and its functions needs to be mocked since DismHost.exe inherits log file handles.
        // Without this, the unit tests will fail to complete waiting for DismHost.exe to release the log file handles.
        if (s_MockDismHelper_Override)
        {
            return;
        }
#endif

        m_module.reset(LoadLibraryEx(L"dismapi.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32));
        
        if (!m_module)
        {
            AICLI_LOG(Core, Error, << "Could not load dismapi.dll");
            THROW_LAST_ERROR();
        }

        m_dismInitialize = GetProcAddressHelper<DismInitializePtr>(m_module.get(), "DismInitialize");
        m_dismOpenSession = GetProcAddressHelper<DismOpenSessionPtr>(m_module.get(), "DismOpenSession");
        m_dismGetFeatureInfo = GetProcAddressHelper<DismGetFeatureInfoPtr>(m_module.get(), "DismGetFeatureInfo");
        m_dismEnableFeature = GetProcAddressHelper<DismEnableFeaturePtr>(m_module.get(), "DismEnableFeature");
        m_dismDisableFeature = GetProcAddressHelper<DismDisableFeaturePtr>(m_module.get(), "DismDisableFeature");
        m_dismDelete = GetProcAddressHelper<DismDeletePtr>(m_module.get(), "DismDelete");
        m_dismCloseSession = GetProcAddressHelper<DismCloseSessionPtr>(m_module.get(), "DismCloseSession");
        m_dismShutdown = GetProcAddressHelper<DismShutdownPtr>(m_module.get(), "DismShutdown");

        Initialize();
        OpenSession();
    }

    DismHelper::~DismHelper()
    {
#ifndef AICLI_DISABLE_TEST_HOOKS
        if (s_MockDismHelper_Override)
        {
            return;
        }
#endif
        CloseSession();
        Shutdown();
    };

    template<typename FuncType>
    FuncType DismHelper::GetProcAddressHelper(HMODULE module, LPCSTR functionName)
    {
        FuncType result = reinterpret_cast<FuncType>(GetProcAddress(module, functionName));
        if (!result)
        {
            AICLI_LOG(Core, Error, << "Could not get proc address of " << functionName);
            THROW_LAST_ERROR();
        }
        return result;
    }

    void DismHelper::Initialize()
    {
        LOG_IF_FAILED(m_dismInitialize(DismLogErrorsWarningsInfo, NULL, NULL));
    }

    void DismHelper::OpenSession()
    {
        LOG_IF_FAILED(m_dismOpenSession(DISM_ONLINE_IMAGE, NULL, NULL, &m_dismSession));
    }

    void DismHelper::CloseSession()
    {
        LOG_IF_FAILED(m_dismCloseSession(m_dismSession));
    }

    HRESULT DismHelper::EnableFeature(
        PCWSTR featureName,
        PCWSTR identifier,
        DismPackageIdentifier packageIdentifier,
        BOOL limitAccess,
        PCWSTR* sourcePaths,
        UINT sourcePathCount,
        BOOL enableAll,
        HANDLE cancelEvent,
        DISM_PROGRESS_CALLBACK progress,
        PVOID userData)
    {
        return m_dismEnableFeature(m_dismSession, featureName, identifier, packageIdentifier, limitAccess, sourcePaths, sourcePathCount, enableAll, cancelEvent, progress, userData);
    }

    HRESULT DismHelper::DisableFeature(PCWSTR featureName, PCWSTR packageName, BOOL removePayload, HANDLE cancelEvent, DISM_PROGRESS_CALLBACK progress, PVOID userData)
    {
        return m_dismDisableFeature(m_dismSession, featureName, packageName, removePayload, cancelEvent, progress, userData);
    }

    HRESULT DismHelper::GetFeatureInfo(PCWSTR featureName, PCWSTR identifier, DismPackageIdentifier packageIdentifier, DismFeatureInfo** featureInfo)
    {
        return m_dismGetFeatureInfo(m_dismSession, featureName, identifier, packageIdentifier, featureInfo);
    }

    HRESULT DismHelper::Delete(VOID* dismStructure)
    {
        return m_dismDelete(dismStructure);
    }

    void DismHelper::Shutdown()
    {
        LOG_IF_FAILED(m_dismShutdown());
    }

    WindowsFeature::WindowsFeature(std::shared_ptr<DismHelper> dismHelper, const std::string& name)
        : m_dismHelper(dismHelper), m_featureName(name)
    {
#ifndef AICLI_DISABLE_TEST_HOOKS
        if (s_MockDismHelper_Override)
        {
            return;
        }
#endif
        GetFeatureInfo();
    }

    WindowsFeature::~WindowsFeature()
    {
        if (m_featureInfo)
        {
            LOG_IF_FAILED(m_dismHelper->Delete(m_featureInfo));
        }
    }

#ifndef AICLI_DISABLE_TEST_HOOKS
    static HRESULT* s_EnableWindowsFeatureResult_TestHook_Override = nullptr;

    void TestHook_SetEnableWindowsFeatureResult_Override(HRESULT* result)
    {
        s_EnableWindowsFeatureResult_TestHook_Override = result;
    }

    static bool* s_DoesWindowsFeatureExistResult_TestHook_Override = nullptr;

    void TestHook_SetDoesWindowsFeatureExistResult_Override(bool* result)
    {
        s_DoesWindowsFeatureExistResult_TestHook_Override = result;
    }

    static bool* s_IsWindowsFeatureEnabledResult_TestHook_Override = nullptr;

    void TestHook_SetIsWindowsFeatureEnabledResult_Override(bool* status)
    {
        s_IsWindowsFeatureEnabledResult_TestHook_Override = status;
    }

    static Utility::LocIndString* s_WindowsFeatureGetDisplayNameResult_TestHook_Override = nullptr;

    void TestHook_SetWindowsFeatureGetDisplayNameResult_Override(Utility::LocIndString* displayName)
    {
        s_WindowsFeatureGetDisplayNameResult_TestHook_Override = displayName;
    }

    static DismRestartType* s_WindowsFeatureGetRestartRequiredStatusResult_TestHook_Override = nullptr;

    void TestHook_SetWindowsFeatureGetRestartStatusResult_Override(DismRestartType* restartType)
    {
        s_WindowsFeatureGetRestartRequiredStatusResult_TestHook_Override = restartType;
    }
#endif

    HRESULT WindowsFeature::Enable(AppInstaller::IProgressCallback& progress)
    {
        UNREFERENCED_PARAMETER(progress);

#ifndef AICLI_DISABLE_TEST_HOOKS
        if (s_EnableWindowsFeatureResult_TestHook_Override)
        {
            return *s_EnableWindowsFeatureResult_TestHook_Override;
        }
#endif
        HRESULT hr = m_dismHelper->EnableFeature(Utility::ConvertToUTF16(m_featureName).c_str(), NULL, DismPackageNone, FALSE, NULL, NULL, FALSE, NULL, NULL, NULL);
        LOG_IF_FAILED(hr);
        return hr;
    }

    HRESULT WindowsFeature::Disable()
    {
        HRESULT hr = m_dismHelper->DisableFeature(Utility::ConvertToUTF16(m_featureName).c_str(), NULL, FALSE, NULL, NULL, NULL);
        LOG_IF_FAILED(hr);
        return hr;
    }

    bool WindowsFeature::DoesExist()
    {
#ifndef AICLI_DISABLE_TEST_HOOKS
        if (s_DoesWindowsFeatureExistResult_TestHook_Override)
        {
            return *s_DoesWindowsFeatureExistResult_TestHook_Override;
        }
#endif
        return m_featureInfo;
    }

    bool WindowsFeature::IsEnabled()
    {
#ifndef AICLI_DISABLE_TEST_HOOKS
        if (s_IsWindowsFeatureEnabledResult_TestHook_Override)
        {
            return *s_IsWindowsFeatureEnabledResult_TestHook_Override;
        }
#endif
        // Refresh feature info state prior to retrieving state info.
        GetFeatureInfo();
        DismPackageFeatureState featureState = GetState();
        AICLI_LOG(Core, Info, << "Feature state of " << m_featureName << " is " << featureState);
        return featureState == DismStateInstalled;
    }

    Utility::LocIndString WindowsFeature::GetDisplayName()
    {
#ifndef AICLI_DISABLE_TEST_HOOKS
        if (s_WindowsFeatureGetDisplayNameResult_TestHook_Override)
        {
            return *s_WindowsFeatureGetDisplayNameResult_TestHook_Override;
        }
#endif
        return Utility::LocIndString{ Utility::ConvertToUTF8(std::wstring{ m_featureInfo->DisplayName }) };
    }

    DismRestartType WindowsFeature::GetRestartRequiredStatus()
    {
#ifndef AICLI_DISABLE_TEST_HOOKS
        if (s_WindowsFeatureGetRestartRequiredStatusResult_TestHook_Override)
        {
            return *s_WindowsFeatureGetRestartRequiredStatusResult_TestHook_Override;
        }
#endif
        return m_featureInfo->RestartRequired;
    }

    void WindowsFeature::GetFeatureInfo()
    {
        LOG_IF_FAILED(m_dismHelper->GetFeatureInfo(Utility::ConvertToUTF16(m_featureName).c_str(), NULL, DismPackageNone, &m_featureInfo));
    }
}