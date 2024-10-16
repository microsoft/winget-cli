// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "WinGetUtil.h"

#include <AppInstallerDownloader.h>
#include <AppInstallerFileLogger.h>
#include <AppInstallerLogging.h>
#include <AppInstallerStrings.h>
#include <AppInstallerTelemetry.h>
#include <Microsoft/SQLiteIndex.h>
#include <winget/ManifestYamlParser.h>
#include <winget/ThreadGlobals.h>
#include <winget/InstallerMetadataCollectionContext.h>
#include <PackageDependenciesValidation.h>
#include <public/winget/PackageDependenciesValidationUtil.h>
#include <ArpVersionValidation.h>

using namespace AppInstaller::Utility;
using namespace AppInstaller::Manifest;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Repository::Metadata;
using namespace AppInstaller::Repository::Microsoft;

namespace
{
    std::filesystem::path GetPathOrEmpty(WINGET_STRING potentiallyNullPath)
    {
        return potentiallyNullPath ? std::filesystem::path{ potentiallyNullPath } : std::filesystem::path{};
    }

    SQLiteIndex::Property GetSQLiteIndexProperty(WinGetSQLiteIndexProperty property)
    {
        switch (property)
        {
        case WinGetSQLiteIndexProperty_PackageUpdateTrackingBaseTime: return SQLiteIndex::Property::PackageUpdateTrackingBaseTime;
        case WinGetSQLiteIndexProperty_IntermediateFileOutputPath: return SQLiteIndex::Property::IntermediateFileOutputPath;
        }

        THROW_HR(E_INVALIDARG);
    }
}

extern "C"
{
    WINGET_UTIL_API WinGetLoggingInit(WINGET_STRING logPath) try
    {
        THROW_HR_IF(E_INVALIDARG, !logPath);

        thread_local AppInstaller::ThreadLocalStorage::WingetThreadGlobals threadGlobals;
        thread_local std::once_flag initLogging;

        std::call_once(initLogging, []() {
            std::unique_ptr<AppInstaller::ThreadLocalStorage::PreviousThreadGlobals> previous = threadGlobals.SetForCurrentThread();
            // Intentionally release to leave the local ThreadGlobals.
            previous.release();
            // Enable all logs for now.
            AppInstaller::Logging::Log().EnableChannel(AppInstaller::Logging::Channel::All);
            AppInstaller::Logging::Log().SetLevel(AppInstaller::Logging::Level::Verbose);
            AppInstaller::Logging::EnableWilFailureTelemetry();
            });

        std::filesystem::path pathAsPath = logPath;
        std::string loggerName = AppInstaller::Logging::FileLogger::GetNameForPath(pathAsPath);

        if (!AppInstaller::Logging::Log().ContainsLogger(loggerName))
        {
            // Let FileLogger use default file prefix
            AppInstaller::Logging::FileLogger::Add(pathAsPath);
        }

        return S_OK;
    }
    CATCH_RETURN()

    WINGET_UTIL_API WinGetLoggingTerm(WINGET_STRING logPath) try
    {
        if (logPath)
        {
            std::string loggerName = AppInstaller::Logging::FileLogger::GetNameForPath(logPath);
            (void)AppInstaller::Logging::Log().RemoveLogger(loggerName);
        }
        else
        {
            AppInstaller::Logging::Log().RemoveAllLoggers();
        }

        return S_OK;
    }
    CATCH_RETURN()

    WINGET_UTIL_API WinGetSQLiteIndexCreate(WINGET_STRING filePath, UINT32 majorVersion, UINT32 minorVersion, WINGET_SQLITE_INDEX_HANDLE* index) try
    {
        THROW_HR_IF(E_INVALIDARG, !filePath);
        THROW_HR_IF(E_INVALIDARG, !index);
        THROW_HR_IF(E_INVALIDARG, !!*index);

        std::string filePathUtf8 = ConvertToUTF8(filePath);
        AppInstaller::SQLite::Version internalVersion{ majorVersion, minorVersion };

        std::unique_ptr<SQLiteIndex> result = std::make_unique<SQLiteIndex>(SQLiteIndex::CreateNew(filePathUtf8, internalVersion));

        *index = static_cast<WINGET_SQLITE_INDEX_HANDLE>(result.release());

        return S_OK;
    }
    CATCH_RETURN()

    WINGET_UTIL_API WinGetSQLiteIndexOpen(WINGET_STRING filePath, WINGET_SQLITE_INDEX_HANDLE* index) try
    {
        THROW_HR_IF(E_INVALIDARG, !filePath);
        THROW_HR_IF(E_INVALIDARG, !index);
        THROW_HR_IF(E_INVALIDARG, !!*index);

        std::string filePathUtf8 = ConvertToUTF8(filePath);

        std::unique_ptr<SQLiteIndex> result = std::make_unique<SQLiteIndex>(SQLiteIndex::Open(filePathUtf8, SQLiteIndex::OpenDisposition::ReadWrite));

        *index = static_cast<WINGET_SQLITE_INDEX_HANDLE>(result.release());

        return S_OK;
    }
    CATCH_RETURN()

    WINGET_UTIL_API WinGetSQLiteIndexClose(WINGET_SQLITE_INDEX_HANDLE index) try
    {
        std::unique_ptr<SQLiteIndex> toClose(reinterpret_cast<SQLiteIndex*>(index));

        return S_OK;
    }
    CATCH_RETURN()

    WINGET_UTIL_API WinGetSQLiteIndexMigrate(
        WINGET_SQLITE_INDEX_HANDLE index,
        UINT32 majorVersion,
        UINT32 minorVersion) try
    {
        THROW_HR_IF(E_INVALIDARG, !index);

        return reinterpret_cast<SQLiteIndex*>(index)->MigrateTo({ majorVersion, minorVersion }) ? S_OK : HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
    }
    CATCH_RETURN()


    WINGET_UTIL_API WinGetSQLiteIndexSetProperty(
        WINGET_SQLITE_INDEX_HANDLE index,
        WinGetSQLiteIndexProperty property,
        WINGET_STRING value) try
    {
        THROW_HR_IF(E_INVALIDARG, !index);
        THROW_HR_IF(E_INVALIDARG, !value);

        std::string valueUtf8 = ConvertToUTF8(value);

        reinterpret_cast<SQLiteIndex*>(index)->SetProperty(GetSQLiteIndexProperty(property), valueUtf8);

        return S_OK;
    }
    CATCH_RETURN()

    WINGET_UTIL_API WinGetSQLiteIndexAddManifest(
        WINGET_SQLITE_INDEX_HANDLE index, 
        WINGET_STRING manifestPath, WINGET_STRING relativePath) try
    {
        THROW_HR_IF(E_INVALIDARG, !index);
        THROW_HR_IF(E_INVALIDARG, !manifestPath);
        THROW_HR_IF(E_INVALIDARG, !relativePath);

        reinterpret_cast<SQLiteIndex*>(index)->AddManifest(manifestPath, relativePath);

        return S_OK;
    }
    CATCH_RETURN()

    WINGET_UTIL_API WinGetSQLiteIndexUpdateManifest(
        WINGET_SQLITE_INDEX_HANDLE index,
        WINGET_STRING manifestPath,
        WINGET_STRING relativePath,
        BOOL* indexModified) try
    {
        THROW_HR_IF(E_INVALIDARG, !index);
        THROW_HR_IF(E_INVALIDARG, !manifestPath);
        THROW_HR_IF(E_INVALIDARG, !relativePath);

        bool result = reinterpret_cast<SQLiteIndex*>(index)->UpdateManifest(manifestPath, relativePath);
        if (indexModified)
        {
            *indexModified = (result ? TRUE : FALSE);
        }

        return S_OK;
    }
    CATCH_RETURN()

    WINGET_UTIL_API WinGetSQLiteIndexAddOrUpdateManifest(
        WINGET_SQLITE_INDEX_HANDLE index,
        WINGET_STRING manifestPath,
        WINGET_STRING relativePath,
        BOOL* indexModified) try
    {
        THROW_HR_IF(E_INVALIDARG, !index);
        THROW_HR_IF(E_INVALIDARG, !manifestPath);
        THROW_HR_IF(E_INVALIDARG, !relativePath);

        bool result = reinterpret_cast<SQLiteIndex*>(index)->AddOrUpdateManifest(manifestPath, relativePath);
        if (indexModified)
        {
            *indexModified = (result ? TRUE : FALSE);
        }

        return S_OK;
    }
    CATCH_RETURN()

    WINGET_UTIL_API WinGetSQLiteIndexRemoveManifest(
        WINGET_SQLITE_INDEX_HANDLE index, 
        WINGET_STRING manifestPath,
        WINGET_STRING relativePath) try
    {
        THROW_HR_IF(E_INVALIDARG, !index);
        THROW_HR_IF(E_INVALIDARG, !manifestPath);
        THROW_HR_IF(E_INVALIDARG, !relativePath);

        reinterpret_cast<SQLiteIndex*>(index)->RemoveManifest(manifestPath, relativePath);

        return S_OK;
    }
    CATCH_RETURN()

    WINGET_UTIL_API WinGetSQLiteIndexPrepareForPackaging(
        WINGET_SQLITE_INDEX_HANDLE index) try
    {
        THROW_HR_IF(E_INVALIDARG, !index);

        reinterpret_cast<SQLiteIndex*>(index)->PrepareForPackaging();

        return S_OK;
    }
    CATCH_RETURN()

    WINGET_UTIL_API WinGetSQLiteIndexCheckConsistency(
        WINGET_SQLITE_INDEX_HANDLE index,
        BOOL* succeeded) try
    {
        THROW_HR_IF(E_INVALIDARG, !index);
        THROW_HR_IF(E_INVALIDARG, !succeeded);

        auto sqliteIndex = reinterpret_cast<SQLiteIndex*>(index);
        bool result = sqliteIndex->CheckConsistency(true);

        *succeeded = (result ? TRUE : FALSE);

        return S_OK;
    }
    CATCH_RETURN()

    WINGET_UTIL_API WinGetValidateManifest(
        WINGET_STRING manifestPath,
        BOOL* succeeded,
        WINGET_STRING_OUT* message) try
    {
        THROW_HR_IF(E_INVALIDARG, !manifestPath);
        THROW_HR_IF(E_INVALIDARG, !succeeded);

        try
        {
            ManifestValidateOption validateOption;
            validateOption.FullValidation = true;
            validateOption.ThrowOnWarning = true;

            (void)YamlParser::CreateFromPath(manifestPath, validateOption);

            *succeeded = TRUE;
        }
        catch (const ManifestException& e)
        {
            *succeeded = e.IsWarningOnly();
            if (message)
            {
                *message = ::SysAllocString(ConvertToUTF16(e.GetManifestErrorMessage()).c_str());
            }
        }

        return S_OK;
    }
    CATCH_RETURN()

    WINGET_UTIL_API WinGetValidateManifestV2(
        WINGET_STRING inputPath,
        BOOL* succeeded,
        WINGET_STRING_OUT* message,
        WINGET_STRING mergedManifestPath,
        WinGetValidateManifestOption option) try
    {
        THROW_HR_IF(E_INVALIDARG, !inputPath);
        THROW_HR_IF(E_INVALIDARG, !succeeded);

        try
        {
            ManifestValidateOption validateOption;
            validateOption.FullValidation = true;
            validateOption.ThrowOnWarning = true;
            validateOption.SchemaValidationOnly = WI_IsFlagSet(option, WinGetValidateManifestOption::SchemaValidationOnly);
            validateOption.ErrorOnVerifiedPublisherFields = WI_IsFlagSet(option, WinGetValidateManifestOption::ErrorOnVerifiedPublisherFields);
            validateOption.InstallerValidation = WI_IsFlagSet(option, WinGetValidateManifestOption::InstallerValidations);

            (void)YamlParser::CreateFromPath(inputPath, validateOption, mergedManifestPath ? mergedManifestPath : L"");

            *succeeded = TRUE;
        }
        catch (const ManifestException& e)
        {
            *succeeded = e.IsWarningOnly();
            if (message)
            {
                *message = ::SysAllocString(ConvertToUTF16(e.GetManifestErrorMessage()).c_str());
            }
        }

        return S_OK;
    }
    CATCH_RETURN()

    WINGET_UTIL_API WinGetCreateManifest(
        WINGET_STRING inputPath,
        BOOL* succeeded,
        WINGET_MANIFEST_HANDLE* manifest,
        WINGET_STRING_OUT* message,
        WINGET_STRING mergedManifestPath,
        WinGetCreateManifestOption option) try
    {
        THROW_HR_IF(E_INVALIDARG, !inputPath);
        THROW_HR_IF(E_INVALIDARG, !succeeded);
        THROW_HR_IF(E_INVALIDARG, !!*manifest);
        // ErrorOnVerifiedPublisherFields can only be used with SchemaAndSemanticValidation
        THROW_HR_IF(E_INVALIDARG, (WI_IsFlagSet(option, WinGetCreateManifestOption::ReturnErrorOnVerifiedPublisherFields) && WI_IsFlagClear(option, WinGetCreateManifestOption::SchemaAndSemanticValidation)));

        *succeeded = false;
        *manifest = nullptr;

        try
        {
            ManifestValidateOption validateOption;

            if (WI_IsFlagSet(option, WinGetCreateManifestOption::SchemaValidation) || WI_IsFlagSet(option, WinGetCreateManifestOption::SchemaAndSemanticValidation))
            {
                validateOption.FullValidation = true;
                validateOption.ThrowOnWarning = true;
                validateOption.SchemaValidationOnly = WI_IsFlagClear(option, WinGetCreateManifestOption::SchemaAndSemanticValidation);
                validateOption.ErrorOnVerifiedPublisherFields = WI_IsFlagSet(option, WinGetCreateManifestOption::ReturnErrorOnVerifiedPublisherFields);
            }

            if (WI_IsFlagSet(option, WinGetCreateManifestOption::AllowShadowManifest))
            {
                validateOption.AllowShadowManifest = true;
            }

            std::unique_ptr<Manifest> result = std::make_unique<Manifest>(YamlParser::CreateFromPath(inputPath, validateOption, mergedManifestPath ? mergedManifestPath : L""));

            *manifest = static_cast<WINGET_MANIFEST_HANDLE>(result.release());
            *succeeded = true;
        }
        catch (const ManifestException& e)
        {
            *succeeded = e.IsWarningOnly();
            if (*succeeded)
            {
                ManifestValidateOption validateOption;
                if (WI_IsFlagSet(option, WinGetCreateManifestOption::AllowShadowManifest))
                {
                    validateOption.AllowShadowManifest = true;
                }

                std::unique_ptr<Manifest> result = std::make_unique<Manifest>(YamlParser::CreateFromPath(inputPath, validateOption));
                *manifest = static_cast<WINGET_MANIFEST_HANDLE>(result.release());
            }
            if (message)
            {
                *message = ::SysAllocString(ConvertToUTF16(e.GetManifestErrorMessage()).c_str());
            }
        }

        return S_OK;
    }
    CATCH_RETURN()

    WINGET_UTIL_API WinGetCloseManifest(
        WINGET_MANIFEST_HANDLE manifest) try
    {
        THROW_HR_IF(E_INVALIDARG, !manifest);

        std::unique_ptr<Manifest> toClose{ reinterpret_cast<Manifest*>(manifest) };

        return S_OK;
    }
    CATCH_RETURN()

    WINGET_UTIL_API WinGetValidateManifestV3(
        WINGET_MANIFEST_HANDLE manifest,
        WINGET_SQLITE_INDEX_HANDLE index,
        WinGetValidateManifestResult* result,
        WINGET_STRING_OUT* message,
        WinGetValidateManifestOptionV2 option,
        WinGetValidateManifestOperationType operationType) try
    {
        THROW_HR_IF(E_INVALIDARG, !manifest);
        THROW_HR_IF(E_INVALIDARG, !result);
        // Index should be provided if DependenciesValidation or ArpVersionValidation is to be performed
        THROW_HR_IF(E_INVALIDARG, !index && (WI_IsFlagSet(option, WinGetValidateManifestOptionV2::DependenciesValidation) || WI_IsFlagSet(option, WinGetValidateManifestOptionV2::ArpVersionValidation)));
        THROW_HR_IF(E_INVALIDARG, option == WinGetValidateManifestOptionV2::None);
        
        *result = WinGetValidateManifestResult::InternalError;

        std::string validationMessage;
        auto validationResult = WinGetValidateManifestResult::Success;

        Manifest* manifestPtr = reinterpret_cast<Manifest*>(manifest);
        SQLiteIndex* sqliteIndex = reinterpret_cast<SQLiteIndex*>(index);

        if (WI_IsFlagSet(option, WinGetValidateManifestOptionV2::DependenciesValidation))
        {
            try
            {
                if (operationType == WinGetValidateManifestOperationType::OperationTypeDelete)
                {
                    PackageDependenciesValidation::VerifyDependenciesStructureForManifestDelete(sqliteIndex, *manifestPtr);
                }
                else
                {
                    PackageDependenciesValidation::ValidateManifestDependencies(sqliteIndex, *manifestPtr);
                }
            }
            catch (const ManifestException& e)
            {
                if (!e.IsWarningOnly())
                {
                    validationResult |= WinGetValidateManifestResult::DependenciesValidationFailure;
                }
                
                validationResult |= static_cast<WinGetValidateManifestResult>( AppInstaller::Manifest::GetDependenciesValidationResultFromException(e) );
              
                if (message)
                {
                    validationMessage += e.GetManifestErrorMessage();
                }
            }
        }
        
        if (WI_IsFlagSet(option, WinGetValidateManifestOptionV2::ArpVersionValidation))
        {
            try
            {
                ValidateManifestArpVersion(sqliteIndex, *manifestPtr);
            }
            catch (const ManifestException& e)
            {
                WI_SetFlagIf(validationResult, WinGetValidateManifestResult::ArpVersionValidationFailure, !e.IsWarningOnly());
                if (message)
                {
                    validationMessage += e.GetManifestErrorMessage();
                }
            }
        }

        if (WI_IsFlagSet(option, WinGetValidateManifestOptionV2::InstallerValidation))
        {
            try
            {
                auto errors = ValidateManifestInstallers(*manifestPtr);
                if (errors.size() > 0)
                {
                    // Throw the errors as ManifestExceptions to get processed errors and message.
                    THROW_EXCEPTION(ManifestException({ std::move(errors) }));
                }
            }
            catch (const ManifestException& e)
            {
                WI_SetFlagIf(validationResult, WinGetValidateManifestResult::InstallerValidationFailure, !e.IsWarningOnly());
                if (message)
                {
                    validationMessage += e.GetManifestErrorMessage();
                }
            }
        }

        *result = validationResult;
        if (message)
        {
            *message = ::SysAllocString(ConvertToUTF16(validationMessage).c_str());
        }

        return S_OK;
    }
    CATCH_RETURN()

    WINGET_UTIL_API WinGetValidateManifestDependencies(
        WINGET_STRING inputPath,
        BOOL* succeeded,
        WINGET_STRING_OUT* message,
        WINGET_SQLITE_INDEX_HANDLE index,
        WinGetValidateManifestDependenciesOption validationOption) try
    {
        THROW_HR_IF(E_INVALIDARG, !inputPath);
        THROW_HR_IF(E_INVALIDARG, !succeeded);

        try
        {
            Manifest manifest = YamlParser::CreateFromPath(inputPath);
            SQLiteIndex* sqliteIndex(reinterpret_cast<SQLiteIndex*>(index));
            
            switch (validationOption)
            {
                case WinGetValidateManifestDependenciesOption::DefaultValidation:
                    PackageDependenciesValidation::ValidateManifestDependencies(sqliteIndex, manifest);
                    break;
                case WinGetValidateManifestDependenciesOption::ForDelete:
                    PackageDependenciesValidation::VerifyDependenciesStructureForManifestDelete(sqliteIndex, manifest);
                    break;
                default:
                    THROW_HR(E_INVALIDARG);
            }

            *succeeded = TRUE;
        }
        catch (const ManifestException& e)
        {
            *succeeded = e.IsWarningOnly();
            if (message)
            {
                *message = ::SysAllocString(ConvertToUTF16(e.GetManifestErrorMessage()).c_str());
            }
        }

        return S_OK;
    }
    CATCH_RETURN()

    WINGET_UTIL_API WinGetDownload(
        WINGET_STRING url,
        WINGET_STRING filePath,
        BYTE* sha256Hash,
        UINT32 sha256HashLength) try
    {
        THROW_HR_IF(E_INVALIDARG, !url);
        THROW_HR_IF(E_INVALIDARG, !filePath);

        bool computeHash = sha256Hash != nullptr && sha256HashLength != 0;
        THROW_HR_IF(E_INVALIDARG, !computeHash && (sha256Hash != nullptr || sha256HashLength != 0));
        THROW_HR_IF(E_INVALIDARG, computeHash && sha256HashLength != 32);

        AppInstaller::ProgressCallback callback;
        auto downloadResult = Download(ConvertToUTF8(url), filePath, DownloadType::WinGetUtil, callback);

        // At this point, if computeHash is set we have verified that the buffer is valid and 32 bytes.
        if (computeHash)
        {
            const auto& hash = downloadResult.Sha256Hash;

            // The SHA 256 hash length should always be 32 bytes.
            THROW_HR_IF(E_UNEXPECTED, hash.size() != sha256HashLength);
            std::copy(hash.begin(), hash.end(), sha256Hash);
        }

        return S_OK;
    }
    CATCH_RETURN()

    WINGET_UTIL_API WinGetCompareVersions(
        WINGET_STRING versionA,
        WINGET_STRING versionB,
        INT* comparisonResult) try
    {
        THROW_HR_IF(E_INVALIDARG, !versionA);
        THROW_HR_IF(E_INVALIDARG, !versionB);

        Version vA{ ConvertToUTF8(versionA) };
        Version vB{ ConvertToUTF8(versionB) };

        *comparisonResult = vA < vB ? -1 : (vA == vB ? 0 : 1);

        return S_OK;
    }
    CATCH_RETURN()

    WINGET_UTIL_API WinGetBeginInstallerMetadataCollection(
        WINGET_STRING inputJSON,
        WINGET_STRING logFilePath,
        WinGetBeginInstallerMetadataCollectionOptions options,
        WINGET_INSTALLER_METADATA_COLLECTION_HANDLE* collectionHandle) try
    {
        THROW_HR_IF(E_INVALIDARG, !inputJSON);
        THROW_HR_IF(E_INVALIDARG, !collectionHandle);
        THROW_HR_IF(E_INVALIDARG, !!*collectionHandle);
        // Flags specifying what inputJSON means are mutually exclusive
        THROW_HR_IF(E_INVALIDARG, !WI_IsClearOrSingleFlagSetInMask(options,
            WinGetBeginInstallerMetadataCollectionOption_InputIsFilePath | WinGetBeginInstallerMetadataCollectionOption_InputIsURI));

        std::unique_ptr<InstallerMetadataCollectionContext> result;

        if (WI_IsFlagSet(options, WinGetBeginInstallerMetadataCollectionOption_InputIsFilePath))
        {
            result = InstallerMetadataCollectionContext::FromFile(inputJSON, GetPathOrEmpty(logFilePath));
        }
        else if (WI_IsFlagSet(options, WinGetBeginInstallerMetadataCollectionOption_InputIsURI))
        {
            result = InstallerMetadataCollectionContext::FromURI(inputJSON, GetPathOrEmpty(logFilePath));
        }
        else
        {
            result = InstallerMetadataCollectionContext::FromJSON(inputJSON, GetPathOrEmpty(logFilePath));
        }

        *collectionHandle = static_cast<WINGET_INSTALLER_METADATA_COLLECTION_HANDLE>(result.release());

        return S_OK;
    }
    CATCH_RETURN()

    WINGET_UTIL_API WinGetCompleteInstallerMetadataCollection(
        WINGET_INSTALLER_METADATA_COLLECTION_HANDLE collectionHandle,
        WINGET_STRING outputFilePath,
        WinGetCompleteInstallerMetadataCollectionOptions options) try
    {
        THROW_HR_IF(E_INVALIDARG, !collectionHandle);

        // Since we always free the handle from calling this function, we can just store it in a unique_ptr from the start
        std::unique_ptr<InstallerMetadataCollectionContext> context{ reinterpret_cast<InstallerMetadataCollectionContext*>(collectionHandle) };

        if (WI_IsFlagSet(options, WinGetCompleteInstallerMetadataCollectionOption_Abandon))
        {
            return S_OK;
        }

        THROW_HR_IF(E_INVALIDARG, !outputFilePath);

        context->Complete(outputFilePath);

        return S_OK;
    }
    CATCH_RETURN()

    WINGET_UTIL_API WinGetMergeInstallerMetadata(
        WINGET_STRING inputJSON,
        WINGET_STRING_OUT* outputJSON,
        UINT32 maximumOutputSizeInBytes,
        WINGET_STRING logFilePath,
        WinGetMergeInstallerMetadataOptions) try
    {
        THROW_HR_IF(E_INVALIDARG, !inputJSON);
        THROW_HR_IF(E_INVALIDARG, !outputJSON);

        std::wstring merged = InstallerMetadataCollectionContext::Merge(inputJSON, maximumOutputSizeInBytes, GetPathOrEmpty(logFilePath));
        *outputJSON = ::SysAllocString(merged.c_str());

        return S_OK;
    }
    CATCH_RETURN()
}
