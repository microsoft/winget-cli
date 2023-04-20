// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerProgress.h>
#include <winget/LocIndependent.h>

namespace AppInstaller::WindowsFeature
{
    /****************************************************************************\

        Declaration copied from DismApi.h to support enabling Windows Features.

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
    using DismOpenSessionPtr = HRESULT(WINAPI*)(PCWSTR, PCWSTR, PCWSTR, DismSession*);
    using DismCloseSessionPtr = HRESULT(WINAPI*)(DismSession);
    using DismShutdownPtr = HRESULT(WINAPI*)();
    using DismGetFeatureInfoPtr = HRESULT(WINAPI*)(UINT, PCWSTR, PCWSTR, DismPackageIdentifier, DismFeatureInfo**);
    using DismEnableFeaturePtr = HRESULT(WINAPI*)(UINT, PCWSTR, PCWSTR, DismPackageIdentifier, BOOL, PCWSTR*, UINT, BOOL, HANDLE, DISM_PROGRESS_CALLBACK, PVOID);
    using DismDisableFeaturePtr = HRESULT(WINAPI*)(UINT, PCWSTR, PCWSTR, BOOL, HANDLE, DISM_PROGRESS_CALLBACK, PVOID);
    using DismDeletePtr = HRESULT(WINAPI*)(VOID*);

    // Forward declaration
    struct DismHelper;

    /// <summary>
    /// Struct representation of a single Windows Feature.
    /// </summary>
    struct WindowsFeature
    {
        friend DismHelper;

        ~WindowsFeature();

        // TODO: Implement progress via DismProgressFunction
        HRESULT Enable(IProgressCallback& progress);
        HRESULT Disable();
        bool DoesExist();
        bool IsEnabled();
        Utility::LocIndString GetDisplayName();
        DismRestartType GetRestartRequiredStatus();

        DismPackageFeatureState GetState()
        {
            return m_featureInfo->FeatureState;
        }

    protected:
        WindowsFeature(std::shared_ptr<DismHelper> dismHelper, const std::string& name);

    private:
        void GetFeatureInfo();

        std::string m_featureName;
        std::shared_ptr<DismHelper> m_dismHelper;
        DismFeatureInfo* m_featureInfo = nullptr;
    };

    struct DismHelper : public std::enable_shared_from_this<DismHelper>
    {
        DismHelper();
        ~DismHelper();

        WindowsFeature GetWindowsFeature(const std::string& featureName)
        {
            return WindowsFeature(std::move(GetPtr()), featureName);
        }

        HRESULT EnableFeature(
            PCWSTR featureName,
            PCWSTR identifier,
            DismPackageIdentifier packageIdentifier,
            BOOL limitAccess,
            PCWSTR* sourcePaths,
            UINT sourcePathCount,
            BOOL enableAll,
            HANDLE cancelEvent,
            DISM_PROGRESS_CALLBACK progress,
            PVOID userData);

        HRESULT DisableFeature(PCWSTR featureName, PCWSTR packageName, BOOL removePayload, HANDLE cancelEvent, DISM_PROGRESS_CALLBACK progress, PVOID userData);

        HRESULT GetFeatureInfo(PCWSTR featureName, PCWSTR identifier, DismPackageIdentifier packageIdentifier, DismFeatureInfo** featureInfo);

        HRESULT Delete(VOID* dismStructure);

    private:
        typedef UINT DismSession;

        wil::unique_hmodule m_module;
        DismSession m_dismSession = DISM_SESSION_DEFAULT;

        DismInitializePtr m_dismInitialize = nullptr;
        DismOpenSessionPtr m_dismOpenSession = nullptr;
        DismGetFeatureInfoPtr m_dismGetFeatureInfo = nullptr;
        DismEnableFeaturePtr m_dismEnableFeature = nullptr;
        DismDisableFeaturePtr m_dismDisableFeature = nullptr;
        DismCloseSessionPtr m_dismCloseSession = nullptr;
        DismShutdownPtr m_dismShutdown = nullptr;
        DismDeletePtr m_dismDelete = nullptr;

        std::shared_ptr<DismHelper> GetPtr()
        {
            return shared_from_this();
        }

        void Initialize();
        void OpenSession();
        void CloseSession();
        void Shutdown();

        template<typename FuncType>
        FuncType GetProcAddressHelper(HMODULE module, LPCSTR functionName);
    };
}