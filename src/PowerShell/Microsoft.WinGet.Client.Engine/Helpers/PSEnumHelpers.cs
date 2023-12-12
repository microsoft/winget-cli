// -----------------------------------------------------------------------------
// <copyright file="PSEnumHelpers.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Helpers
{
    using System;
    using Microsoft.Management.Deployment;
    using Microsoft.WinGet.Client.Engine.PSObjects;
    using Windows.System;

    /// <summary>
    /// Extension methods for PS Enum wrappers for Microsoft.Management.Deployment enums.
    /// </summary>
    internal static class PSEnumHelpers
    {
        /// <summary>
        /// Converts PSPackageInstallMode to PackageInstallMode.
        /// </summary>
        /// <param name="value">PSPackageInstallMode.</param>
        /// <returns>PackageInstallMode.</returns>
        public static PackageInstallMode ToPackageInstallMode(PSPackageInstallMode value)
        {
            return value switch
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
        /// <param name="value">PSPackageInstallScope.</param>
        /// <returns>PackageInstallScope.</returns>
        public static PackageInstallScope ToPackageInstallScope(PSPackageInstallScope value)
        {
            return value switch
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
        /// <param name="value">PSProcessorArchitecture.</param>
        /// <returns>ProcessorArchitecture.</returns>
        public static ProcessorArchitecture ToProcessorArchitecture(PSProcessorArchitecture value)
        {
            return value switch
            {
                PSProcessorArchitecture.X86 => ProcessorArchitecture.X86,
                PSProcessorArchitecture.Arm => ProcessorArchitecture.Arm,
                PSProcessorArchitecture.X64 => ProcessorArchitecture.X64,
                PSProcessorArchitecture.Arm64 => ProcessorArchitecture.Arm64,
                _ => throw new InvalidOperationException(),
            };
        }

        /// <summary>
        /// Converts PSPackageUninstallMode to PackageUninstallMode.
        /// </summary>
        /// <param name="value">PSPackageUninstallMode string value.</param>
        /// <returns>PackageUninstallMode.</returns>
        public static PackageUninstallMode ToPackageUninstallMode(PSPackageUninstallMode value)
        {
            return value switch
            {
                PSPackageUninstallMode.Default => PackageUninstallMode.Default,
                PSPackageUninstallMode.Silent => PackageUninstallMode.Silent,
                PSPackageUninstallMode.Interactive => PackageUninstallMode.Interactive,
                _ => throw new InvalidOperationException(),
            };
        }

        /// <summary>
        /// Converts PSPackageFieldMatchOption to PackageFieldMatchOption.
        /// </summary>
        /// <param name="value">PSPackageFieldMatchOption.</param>
        /// <returns>PackageFieldMatchOption.</returns>
        public static PackageFieldMatchOption ToPackageFieldMatchOption(PSPackageFieldMatchOption value)
        {
            return value switch
            {
                PSPackageFieldMatchOption.Equals => PackageFieldMatchOption.Equals,
                PSPackageFieldMatchOption.EqualsCaseInsensitive => PackageFieldMatchOption.EqualsCaseInsensitive,
                PSPackageFieldMatchOption.StartsWithCaseInsensitive => PackageFieldMatchOption.StartsWithCaseInsensitive,
                PSPackageFieldMatchOption.ContainsCaseInsensitive => PackageFieldMatchOption.ContainsCaseInsensitive,
                _ => throw new InvalidOperationException(),
            };
        }

        /// <summary>
        /// Converts PSPackageInstallerType to PackageInstallerType.
        /// </summary>
        /// <param name="value">PSPackageInstallerType.</param>
        /// <returns>PackageInstallerType.</returns>
        public static PackageInstallerType ToPackageInstallerType(PSPackageInstallerType value)
        {
            return value switch
            {
                PSPackageInstallerType.Unknown => PackageInstallerType.Unknown,
                PSPackageInstallerType.Inno => PackageInstallerType.Inno,
                PSPackageInstallerType.Wix => PackageInstallerType.Wix,
                PSPackageInstallerType.Msi => PackageInstallerType.Msi,
                PSPackageInstallerType.Nullsoft => PackageInstallerType.Nullsoft,
                PSPackageInstallerType.Zip => PackageInstallerType.Zip,
                PSPackageInstallerType.Msix => PackageInstallerType.Msix,
                PSPackageInstallerType.Exe => PackageInstallerType.Exe,
                PSPackageInstallerType.Burn => PackageInstallerType.Burn,
                PSPackageInstallerType.MSStore => PackageInstallerType.MSStore,
                PSPackageInstallerType.Portable => PackageInstallerType.Portable,
                _ => throw new InvalidOperationException(),
            };
        }
    }
}
