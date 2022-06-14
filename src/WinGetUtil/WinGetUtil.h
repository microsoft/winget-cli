// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

extern "C"
{
    // A handle to the index.
    typedef void* WINGET_SQLITE_INDEX_HANDLE;

    // A handle to the manifest.
    typedef void* WINGET_MANIFEST_HANDLE;

    // A string taken in by the utility; in UTF16.
    typedef wchar_t const* const WINGET_STRING;

    // A string returned by the utility; in UTF16.
    typedef BSTR WINGET_STRING_OUT;

#define WINGET_UTIL_API HRESULT __stdcall

#define WINGET_SQLITE_INDEX_VERSION_LATEST ((UINT32)-1)

    enum WinGetValidateManifestOption
    {
        Default = 0,
        SchemaValidationOnly = 0x1,
        ErrorOnVerifiedPublisherFields = 0x2,
        InstallerValidations = 0x4,
    };

    enum WinGetCreateManifestOption
    {
        // Just create the manifest without any validation
        NoValidation = 0,
        // Only validate against json schema
        SchemaValidation = 0x1,
        // Validate against schema and also perform semantic validation
        SchemaAndSemanticValidation = 0x2,

        /// Below options are additional validation behaviors if needed

        // Return error on manifest fields that require verified publishers, used during semantic validation
        ReturnErrorOnVerifiedPublisherFields = 0x1000,
    };

    enum WinGetValidateManifestOptionV2
    {
        // No validation, caller will get E_INVALIDARG
        None = 0,
        // Dependencies validation against index
        DependenciesValidation = 0x1,
        // Arp version validation against index
        ArpVersionValidation = 0x2,
        // Installer validation
        InstallerValidation = 0x4,
    };

    enum WinGetValidateManifestOperationType
    {
        OperationTypeAdd = 0,
        OperationTypeUpdate = 1,
        OperationTypeDelete = 2,
    };

    enum WinGetValidateManifestResult
    {
        Success = 0,

        // Each validation step should have an enum for corresponding failure.
        DependenciesValidationFailure = 0x1,
        ArpVersionValidationFailure = 0x2,
        InstallerValidationFailure = 0x4,

        // Internal error meaning validation does not complete as desired.
        InternalError = 0x1000,
    };

    enum WinGetValidateManifestDependenciesOption
    {
        DefaultValidation = 0,
        ForDelete = 0x1,
    };

    DEFINE_ENUM_FLAG_OPERATORS(WinGetValidateManifestOption);

    // Initializes the logging infrastructure.
    WINGET_UTIL_API WinGetLoggingInit(
        WINGET_STRING logPath);

    // Removes the given log file from the logging infrastructure.
    // If logPath is nullptr, then remove all loggers.
    WINGET_UTIL_API WinGetLoggingTerm(
        WINGET_STRING logPath);

    // Creates a new index file at filePath with the given version.
    WINGET_UTIL_API WinGetSQLiteIndexCreate(
        WINGET_STRING filePath, 
        UINT32 majorVersion,
        UINT32 minorVersion,
        WINGET_SQLITE_INDEX_HANDLE* index);

    // Opens an existing index at filePath.
    WINGET_UTIL_API WinGetSQLiteIndexOpen(
        WINGET_STRING filePath, 
        WINGET_SQLITE_INDEX_HANDLE* index);

    // Closes the index.
    WINGET_UTIL_API WinGetSQLiteIndexClose(
        WINGET_SQLITE_INDEX_HANDLE index);

    // Adds the manifest at the repository relative path to the index.
    // If the function succeeds, the manifest has been added.
    WINGET_UTIL_API WinGetSQLiteIndexAddManifest(
        WINGET_SQLITE_INDEX_HANDLE index, 
        WINGET_STRING manifestPath, 
        WINGET_STRING relativePath);

    // Updates the manifest with matching { Id, Version, Channel } in the index.
    // The return value indicates whether the index was modified by the function.
    WINGET_UTIL_API WinGetSQLiteIndexUpdateManifest(
        WINGET_SQLITE_INDEX_HANDLE index, 
        WINGET_STRING manifestPath, 
        WINGET_STRING relativePath,
        BOOL* indexModified);

    // Removes the manifest with matching { Id, Version, Channel } from the index.
    // Path is currently ignored.
    WINGET_UTIL_API WinGetSQLiteIndexRemoveManifest(
        WINGET_SQLITE_INDEX_HANDLE index, 
        WINGET_STRING manifestPath, 
        WINGET_STRING relativePath);

    // Removes data that is no longer needed for an index that is to be published.
    WINGET_UTIL_API WinGetSQLiteIndexPrepareForPackaging(
        WINGET_SQLITE_INDEX_HANDLE index);

    // Checks the index for consistency, ensuring that at a minimum all referenced rows actually exist.
    WINGET_UTIL_API WinGetSQLiteIndexCheckConsistency(
        WINGET_SQLITE_INDEX_HANDLE index,
        BOOL* succeeded);

    // Validates a given manifest. Returns a bool for validation result and
    // a string representing validation errors if validation failed.
    WINGET_UTIL_API WinGetValidateManifest(
        WINGET_STRING manifestPath,
        BOOL* succeeded,
        WINGET_STRING_OUT* message);

    // Validates a given manifest. Returns a bool for validation result and
    // a string representing validation errors if validation failed.
    // If mergedManifestPath is provided, this method will write a merged manifest
    // to the location specified by mergedManifestPath
    WINGET_UTIL_API WinGetValidateManifestV2(
        WINGET_STRING inputPath,
        BOOL* succeeded,
        WINGET_STRING_OUT* message,
        WINGET_STRING mergedManifestPath,
        WinGetValidateManifestOption option);

    // Creates a given manifest with optional validation. Returns a bool for operation result and
    // a string representing validation errors if validation failed.
    // If mergedManifestPath is provided, this method will write a merged manifest
    // to the location specified by mergedManifestPath
    WINGET_UTIL_API WinGetCreateManifest(
        WINGET_STRING inputPath,
        BOOL* succeeded,
        WINGET_MANIFEST_HANDLE* manifest,
        WINGET_STRING_OUT* message,
        WINGET_STRING mergedManifestPath,
        WinGetCreateManifestOption option);

    // Closes a given manifest.
    WINGET_UTIL_API WinGetCloseManifest(
        WINGET_MANIFEST_HANDLE manifest);

    // Validates a given manifest. Returns WinGetValidateManifestResult for validation result and
    // a string representing validation errors if validation failed.
    // If result is 0, it is success. Otherwise, caller can check the result with flags to see
    // which phases failed.
    WINGET_UTIL_API WinGetValidateManifestV3(
        WINGET_MANIFEST_HANDLE manifest,
        WINGET_SQLITE_INDEX_HANDLE index,
        WinGetValidateManifestResult* result,
        WINGET_STRING_OUT* message,
        WinGetValidateManifestOptionV2 option,
        WinGetValidateManifestOperationType operationType);

    // Validates a given manifest with dependencies. Returns a bool for validation result and
    // a string representing validation errors if validation failed.
    // If mergedManifestPath is provided, this method will write a merged manifest
    // to the location specified by mergedManifestPath
    WINGET_UTIL_API WinGetValidateManifestDependencies(
        WINGET_STRING inputPath,
        BOOL* succeeded,
        WINGET_STRING_OUT* message,
        WINGET_SQLITE_INDEX_HANDLE index,
        WinGetValidateManifestDependenciesOption dependenciesValidationOption);

    // Downloads a file to the given path, returning the SHA 256 hash of the file.
    WINGET_UTIL_API WinGetDownload(
        WINGET_STRING url,
        WINGET_STRING filePath,
        BYTE* sha256Hash,
        UINT32 sha256HashLength);

    // Compares two version strings, returning -1 if versionA is less than versionB, 0 if they're equal, or 1 if versionA is greater than versionB
    WINGET_UTIL_API WinGetCompareVersions(
        WINGET_STRING versionA,
        WINGET_STRING versionB,
        INT* comparisonResult);
}
