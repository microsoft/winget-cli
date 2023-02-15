// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/WindowsFeature.h"
#include "Public/AppInstallerLogging.h"
#include "Public/AppInstallerStrings.h"

namespace AppInstaller::WindowsFeature
{
    DismApiHelper::DismApiHelper()
    {
        m_module.reset(LoadLibraryEx(L"dismapi.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32));
        if (!m_module)
        {
            // Do I need special handling here?
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

    DismApiHelper::~DismApiHelper()
    {
        if (m_featureInfo)
        {
            Delete();
        }

        Shutdown();
    }

    DismFeatureInfo* DismApiHelper::GetFeatureInfo(const std::string_view& name)
    {
        if (m_dismGetFeatureInfo)
        {
            LOG_IF_FAILED(m_dismGetFeatureInfo(m_session, Utility::ConvertToUTF16(name).c_str(), NULL, DismPackageNone, &m_featureInfo));
        }

        return m_featureInfo;
    }

    HRESULT DismApiHelper::EnableFeature(const std::string_view& name)
    {
        HRESULT hr = ERROR_PROC_NOT_FOUND;
        if (m_dismEnableFeature)
        {
            hr = m_dismEnableFeature(m_session, Utility::ConvertToUTF16(name).c_str(), NULL, DismPackageNone, FALSE, NULL, NULL, TRUE, NULL, NULL, NULL);
            LOG_IF_FAILED(hr);
        }

        return hr;
    }

    HRESULT DismApiHelper::DisableFeature(const std::string_view& name)
    {
        HRESULT hr = ERROR_PROC_NOT_FOUND;
        if (m_dismDisableFeature)
        {
            hr = m_dismDisableFeature(m_session, Utility::ConvertToUTF16(name).c_str(), NULL, FALSE, NULL, NULL, NULL);
            LOG_IF_FAILED(hr);
        }

        return hr;
    }

    void DismApiHelper::Initialize()
    {
        if (m_dismInitialize)
        {
            LOG_IF_FAILED(m_dismInitialize(2, nullptr, nullptr));
        }
    }

    void DismApiHelper::OpenSession()
    {
        if (m_dismOpenSession)
        {
            LOG_IF_FAILED(m_dismOpenSession(L"DISM_{53BFAE52-B167-4E2F-A258-0A37B57FF845}", nullptr, nullptr, &m_session));
        }
    }

    void DismApiHelper::Delete()
    {
        if (m_dismDelete)
        {
            LOG_IF_FAILED(m_dismDelete(m_featureInfo));
        }
    }

    void DismApiHelper::Shutdown()
    {
        if (m_dismShutdown)
        {
            LOG_IF_FAILED(m_dismShutdown());
        }
    }

    bool WindowsFeature::EnableFeature()
    {
        DismFeatureInfo* featureInfo = m_dismApiHelper.GetFeatureInfo(m_featureName);
        if (!featureInfo)
        {
            AICLI_LOG(Core, Verbose, << "Windows feature " << m_featureName << " does not exist.");
            return false; 
        }

        HRESULT hr = m_dismApiHelper.EnableFeature(m_featureName);
        if (SUCCEEDED(hr))
        {
            AICLI_LOG(Core, Verbose, << "Windows feature " << m_featureName << " enabled successfully.");
            return true;
        }
        else
        {
            AICLI_LOG(Core, Verbose, << "Failed to enable Windows Feature " << m_featureName << " with HRESULT " << hr);
            return false;
        }
    }

    bool WindowsFeature::DisableFeature()
    {
        DismFeatureInfo* featureInfo = m_dismApiHelper.GetFeatureInfo(m_featureName);
        if (!featureInfo)
        {
            AICLI_LOG(Core, Verbose, << "Windows feature " << m_featureName << " does not exist.");
            return true;
        }

        HRESULT hr = m_dismApiHelper.DisableFeature(m_featureName);
        if (SUCCEEDED(hr))
        {
            AICLI_LOG(Core, Verbose, << "Windows feature " << m_featureName << " disabled successfully.");
            return true;
        }
        else
        {
            AICLI_LOG(Core, Verbose, << "Failed to disable Windows Feature " << m_featureName << " with HRESULT " << hr);
            return false;
        }
    }

    bool WindowsFeature::DoesFeatureExist()
    {
        DismFeatureInfo* featureInfo = m_dismApiHelper.GetFeatureInfo(m_featureName);
        return featureInfo;
    }

    bool WindowsFeature::IsEnabled()
    {
        DismFeatureInfo* featureInfo = m_dismApiHelper.GetFeatureInfo(m_featureName);
        DismPackageFeatureState featureState = featureInfo->FeatureState;
        AICLI_LOG(Core, Verbose, << "Feature state of " << m_featureName << " is " << featureState);
        return (featureState == DismStateInstalled || featureState == DismStateInstallPending);
    }
}