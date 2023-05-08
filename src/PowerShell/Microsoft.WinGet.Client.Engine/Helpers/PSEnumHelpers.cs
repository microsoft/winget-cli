﻿// -----------------------------------------------------------------------------
// <copyright file="PSEnumHelpers.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Helpers
{
    using System;
    using Microsoft.Management.Deployment;
    using Windows.System;

    /// <summary>
    /// Extension methods for PS Enum wrappers for Microsoft.Management.Deployment enums.
    /// </summary>
    internal static class PSEnumHelpers
    {
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
    }
}
