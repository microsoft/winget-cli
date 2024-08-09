// -----------------------------------------------------------------------------
// <copyright file="Enums.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGetUtil.Common
{
    using System;

    /// <summary>
    /// Options for creating a manifest for validation.
    /// Must be in sync with WinGetCreateManifestOption at:
    ///     https://github.com/microsoft/winget-cli/blob/master/src/WinGetUtil/WinGetUtil.h.
    /// </summary>
    [Flags]
    public enum WinGetCreateManifestOption
    {
        /// <summary>
        /// Just create the manifest without any validation
        /// </summary>
        NoValidation = 0,

        /// <summary>
        /// Only validate against json schema
        /// </summary>
        SchemaValidation = 0x1,

        /// <summary>
        /// Validate against schema and also perform semantic validation
        /// </summary>
        SchemaAndSemanticValidation = 0x2,

        /// <summary>
        /// Allow shadow manifest
        /// </summary>
        AllowShadowManifest = 0x4,

        // Below options are additional validation behaviors if needed

        /// <summary>
        /// Return error on manifest fields that require verified publishers, used during semantic validation
        /// </summary>
        ReturnErrorOnVerifiedPublisherFields = 0x1000,
    }

    /// <summary>
    /// Options for validating a manifest.
    /// Must be in sync with WinGetValidateManifestOptionV2 at:
    ///     https://github.com/microsoft/winget-cli/blob/master/src/WinGetUtil/WinGetUtil.h.
    /// </summary>
    [Flags]
    public enum WinGetValidateManifestOptionV2
    {
        /// <summary>
        /// No validation, caller will get E_INVALIDARG
        /// </summary>
        None = 0,

        /// <summary>
        /// Dependencies validation against index
        /// </summary>
        DependenciesValidation = 0x1,

        /// <summary>
        /// Arp version validation against index
        /// </summary>
        ArpVersionValidation = 0x2,

        /// <summary>
        /// Installer validation
        /// </summary>
        InstallerValidation = 0x4,
    }

    /// <summary>
    /// Operation type for validating a manifest.
    /// Must be in sync with WinGetValidateManifestOperationType at:
    ///     https://github.com/microsoft/winget-cli/blob/master/src/WinGetUtil/WinGetUtil.h.
    /// </summary>
    public enum WinGetValidateManifestOperationType
    {
        /// <summary>
        /// Add
        /// </summary>
        OperationTypeAdd = 0,

        /// <summary>
        /// Update
        /// </summary>
        OperationTypeUpdate = 1,

        /// <summary>
        /// Delete
        /// </summary>
        OperationTypeDelete = 2,
    }

    /// <summary>
    /// Manifest validation result.
    /// Must be in sync with WinGetValidateManifestResult at:
    ///     https://github.com/microsoft/winget-cli/blob/master/src/WinGetUtil/WinGetUtil.h.
    /// </summary>
    [Flags]
    public enum WinGetValidateManifestResult
    {
        /// <summary>
        /// Manifest validation success.
        /// </summary>
        Success = 0,

        // Each validation step should have an enum for corresponding failure.

        /// <summary>
        /// Dependencies validation failure.
        /// </summary>
        DependenciesValidationFailure = 0x1,

        /// <summary>
        /// Arp version validation failure.
        /// </summary>
        ArpVersionValidationFailure = 0x2,

        /// <summary>
        /// Installer validation failure.
        /// </summary>
        InstallerValidationFailure = 0x4,

        // Dependencies validation result.

        /// <summary>
        /// Single Manifest package has dependencies.
        /// </summary>
        SingleManifestPackageHasDependencies = 0x10000,

        /// <summary>
        /// Multi manifest package has dependencies.
        /// </summary>
        MultiManifestPackageHasDependencies = 0x20000,

        /// <summary>
        /// Missing manifest dependencies node.
        /// </summary>
        MissingManifestDependenciesNode = 0x40000,

        /// <summary>
        /// No Suitable min version found for manifest.
        /// </summary>
        NoSuitableMinVersionDependency = 0x80000,

        /// <summary>
        /// Validation encountered a loop during dependencies validation.
        /// </summary>
        FoundDependencyLoop = 0x100000,

        // Internal error meaning validation does not complete as desired.

        /// <summary>
        /// Manifest validation internal error.
        /// </summary>
        InternalError = 0x1000,
    }

    /// <summary>
    ///  Option flags for WinGetBeginInstallerMetadataCollection.
    /// </summary>
    [Flags]
    public enum WinGetBeginInstallerMetadataCollectionOptions
    {
        /// <summary>
        /// None.
        /// </summary>
        WinGetBeginInstallerMetadataCollectionOption_None = 0,

        /// <summary>
        /// The inputJSON is a local file path, not a JSON string.
        /// </summary>
        WinGetBeginInstallerMetadataCollectionOption_InputIsFilePath = 0x1,

        /// <summary>
        ///  The inputJSON is a remote URI, not a JSON string.
        /// </summary>
        WinGetBeginInstallerMetadataCollectionOption_InputIsURI = 0x2,
    }

    /// <summary>
    /// Option flags for WinGetCompleteInstallerMetadataCollection.
    /// </summary>
    [Flags]
    public enum WinGetCompleteInstallerMetadataCollectionOptions
    {
        /// <summary>
        /// Metadata collection option none.
        /// </summary>
        WinGetCompleteInstallerMetadataCollectionOption_None = 0,

        /// <summary>
        /// Complete will simply free the collection handle without doing any additional work.
        /// </summary>
        WinGetCompleteInstallerMetadataCollectionOption_Abandon = 0x1,
    }
}
