// -----------------------------------------------------------------------------
// <copyright file="PSEnumHelpers.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Helpers
{
    using System;
    using Microsoft.Management.Deployment;
    using Newtonsoft.Json.Linq;
    using Windows.System;

    /// <summary>
    /// Extension methods for PS Enum wrappers for Microsoft.Management.Deployment enums.
    /// </summary>
    internal static class PSEnumHelpers
    {
        /// <summary>
        /// Checks if the provided enum string value matches the 'Default' value for PS Enums.
        /// </summary>
        /// <param name="value">Enum string value.</param>
        /// <returns>Boolean value.</returns>
        public static bool IsDefaultEnum(string value)
        {
            return string.Equals(value, "Default", StringComparison.OrdinalIgnoreCase);
        }

        /// <summary>
        /// Converts PSPackageInstallMode string value to PackageInstallMode.
        /// </summary>
        /// <param name="value">PSPackageInstallMode to string value.</param>
        /// <returns>PackageInstallMode.</returns>
        public static PackageInstallMode ToPackageInstallMode(string value)
        {
            return value switch
            {
                "Default" => PackageInstallMode.Default,
                "Silent" => PackageInstallMode.Silent,
                "Interactive" => PackageInstallMode.Interactive,
                _ => throw new InvalidOperationException(),
            };
        }

        /// <summary>
        /// Converts PSPackageInstallScope string value to PackageInstallScope.
        /// </summary>
        /// <param name="value">PSPackageInstallScope to string value.</param>
        /// <returns>PackageInstallScope.</returns>
        public static PackageInstallScope ToPackageInstallScope(string value)
        {
            return value switch
            {
                "Any" => PackageInstallScope.Any,
                "User" => PackageInstallScope.User,
                "System" => PackageInstallScope.System,
                "UserOrUnknown" => PackageInstallScope.UserOrUnknown,
                "SystemOrUnknown" => PackageInstallScope.SystemOrUnknown,
                _ => throw new InvalidOperationException(),
            };
        }

        /// <summary>
        /// Converts PSProcessorArchitecture string value to ProcessorArchitecture.
        /// </summary>
        /// <param name="value">PSProcessorArchitecture to string value.</param>
        /// <returns>ProcessorArchitecture.</returns>
        public static ProcessorArchitecture ToProcessorArchitecture(string value)
        {
            return value switch
            {
                "X86" => ProcessorArchitecture.X86,
                "Arm" => ProcessorArchitecture.Arm,
                "X64" => ProcessorArchitecture.X64,
                "Arm64" => ProcessorArchitecture.Arm64,
                _ => throw new InvalidOperationException(),
            };
        }

        /// <summary>
        /// Converts PSPackageUninstallMode string value to PackageUninstallMode.
        /// </summary>
        /// <param name="value">PSPackageUninstallMode string value.</param>
        /// <returns>PackageUninstallMode.</returns>
        public static PackageUninstallMode ToPackageUninstallMode(string value)
        {
            return value switch
            {
                "Default" => PackageUninstallMode.Default,
                "Silent" => PackageUninstallMode.Silent,
                "Interactive" => PackageUninstallMode.Interactive,
                _ => throw new InvalidOperationException(),
            };
        }

        /// <summary>
        /// Converts PSPackageFieldMatchOption string value to PackageFieldMatchOption.
        /// </summary>
        /// <param name="value">PSPackageFieldMatchOption string value.</param>
        /// <returns>PackageFieldMatchOption.</returns>
        public static PackageFieldMatchOption ToPackageFieldMatchOption(string value)
        {
            return value switch
            {
                "Equals" => PackageFieldMatchOption.Equals,
                "EqualsCaseInsensitive" => PackageFieldMatchOption.EqualsCaseInsensitive,
                "StartsWithCaseInsensitive" => PackageFieldMatchOption.StartsWithCaseInsensitive,
                "ContainsCaseInsensitive" => PackageFieldMatchOption.ContainsCaseInsensitive,
                _ => throw new InvalidOperationException(),
            };
        }

        /// <summary>
        /// Converts PSPackageInstallerType string value to PackageInstallerType.
        /// </summary>
        /// <param name="value">PSPackageInstallerType string value.</param>
        /// <returns>PackageInstallerType.</returns>
        public static PackageInstallerType ToPackageInstallerType(string value)
        {
            return value switch
            {
                "Unknown" => PackageInstallerType.Unknown,
                "Inno" => PackageInstallerType.Inno,
                "Wix" => PackageInstallerType.Wix,
                "Msi" => PackageInstallerType.Msi,
                "Nullsoft" => PackageInstallerType.Nullsoft,
                "Zip" => PackageInstallerType.Zip,
                "Msix" => PackageInstallerType.Msix,
                "Exe" => PackageInstallerType.Exe,
                "Burn" => PackageInstallerType.Burn,
                "MSStore" => PackageInstallerType.MSStore,
                "Portable" => PackageInstallerType.Portable,
                _ => throw new InvalidOperationException(),
            };
        }

        /// <summary>
        /// Converts PSPackageRepairMode string value to PackageRepairMode.
        /// </summary>
        /// <param name="value">PSPackageRepairMode string value.</param>
        /// <returns>PackageRepairMode.</returns>
        public static PackageRepairMode ToPackageRepairMode(string value)
        {
            return value switch
            {
                "Default" => PackageRepairMode.Default,
                "Silent" => PackageRepairMode.Silent,
                "Interactive" => PackageRepairMode.Interactive,
                _ => throw new InvalidOperationException(),
            };
        }
    }
}
