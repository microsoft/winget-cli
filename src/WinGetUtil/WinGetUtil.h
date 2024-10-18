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

    DEFINE_ENUM_FLAG_OPERATORS(WinGetValidateManifestOption);

    enum WinGetCreateManifestOption
    {
        // Just create the manifest without any validation
        NoValidation = 0,
        // Only validate against json schema
        SchemaValidation = 0x1,
        // Validate against schema and also perform semantic validation
        SchemaAndSemanticValidation = 0x2,
        // Use shadow manifest
        AllowShadowManifest = 0x4,

        /// Below options are additional validation behaviors if needed

        // Return error on manifest fields that require verified publishers, used during semantic validation
        ReturnErrorOnVerifiedPublisherFields = 0x1000,
    };

    DEFINE_ENUM_FLAG_OPERATORS(WinGetCreateManifestOption);

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

    DEFINE_ENUM_FLAG_OPERATORS(WinGetValidateManifestOptionV2);

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

        // Dependencies validation result.
        SingleManifestPackageHasDependencies = 0x10000,
        MultiManifestPackageHasDependencies = 0x20000,
        MissingManifestDependenciesNode = 0x40000,
        NoSuitableMinVersionDependency = 0x80000,
        FoundDependencyLoop = 0x100000,

        // Internal error meaning validation does not complete as desired.
        InternalError = 0x1000,
    };

    DEFINE_ENUM_FLAG_OPERATORS(WinGetValidateManifestResult);

    enum WinGetValidateManifestDependenciesOption
    {
        DefaultValidation = 0,
        ForDelete = 0x1,
    };

    DEFINE_ENUM_FLAG_OPERATORS(WinGetValidateManifestDependenciesOption);

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

    // Migrates the index to the new version specified.
    WINGET_UTIL_API WinGetSQLiteIndexMigrate(
        WINGET_SQLITE_INDEX_HANDLE index,
        UINT32 majorVersion,
        UINT32 minorVersion);

    enum WinGetSQLiteIndexProperty
    {
        WinGetSQLiteIndexProperty_PackageUpdateTrackingBaseTime = 0,
        WinGetSQLiteIndexProperty_IntermediateFileOutputPath = 1,
    };

    // Sets the given property on the index.
    WINGET_UTIL_API WinGetSQLiteIndexSetProperty(
        WINGET_SQLITE_INDEX_HANDLE index,
        WinGetSQLiteIndexProperty property,
        WINGET_STRING value);

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

    // Adds or Updates the manifest with matching { Id, Version, Channel } in the index.
    // The return value indicates whether the manifest was added (true) or updated (false).
    WINGET_UTIL_API WinGetSQLiteIndexAddOrUpdateManifest(
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

    // A handle to the metadata collection object.
    typedef void* WINGET_INSTALLER_METADATA_COLLECTION_HANDLE;

    // Option flags for WinGetBeginInstallerMetadataCollection.
    enum WinGetBeginInstallerMetadataCollectionOptions
    {
        WinGetBeginInstallerMetadataCollectionOption_None = 0,
        // The inputJSON is a local file path, not a JSON string.
        WinGetBeginInstallerMetadataCollectionOption_InputIsFilePath = 0x1,
        // The inputJSON is a remote URI, not a JSON string.
        WinGetBeginInstallerMetadataCollectionOption_InputIsURI = 0x2,
    };

    DEFINE_ENUM_FLAG_OPERATORS(WinGetBeginInstallerMetadataCollectionOptions);

    // Begins the installer metadata collection process.
    // By default, inputJSON is expected to be a JSON string. See the WinGetBeginInstallerMetadataCollectionOptions for more options.
    // logFilePath optionally specifies where to write the log file for the collection operation.
    // The collectionHandle is owned by the caller and must be passed to WinGetCompleteInstallerMetadataCollection to free it.
    WINGET_UTIL_API WinGetBeginInstallerMetadataCollection(
        WINGET_STRING inputJSON,
        WINGET_STRING logFilePath,
        WinGetBeginInstallerMetadataCollectionOptions options,
        WINGET_INSTALLER_METADATA_COLLECTION_HANDLE* collectionHandle);

    // Option flags for WinGetCompleteInstallerMetadataCollection.
    enum WinGetCompleteInstallerMetadataCollectionOptions
    {
        WinGetCompleteInstallerMetadataCollectionOption_None = 0,
        // Complete will simply free the collection handle without doing any additional work.
        WinGetCompleteInstallerMetadataCollectionOption_Abandon = 0x1,
    };

    DEFINE_ENUM_FLAG_OPERATORS(WinGetCompleteInstallerMetadataCollectionOptions);

    // Completes the installer metadata collection process.
    // Always frees the collectionHandle; WinGetCompleteInstallerMetadataCollection must be called exactly once for each call to WinGetBeginInstallerMetadataCollection.
    WINGET_UTIL_API WinGetCompleteInstallerMetadataCollection(
        WINGET_INSTALLER_METADATA_COLLECTION_HANDLE collectionHandle,
        WINGET_STRING outputFilePath,
        WinGetCompleteInstallerMetadataCollectionOptions options);

    // Option flags for WinGetMergeInstallerMetadata.
    enum WinGetMergeInstallerMetadataOptions
    {
        WinGetMergeInstallerMetadataOptions_None = 0,
    };

    // Merges the given JSON metadata documents into a single one.
    WINGET_UTIL_API WinGetMergeInstallerMetadata(
        WINGET_STRING inputJSON,
        WINGET_STRING_OUT* outputJSON,
        UINT32 maximumOutputSizeInBytes,
        WINGET_STRING logFilePath,
        WinGetMergeInstallerMetadataOptions options);
}
