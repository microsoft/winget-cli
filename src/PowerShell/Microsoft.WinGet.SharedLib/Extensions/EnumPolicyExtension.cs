// -----------------------------------------------------------------------------
// <copyright file="EnumPolicyExtension.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.SharedLib.Extensions
{
    using Microsoft.WinGet.SharedLib.Exceptions;
    using Microsoft.WinGet.SharedLib.PolicySettings;
    using Microsoft.WinGet.SharedLib.Resources;

    /// <summary>
    /// Implements extensions methods around Group Policy Enum type.
    /// </summary>
    public static class EnumPolicyExtension
    {
        /// <summary>
        /// Gets ResourceString for the mapped Policy type.
        /// </summary>
        /// <param name="policy">Policy.</param>
        /// <returns>Resource string.</returns>
        public static string GetResourceString(this Policy policy)
        {
            switch (policy)
            {
                case Policy.WinGet:
                    return GroupPolicyResource.PolicyEnableWinGet;
                case Policy.Settings:
                    return GroupPolicyResource.PolicyEnableWinGetSettings;
                case Policy.ExperimentalFeatures:
                    return GroupPolicyResource.PolicyEnableExperimentalFeatures;
                case Policy.LocalManifestFiles:
                    return GroupPolicyResource.PolicyEnableLocalManifests;
                case Policy.HashOverride:
                    return GroupPolicyResource.PolicyEnableHashOverride;
                case Policy.LocalArchiveMalwareScanOverride:
                    return GroupPolicyResource.PolicyEnableLocalArchiveMalwareScanOverride;
                case Policy.DefaultSource:
                    return GroupPolicyResource.PolicyEnableDefaultSource;
                case Policy.MSStoreSource:
                    return GroupPolicyResource.PolicyEnableMSStoreSource;
                case Policy.AdditionalSources:
                    return GroupPolicyResource.PolicyAdditionalSources;
                case Policy.AllowedSources:
                    return GroupPolicyResource.PolicyAllowedSources;
                case Policy.BypassCertificatePinningForMicrosoftStore:
                    return GroupPolicyResource.PolicyEnableBypassCertificatePinningForMicrosoftStore;
                case Policy.WinGetCommandLineInterfaces:
                    return GroupPolicyResource.PolicyEnableWindowsPackageManagerCommandLineInterfaces;
                case Policy.Configuration:
                    return GroupPolicyResource.PolicyEnableWinGetConfiguration;
                default:
                    return string.Empty;
            }
        }

        /// <summary>
        /// Gets failure string mapped to GroupPolicyFailureType.
        /// </summary>
        /// <param name="policyFailure">GroupPolicyFailureType.</param>
        /// <returns>Failure resources string.</returns>
        public static string GetFailureString(this GroupPolicyFailureType policyFailure)
        {
            switch (policyFailure)
            {
                case GroupPolicyFailureType.BlockedByPolicy:
                    return GroupPolicyResource.ErrorBlockedByPolicy;
                case GroupPolicyFailureType.NotFound:
                    return GroupPolicyResource.PolicyNotFound;
                case GroupPolicyFailureType.LoadError:
                    return GroupPolicyResource.PolicyLoadError;
                default: return string.Empty;
            }
        }

        /// <summary>
        /// Gets ErrorCode mapped to GroupPolicyFailureType.
        /// </summary>
        /// <param name="policyFailure">GroupPolicyFailureType.</param>
        /// <returns>ErrorCode.</returns>
        public static int GetErrorCode(this GroupPolicyFailureType policyFailure)
        {
            switch (policyFailure)
            {
                case GroupPolicyFailureType.BlockedByPolicy:
                    return ErrorCodes.AppInstallerCLIErrorBlockedByPolicy;
                case GroupPolicyFailureType.NotFound:
                case GroupPolicyFailureType.LoadError:
                default:
                    return ErrorCodes.AppInstallerCLIErrorInternalError;
            }
        }
    }
}
