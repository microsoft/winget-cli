// -----------------------------------------------------------------------------
// <copyright file="Enums.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.SharedLib.PolicySettings
{
    /// <summary>
    /// Group policy state.
    /// </summary>
    public enum PolicyState
    {
        /// <summary>
        /// Not-Configured
        /// </summary>
        NotConfigured,

        /// <summary>
        /// Disabled.
        /// </summary>
        Disabled,

        /// <summary>
        /// Enabled.
        /// </summary>
        Enabled,
    }

    /// <summary>
    /// Supported Toggle policy types.
    /// </summary>
    public enum Policy
    {
        /// <summary>
        /// EnableAppInstaller.
        /// </summary>
        WinGet,

        /// <summary>
        /// Enable WinGet Settings.
        /// </summary>
        Settings,

        /// <summary>
        /// Enable Experimental Features.
        /// </summary>
        ExperimentalFeatures,

        /// <summary>
        /// Enable Local Manifest Files.
        /// </summary>
        LocalManifestFiles,

        /// <summary>
        /// Enable Hash Override.
        /// </summary>
        HashOverride,

        /// <summary>
        /// Enable Local Archive Malware scan override.
        /// </summary>
        LocalArchiveMalwareScanOverride,

        /// <summary>
        /// Enable DefaultSource.
        /// </summary>
        DefaultSource,

        /// <summary>
        /// Enable Microsoft Store Source.
        /// </summary>
        MSStoreSource,

        /// <summary>
        /// Enable Additional Source.
        /// </summary>
        AdditionalSources,

        /// <summary>
        /// Enabled Allowed Sources.
        /// </summary>
        AllowedSources,

        /// <summary>
        /// Enable Bypass Certificate Pinning for Microsoft Store.
        /// </summary>
        BypassCertificatePinningForMicrosoftStore,

        /// <summary>
        /// Enabled Command line Interfaces.
        /// </summary>
        WinGetCommandLineInterfaces,

        /// <summary>
        /// Enabled configuration.
        /// </summary>
        Configuration,
    }

    /// <summary>
    /// Group policy failures kind.
    /// </summary>
    public enum GroupPolicyFailureType
    {
        /// <summary>
        /// Blocked by Policy.
        /// </summary>
        BlockedByPolicy,

        /// <summary>
        /// Policy not found or failed to read.
        /// </summary>
        NotFound,

        /// <summary>
        /// Group policy load error.
        /// </summary>
        LoadError,
    }
}
