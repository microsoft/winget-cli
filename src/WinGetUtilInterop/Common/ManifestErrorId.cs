// -----------------------------------------------------------------------------
// <copyright file="ManifestErrorId.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGetUtil.Common
{
    /// <summary>
    /// Identifies a specific manifest validation error or warning.
    /// Each value corresponds to a <c>WINGET_DEFINE_RESOURCE_STRINGID</c> entry in
    /// <c>src/AppInstallerCommonCore/Public/winget/ManifestValidation.h</c> (ManifestError namespace)
    /// and an entry in the <c>ErrorIdToMessageMap</c> in
    /// <c>src/AppInstallerCommonCore/Manifest/ManifestValidation.cpp</c>.
    /// When adding, removing, or renaming an error ID, all three locations must be updated.
    /// </summary>
    public enum ManifestErrorId
    {
        /// <summary>
        /// The error ID was not recognized (e.g. defined in a newer native version).
        /// </summary>
        Unknown = 0,

        /// <summary>Approximate version not allowed.</summary>
        ApproximateVersionNotAllowed,

        /// <summary>Arp Validation Error.</summary>
        ArpValidationError,

        /// <summary>DisplayVersion declared in the manifest has overlap with existing DisplayVersion range in the index. Existing DisplayVersion range in index: </summary>
        ArpVersionOverlapWithIndex,

        /// <summary>Internal error while validating DisplayVersion against index.</summary>
        ArpVersionValidationInternalError,

        /// <summary>Contains a blocked MSI property.</summary>
        BlockedMsiProperty,

        /// <summary>Both AllowedMarkets and ExcludedMarkets defined.</summary>
        BothAllowedAndExcludedMarketsDefined,

        /// <summary>Installer switch contains network address.</summary>
        ContainsNetworkAddress,

        /// <summary>Duplicate portable command alias found.</summary>
        DuplicatePortableCommandAlias,

        /// <summary>Duplicate relative file path found.</summary>
        DuplicateRelativeFilePath,

        /// <summary>The multi file manifest contains duplicate PackageLocale.</summary>
        DuplicateMultiFileManifestLocale,

        /// <summary>The multi file manifest should contain only one file with the particular ManifestType.</summary>
        DuplicateMultiFileManifestType,

        /// <summary>Duplicate installer entry found.</summary>
        DuplicateInstallerEntry,

        /// <summary>Multiple Installer URLs found with the same InstallerSha256. Please ensure the accuracy of the URLs.</summary>
        DuplicateInstallerHash,

        /// <summary>Duplicate installer return code found.</summary>
        DuplicateReturnCodeEntry,

        /// <summary>Only zero or one entry for Apps and Features may be specified for InstallerType portable.</summary>
        ExceededAppsAndFeaturesEntryLimit,

        /// <summary>Only zero or one value for Commands may be specified for InstallerType portable.</summary>
        ExceededCommandsLimit,

        /// <summary>Only one entry for NestedInstallerFiles can be specified for non-portable InstallerTypes.</summary>
        ExceededNestedInstallerFilesLimit,

        /// <summary>Silent and SilentWithProgress switches are not specified for InstallerType exe. Please make sure the installer can run unattended.</summary>
        ExeInstallerMissingSilentSwitches,

        /// <summary>Duplicate field found in the manifest.</summary>
        FieldDuplicate,

        /// <summary>Failed to process field.</summary>
        FieldFailedToProcess,

        /// <summary>All field names should be PascalCased.</summary>
        FieldIsNotPascalCase,

        /// <summary>Field is not supported.</summary>
        FieldNotSupported,

        /// <summary>Field usage requires verified publishers.</summary>
        FieldRequireVerifiedPublisher,

        /// <summary>Unknown field.</summary>
        FieldUnknown,

        /// <summary>Field value is not supported.</summary>
        FieldValueNotSupported,

        /// <summary>Loop found.</summary>
        FoundDependencyLoop,

        /// <summary>The multi file manifest is incomplete. A multi file manifest must contain at least version, installer and defaultLocale manifest.</summary>
        IncompleteMultiFileManifest,

        /// <summary>The values of InstallerSha256 do not match for all instances of the same InstallerUrl.</summary>
        InconsistentInstallerHash,

        /// <summary>DefaultLocale value in version manifest does not match PackageLocale value in defaultLocale manifest.</summary>
        InconsistentMultiFileManifestDefaultLocale,

        /// <summary>The multi file manifest has inconsistent field values.</summary>
        InconsistentMultiFileManifestFieldValue,

        /// <summary>Failed to process installer.</summary>
        InstallerFailedToProcess,

        /// <summary>Inconsistent value in the manifest.</summary>
        InstallerMsixInconsistencies,

        /// <summary>The specified installer type does not support PackageFamilyName.</summary>
        InstallerTypeDoesNotSupportPackageFamilyName,

        /// <summary>The specified installer type does not support ProductCode.</summary>
        InstallerTypeDoesNotSupportProductCode,

        /// <summary>The specified installer type does not write to Apps and Features entry.</summary>
        InstallerTypeDoesNotWriteAppsAndFeaturesEntry,

        /// <summary>The locale value is not a well formed bcp47 language tag.</summary>
        InvalidBcp47Value,

        /// <summary>Invalid field value.</summary>
        InvalidFieldValue,

        /// <summary>Contains invalid MSI switches.</summary>
        InvalidMsiSwitches,

        /// <summary>Encountered unexpected root node.</summary>
        InvalidRootNode,

        /// <summary>The provided value is not a valid Windows feature name.</summary>
        InvalidWindowsFeatureName,

        /// <summary>Dependency not found: </summary>
        MissingManifestDependenciesNode,

        /// <summary>Failed to calculate MSIX signature hash. Please verify that the input file is a valid, signed MSIX.</summary>
        MsixSignatureHashFailed,

        /// <summary>Deleting the manifest will break the following dependencies.</summary>
        MultiManifestPackageHasDependencies,

        /// <summary>No Suitable Minimum Version: </summary>
        NoSuitableMinVersionDependency,

        /// <summary>No supported platforms.</summary>
        NoSupportedPlatforms,

        /// <summary>Optional field missing.</summary>
        OptionalFieldMissing,

        /// <summary>Relative file path must not point to a location outside of archive directory.</summary>
        RelativeFilePathEscapesDirectory,

        /// <summary>Required field with empty value.</summary>
        RequiredFieldEmpty,

        /// <summary>Required field missing.</summary>
        RequiredFieldMissing,

        /// <summary>Schema Error.</summary>
        SchemaError,

        /// <summary>Scope is not supported for InstallerType portable.</summary>
        ScopeNotSupported,

        /// <summary>Shadow manifest is not allowed.</summary>
        ShadowManifestNotAllowed,

        /// <summary>Package has a single manifest and is a dependency of other manifests.</summary>
        SingleManifestPackageHasDependencies,

        /// <summary>The multi file manifest should not contain file with the particular ManifestType.</summary>
        UnsupportedMultiFileManifestType,

        /// <summary>Schema header not found.</summary>
        SchemaHeaderNotFound,

        /// <summary>The schema header is invalid. Please verify that the schema header is present and formatted correctly.</summary>
        InvalidSchemaHeader,

        /// <summary>The manifest type in the schema header does not match the ManifestType property value in the manifest.</summary>
        SchemaHeaderManifestTypeMismatch,

        /// <summary>The manifest version in the schema header does not match the ManifestVersion property value in the manifest.</summary>
        SchemaHeaderManifestVersionMismatch,

        /// <summary>The schema header URL does not match the expected pattern.</summary>
        SchemaHeaderUrlPatternMismatch,

        /// <summary>The file type of the referenced file is not allowed.</summary>
        InvalidPortableFiletype,

        /// <summary>The file type of the referenced file is not a supported font file type.</summary>
        InvalidFontFiletype,
    }
}
