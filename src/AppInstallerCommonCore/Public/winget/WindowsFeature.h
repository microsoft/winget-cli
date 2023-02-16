// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

namespace AppInstaller::WindowsFeature
{
/****************************************************************************\

    DismApi.H

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

        //////////////////////////////////////////////////////////////////////////////
        //
        // Typedefs
        //
        //////////////////////////////////////////////////////////////////////////////

        typedef UINT DismSession;

        //////////////////////////////////////////////////////////////////////////////
        //
        // Callbacks
        //
        //////////////////////////////////////////////////////////////////////////////

        typedef void(CALLBACK* DISM_PROGRESS_CALLBACK)(_In_ UINT Current, _In_ UINT Total, _In_opt_ PVOID UserData);

        //////////////////////////////////////////////////////////////////////////////
        //
        // Constants
        //
        //////////////////////////////////////////////////////////////////////////////

#define DISM_ONLINE_IMAGE L"DISM_{53BFAE52-B167-4E2F-A258-0A37B57FF845}"
#define DISM_SESSION_DEFAULT 0

    //////////////////////////////////////////////////////////////////////////////
    //
    // Enums
    //
    //////////////////////////////////////////////////////////////////////////////

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

        //////////////////////////////////////////////////////////////////////////////
        //
        // Structs
        //
        //////////////////////////////////////////////////////////////////////////////
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

        //////////////////////////////////////////////////////////////////////////////
        //
        // Success Codes
        //
        //////////////////////////////////////////////////////////////////////////////

        // For online scenario, computer needs to be restarted when the return value is ERROR_SUCCESS_REBOOT_REQUIRED (3010L).

        //
        // MessageId: DISMAPI_S_RELOAD_IMAGE_SESSION_REQUIRED
        //
        // MessageText:
        //
        // The DISM session needs to be reloaded.
        //
#define DISMAPI_S_RELOAD_IMAGE_SESSION_REQUIRED 0x00000001

//////////////////////////////////////////////////////////////////////////////
//
// Error Codes
//
//////////////////////////////////////////////////////////////////////////////

//
// MessageId: DISMAPI_E_DISMAPI_NOT_INITIALIZED
//
// MessageText:
//
// DISM API was not initialized for this process
//
#define DISMAPI_E_DISMAPI_NOT_INITIALIZED 0xC0040001

//
// MessageId: DISMAPI_E_SHUTDOWN_IN_PROGRESS
//
// MessageText:
//
// A DismSession was being shutdown when another operation was called on it
//
#define DISMAPI_E_SHUTDOWN_IN_PROGRESS 0xC0040002

//
// MessageId: DISMAPI_E_OPEN_SESSION_HANDLES
//
// MessageText:
//
// A DismShutdown was called while there were open DismSession handles
//
#define DISMAPI_E_OPEN_SESSION_HANDLES 0xC0040003

//
// MessageId: DISMAPI_E_INVALID_DISM_SESSION
//
// MessageText:
//
// An invalid DismSession handle was passed into a DISMAPI function
//
#define DISMAPI_E_INVALID_DISM_SESSION 0xC0040004

//
// MessageId: DISMAPI_E_INVALID_IMAGE_INDEX
//
// MessageText:
//
// An invalid image index was specified
//
#define DISMAPI_E_INVALID_IMAGE_INDEX 0xC0040005

//
// MessageId: DISMAPI_E_INVALID_IMAGE_NAME
//
// MessageText:
//
// An invalid image name was specified
//
#define DISMAPI_E_INVALID_IMAGE_NAME 0xC0040006

//
// MessageId: DISMAPI_E_UNABLE_TO_UNMOUNT_IMAGE_PATH
//
// MessageText:
//
// An image that is not a mounted WIM or mounted VHD was attempted to be unmounted
//
#define DISMAPI_E_UNABLE_TO_UNMOUNT_IMAGE_PATH 0xC0040007

//
// MessageId: DISMAPI_E_LOGGING_DISABLED
//
// MessageText:
//
// Failed to gain access to the log file user specified. Logging has been disabled..
//
#define DISMAPI_E_LOGGING_DISABLED 0xC0040009

//
// MessageId: DISMAPI_E_OPEN_HANDLES_UNABLE_TO_UNMOUNT_IMAGE_PATH
//
// MessageText:
//
// A DismSession with open handles was attempted to be unmounted
//
#define DISMAPI_E_OPEN_HANDLES_UNABLE_TO_UNMOUNT_IMAGE_PATH 0xC004000A

//
// MessageId: DISMAPI_E_OPEN_HANDLES_UNABLE_TO_MOUNT_IMAGE_PATH
//
// MessageText:
//
// A DismSession with open handles was attempted to be mounted
//
#define DISMAPI_E_OPEN_HANDLES_UNABLE_TO_MOUNT_IMAGE_PATH 0xC004000B

//
// MessageId: DISMAPI_E_OPEN_HANDLES_UNABLE_TO_REMOUNT_IMAGE_PATH
//
// MessageText:
//
// A DismSession with open handles was attempted to be remounted
//
#define DISMAPI_E_OPEN_HANDLES_UNABLE_TO_REMOUNT_IMAGE_PATH 0xC004000C

//
// MessageId: DISMAPI_E_PARENT_FEATURE_DISABLED
//
// MessageText:
//
// One or several parent features are disabled so current feature can not be enabled.
// Solutions:
// 1 Call function DismGetFeatureParent to get all parent features and enable all of them. Or
// 2 Set EnableAll to TRUE when calling function DismEnableFeature.
//
#define DISMAPI_E_PARENT_FEATURE_DISABLED 0xC004000D

//
// MessageId: DISMAPI_E_MUST_SPECIFY_ONLINE_IMAGE
//
// MessageText:
//
// The offline image specified is the running system. The macro DISM_ONLINE_IMAGE must be
// used instead.
//
#define DISMAPI_E_MUST_SPECIFY_ONLINE_IMAGE 0xC004000E

//
// MessageId: DISMAPI_E_INVALID_PRODUCT_KEY
//
// MessageText:
//
// The specified product key could not be validated. Check that the specified
// product key is valid and that it matches the target edition.
//
#define DISMAPI_E_INVALID_PRODUCT_KEY 0xC004000F

//
// MessageId: DISMAPI_E_NEEDS_TO_REMOUNT_THE_IMAGE
//
// MessageText:
//
// The image needs to be remounted before any servicing operation.
//
#define DISMAPI_E_NEEDS_REMOUNT 0XC1510114

//
// MessageId: DISMAPI_E_UNKNOWN_FEATURE
//
// MessageText:
//
// The feature is not present in the package.
//
#define DISMAPI_E_UNKNOWN_FEATURE 0x800f080c

//
// MessageId: DISMAPI_E_BUSY
//
// MessageText:
//
// The current package and feature servicing infrastructure is busy.  Wait a
// bit and try the operation again.
//
#define DISMAPI_E_BUSY 0x800f0902

#ifdef __cplusplus
    }
#endif

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_PKG_DISM) */
#pragma endregion

#endif // _DISMAPI_H_


    struct DismApiHelper
    {
        DismApiHelper();
        ~DismApiHelper();

        DismFeatureInfo* GetFeatureInfo(const std::string_view& name);
        HRESULT EnableFeature(const std::string_view& name);
        HRESULT DisableFeature(const std::string_view& name);

    private:
        using DismInitializePtr = HRESULT(WINAPI*)(int, PCWSTR, PCWSTR);
        using DismOpenSessionPtr = HRESULT(WINAPI*)(PCWSTR, PCWSTR, PCWSTR, UINT*);
        using DismShutdownPtr = HRESULT(WINAPI*)();
        using DismGetFeatureInfoPtr = HRESULT(WINAPI*)(UINT, PCWSTR, PCWSTR, DismPackageIdentifier, DismFeatureInfo**);
        using DismEnableFeaturePtr = HRESULT(WINAPI*)(UINT, PCWSTR, PCWSTR, DismPackageIdentifier, BOOL, PCWSTR*, UINT, BOOL, HANDLE, DISM_PROGRESS_CALLBACK, PVOID);
        using DismDisableFeaturePtr = HRESULT(WINAPI*)(UINT, PCWSTR, PCWSTR, BOOL, HANDLE, DISM_PROGRESS_CALLBACK, PVOID);
        using DismDeletePtr = HRESULT(WINAPI*)(VOID*);

        typedef UINT DismSession;

        wil::unique_hmodule m_module;
        DismSession m_session = DISM_SESSION_DEFAULT;
        DismFeatureInfo* m_featureInfo = nullptr;

        DismInitializePtr m_dismInitialize = nullptr;
        DismOpenSessionPtr m_dismOpenSession = nullptr;
        DismGetFeatureInfoPtr m_dismGetFeatureInfo = nullptr;
        DismEnableFeaturePtr m_dismEnableFeature = nullptr;
        DismDisableFeaturePtr m_dismDisableFeature = nullptr;
        DismShutdownPtr m_dismShutdown = nullptr;
        DismDeletePtr m_dismDelete = nullptr;

        void Initialize();
        void OpenSession();
        void Delete();
        void Shutdown();
    };

    struct WindowsFeature
    {
        WindowsFeature(const std::string& name) : m_featureName{ name } {};

        bool DoesExist();

        bool IsEnabled();

        HRESULT EnableFeature();

        HRESULT DisableFeature();

    private:
        DismApiHelper m_dismApiHelper;
        std::string m_featureName;
    };
}