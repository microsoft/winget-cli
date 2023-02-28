// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/WindowsFeature.h"
#include "Public/AppInstallerLogging.h"
#include "Public/AppInstallerStrings.h"

namespace AppInstaller::WindowsFeature
{
    DismHelper::DismHelper()
    {
        m_module.reset(LoadLibraryEx(L"dismapi.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32));
        if (!m_module)
        {
            AICLI_LOG(Core, Verbose, << "Could not load dismapi.dll");
            return;
        }

        m_dismInitialize =
            reinterpret_cast<DismInitializePtr>(GetProcAddress(m_module.get(), "DismInitialize"));
        if (!m_dismInitialize)
        {
            AICLI_LOG(Core, Verbose, << "Could not get proc address of DismInitialize");
            return;
        }

        m_dismOpenSession =
            reinterpret_cast<DismOpenSessionPtr>(GetProcAddress(m_module.get(), "DismOpenSession"));
        if (!m_dismOpenSession)
        {
            AICLI_LOG(Core, Verbose, << "Could not get proc address of DismOpenSession");
            return;
        }

        m_dismGetFeatureInfo =
            reinterpret_cast<DismGetFeatureInfoPtr>(GetProcAddress(m_module.get(), "DismGetFeatureInfo"));
        if (!m_dismGetFeatureInfo)
        {
            AICLI_LOG(Core, Verbose, << "Could not get proc address of DismGetFeatureInfo");
            return;
        }

        m_dismEnableFeature =
            reinterpret_cast<DismEnableFeaturePtr>(GetProcAddress(m_module.get(), "DismEnableFeature"));
        if (!m_dismEnableFeature)
        {
            AICLI_LOG(Core, Verbose, << "Could not get proc address of DismEnableFeature");
            return;
        }

        m_dismDisableFeature =
            reinterpret_cast<DismDisableFeaturePtr>(GetProcAddress(m_module.get(), "DismDisableFeature"));
        if (!m_dismDisableFeature)
        {
            AICLI_LOG(Core, Verbose, << "Could not get proc address of DismDisableFeature");
            return;
        }

        m_dismDelete =
            reinterpret_cast<DismDeletePtr>(GetProcAddress(m_module.get(), "DismDelete"));
        if (!m_dismDelete)
        {
            AICLI_LOG(Core, Verbose, << "Could not get proc address of DismDelete");
            return;
        }

        m_dismShutdown =
            reinterpret_cast<DismShutdownPtr>(GetProcAddress(m_module.get(), "DismShutdown"));
        if (!m_dismGetFeatureInfo)
        {
            AICLI_LOG(Core, Verbose, << "Could not get proc address of DismShutdown");
            return;
        }

        Initialize();
        OpenSession();
    }

    void DismHelper::Initialize()
    {
        if (m_dismInitialize)
        {
            LOG_IF_FAILED(m_dismInitialize(2, NULL, NULL));
        }
    }

    void DismHelper::OpenSession()
    {
        if (m_dismOpenSession)
        {
            LOG_IF_FAILED(m_dismOpenSession(DISM_ONLINE_IMAGE, NULL, NULL, &m_session));
        }
    }

    void DismHelper::Shutdown()
    {
        if (m_dismShutdown)
        {
            LOG_IF_FAILED(m_dismShutdown());
        }
    }

#ifndef AICLI_DISABLE_TEST_HOOKS
    static HRESULT* s_EnableWindowsFeatureResult_TestHook_Override = nullptr;

    void TestHook_SetEnableWindowsFeatureResult_Override(HRESULT* result)
    {
        s_EnableWindowsFeatureResult_TestHook_Override = result;
    }
#endif

    HRESULT DismHelper::WindowsFeature::Enable()
    {
#ifndef AICLI_DISABLE_TEST_HOOKS
        if (s_EnableWindowsFeatureResult_TestHook_Override)
        {
            return *s_EnableWindowsFeatureResult_TestHook_Override;
        }
#endif
        HRESULT hr = ERROR_PROC_NOT_FOUND;
        if (m_enableFeature)
        {
            hr = m_enableFeature(m_session, Utility::ConvertToUTF16(m_featureName).c_str(), NULL, DismPackageNone, FALSE, NULL, NULL, TRUE, NULL, NULL, NULL);
            LOG_IF_FAILED(hr);
        }

        return hr;
    }

    HRESULT DismHelper::WindowsFeature::Disable()
    {
        HRESULT hr = ERROR_PROC_NOT_FOUND;
        if (m_disableFeaturePtr)
        {
            hr = m_disableFeaturePtr(m_session, Utility::ConvertToUTF16(m_featureName).c_str(), NULL, FALSE, NULL, NULL, NULL);
            LOG_IF_FAILED(hr);
        }

        return hr;
    }

    bool DismHelper::WindowsFeature::DoesExist()
    {
        return m_featureInfo;
    }

#ifndef AICLI_DISABLE_TEST_HOOKS
    static bool* s_IsWindowsFeatureEnabledResult_TestHook_Override = nullptr;

    void TestHook_SetIsWindowsFeatureEnabledResult_Override(bool* status)
    {
        s_IsWindowsFeatureEnabledResult_TestHook_Override = status;
    }
#endif

    bool DismHelper::WindowsFeature::IsEnabled()
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
        AICLI_LOG(Core, Verbose, << "Feature state of " << m_featureName << " is " << featureState);
        return (featureState == DismStateInstalled || featureState == DismStateInstallPending);
    }

    void DismHelper::WindowsFeature::GetFeatureInfo()
    {
        if (m_getFeatureInfoPtr)
        {
            LOG_IF_FAILED(m_getFeatureInfoPtr(m_session, Utility::ConvertToUTF16(m_featureName).c_str(), NULL, DismPackageNone, &m_featureInfo));
        }
    }

    DismHelper::WindowsFeature::WindowsFeature(const std::string& name, DismGetFeatureInfoPtr getFeatureInfoPtr, DismEnableFeaturePtr enableFeaturePtr, DismDisableFeaturePtr disableFeaturePtr, DismDeletePtr deletePtr, DismSession session)
    {
        m_featureName = name;
        m_getFeatureInfoPtr = getFeatureInfoPtr;
        m_enableFeature = enableFeaturePtr;
        m_disableFeaturePtr = disableFeaturePtr;
        m_deletePtr = deletePtr;
        m_session = session;

        GetFeatureInfo();
    }
}