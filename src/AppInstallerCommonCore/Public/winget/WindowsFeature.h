// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

namespace AppInstaller::WindowsFeature
{
    /****************************************************************************\

        Declaration copied from DismApi.H to support enabling Windows Features.

        Copyright (c) Microsoft Corporation.
        All rights reserved.

    \****************************************************************************/

#ifndef _DISMAPI_H_
#define _DISMAPI_H_

#include <winapifamily.h>

#pragma region Desktop Family or DISM Package
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_DISM)

#ifdef __cplusplus
    extern "C"
    {
#endif

        typedef UINT DismSession;
        typedef void(CALLBACK* DISM_PROGRESS_CALLBACK)(_In_ UINT Current, _In_ UINT Total, _In_opt_ PVOID UserData);

#define DISM_ONLINE_IMAGE L"DISM_{53BFAE52-B167-4E2F-A258-0A37B57FF845}"
#define DISM_SESSION_DEFAULT 0

        typedef enum _DismLogLevel
        {
            DismLogErrors = 0,
            DismLogErrorsWarnings,
            DismLogErrorsWarningsInfo,
            DismLogErrorsWarningsInfoDebug
        } DismLogLevel;

        typedef enum _DismPackageIdentifier
        {
            DismPackageNone = 0,
            DismPackageName,
            DismPackagePath
        } DismPackageIdentifier;

        typedef enum _DismPackageFeatureState
        {
            DismStateNotPresent = 0,
            DismStateUninstallPending,
            DismStateStaged,
            DismStateResolved, // For internal use only
            DismStateRemoved = DismStateResolved,
            DismStateInstalled,
            DismStateInstallPending,
            DismStateSuperseded,
            DismStatePartiallyInstalled
        } DismPackageFeatureState;

        typedef enum _DismRestartType
        {
            DismRestartNo = 0,
            DismRestartPossible,
            DismRestartRequired
        } DismRestartType;

#pragma pack(push, 1)

        typedef struct _DismCustomProperty
        {
            PCWSTR Name;
            PCWSTR Value;
            PCWSTR Path;
        } DismCustomProperty;

        typedef struct _DismFeature
        {
            PCWSTR FeatureName;
            DismPackageFeatureState State;
        } DismFeature;

        typedef struct _DismFeatureInfo
        {
            PCWSTR FeatureName;
            DismPackageFeatureState FeatureState;
            PCWSTR DisplayName;
            PCWSTR Description;
            DismRestartType RestartRequired;
            DismCustomProperty* CustomProperty;
            UINT CustomPropertyCount;
        } DismFeatureInfo;

#pragma pack(pop)

#ifdef __cplusplus
    }
#endif

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_DISM) */
#pragma endregion

#endif // _DISMAPI_H_

    using DismInitializePtr = HRESULT(WINAPI*)(int, PCWSTR, PCWSTR);
    using DismOpenSessionPtr = HRESULT(WINAPI*)(PCWSTR, PCWSTR, PCWSTR, UINT*);
    using DismShutdownPtr = HRESULT(WINAPI*)();
    using DismGetFeatureInfoPtr = HRESULT(WINAPI*)(UINT, PCWSTR, PCWSTR, DismPackageIdentifier, DismFeatureInfo**);
    using DismEnableFeaturePtr = HRESULT(WINAPI*)(UINT, PCWSTR, PCWSTR, DismPackageIdentifier, BOOL, PCWSTR*, UINT, BOOL, HANDLE, DISM_PROGRESS_CALLBACK, PVOID);
    using DismDisableFeaturePtr = HRESULT(WINAPI*)(UINT, PCWSTR, PCWSTR, BOOL, HANDLE, DISM_PROGRESS_CALLBACK, PVOID);
    using DismDeletePtr = HRESULT(WINAPI*)(VOID*);

    struct DismHelper
    {

        DismHelper();
        ~DismHelper()
        {
            Shutdown();
        };

        struct WindowsFeature
        {
            bool DoesExist();
            bool IsEnabled();
            DismPackageFeatureState GetState() { return m_featureInfo->FeatureState; };
            std::wstring GetDisplayName() { return std::wstring { m_featureInfo->DisplayName }; }
            DismRestartType GetRestartRequiredStatus() { return m_featureInfo->RestartRequired;  }
            HRESULT Enable();
            HRESULT Disable();

            ~WindowsFeature()
            {
                if (m_featureInfo)
                {
                    m_deletePtr(m_featureInfo);
                }
            }

        private:
            friend DismHelper;

            WindowsFeature(const std::string& name, DismGetFeatureInfoPtr getFeatureInfoPtr, DismEnableFeaturePtr enableFeaturePtr, DismDisableFeaturePtr disableFeaturePtr, DismDeletePtr deletePtr, DismSession session);

            WindowsFeature(const std::string& name) : m_featureName(name)
            {
                GetFeatureInfo();
            }

            std::string m_featureName;
            DismFeatureInfo* m_featureInfo = nullptr;
            void GetFeatureInfo();

            DismGetFeatureInfoPtr m_getFeatureInfoPtr = nullptr;
            DismEnableFeaturePtr m_enableFeature = nullptr;
            DismDisableFeaturePtr m_disableFeaturePtr = nullptr;
            DismDeletePtr m_deletePtr = nullptr;
            DismSession m_session;
        };

        WindowsFeature CreateWindowsFeature(const std::string& name)
        {
            return WindowsFeature(name, m_dismGetFeatureInfo, m_dismEnableFeature, m_dismDisableFeature, m_dismDelete, m_session);
        }

    private:
        typedef UINT DismSession;

        wil::unique_hmodule m_module;
        DismSession m_session = DISM_SESSION_DEFAULT;

        DismInitializePtr m_dismInitialize = nullptr;
        DismOpenSessionPtr m_dismOpenSession = nullptr;
        DismGetFeatureInfoPtr m_dismGetFeatureInfo = nullptr;
        DismEnableFeaturePtr m_dismEnableFeature = nullptr;
        DismDisableFeaturePtr m_dismDisableFeature = nullptr;
        DismShutdownPtr m_dismShutdown = nullptr;
        DismDeletePtr m_dismDelete = nullptr;

        void Initialize();
        void OpenSession();
        void Shutdown();
    };
}