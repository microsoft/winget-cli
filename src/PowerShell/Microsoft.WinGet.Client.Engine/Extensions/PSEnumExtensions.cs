// -----------------------------------------------------------------------------
// <copyright file="PSEnumExtensions.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Extensions
{
    using System;
    using Microsoft.Management.Deployment;
    using Microsoft.WinGet.Client.Engine.PSObjects;
    using Windows.System;

    /// <summary>
    /// Extension methods for PS Enum wrappers for Microsoft.Management.Deployment enums.
    /// </summary>
    internal static class PSEnumExtensions
    {
        /// <summary>
        /// Converts PSPackageInstallMode to PackageInstallMode.
        /// </summary>
        /// <param name="psEnum">PSPackageInstallMode.</param>
        /// <returns>PackageInstallMode.</returns>
        public static PackageInstallMode ToPackageInstallMode(this PSPackageInstallMode psEnum)
        {
            return psEnum switch
            {
                PSPackageInstallMode.Default => PackageInstallMode.Default,
                PSPackageInstallMode.Silent => PackageInstallMode.Silent,
                PSPackageInstallMode.Interactive => PackageInstallMode.Interactive,
                _ => throw new InvalidOperationException(),
            };
        }

        /// <summary>
        /// Converts PSPackageInstallScope to PackageInstallScope.
        /// </summary>
        /// <param name="psEnum">PSPackageInstallScope.</param>
        /// <returns>PackageInstallScope.</returns>
        public static PackageInstallScope ToPackageInstallScope(this PSPackageInstallScope psEnum)
        {
            return psEnum switch
            {
                PSPackageInstallScope.Any => PackageInstallScope.Any,
                PSPackageInstallScope.User => PackageInstallScope.User,
                PSPackageInstallScope.System => PackageInstallScope.System,
                PSPackageInstallScope.UserOrUnknown => PackageInstallScope.UserOrUnknown,
                PSPackageInstallScope.SystemOrUnknown => PackageInstallScope.SystemOrUnknown,
                _ => throw new InvalidOperationException(),
            };
        }

        /// <summary>
        /// Converts PSProcessorArchitecture to ProcessorArchitecture.
        /// </summary>
        /// <param name="psEnum">PSProcessorArchitecture.</param>
        /// <returns>ProcessorArchitecture.</returns>
        public static ProcessorArchitecture ToProcessorArchitecture(this PSProcessorArchitecture psEnum)
        {
            return psEnum switch
            {
                PSProcessorArchitecture.X86 => ProcessorArchitecture.X86,
                PSProcessorArchitecture.Arm => ProcessorArchitecture.Arm,
                PSProcessorArchitecture.X64 => ProcessorArchitecture.X64,
                PSProcessorArchitecture.Neutral => ProcessorArchitecture.Neutral,
                PSProcessorArchitecture.Arm64 => ProcessorArchitecture.Arm64,
                PSProcessorArchitecture.X86OnArm64 => ProcessorArchitecture.X86OnArm64,
                _ => throw new InvalidOperationException(),
            };
        }

        /// <summary>
        /// Converts PSPackageUninstallMode to PackageUninstallMode.
        /// </summary>
        /// <param name="psEnum">PSPackageUninstallMode.</param>
        /// <returns>PackageUninstallMode.</returns>
        public static PackageUninstallMode ToPackageUninstallMode(this PSPackageUninstallMode psEnum)
        {
            return psEnum switch
            {
                PSPackageUninstallMode.Default => PackageUninstallMode.Default,
                PSPackageUninstallMode.Silent => PackageUninstallMode.Silent,
                PSPackageUninstallMode.Interactive => PackageUninstallMode.Interactive,
                _ => throw new InvalidOperationException(),
            };
        }
    }
}
