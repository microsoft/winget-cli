// -----------------------------------------------------------------------------
// <copyright file="Utilities.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Common
{
    using System;
    using System.Security.Principal;
    using System.Threading;

    /// <summary>
    /// This class contains various helper methods for this project.
    /// </summary>
    internal static class Utilities
    {
        /// <summary>
        /// Gets a value indicating whether the current assembly is executing in an administrative context.
        /// </summary>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Interoperability", "CA1416:Validate platform compatibility", Justification = "Windows only API")]
        public static bool ExecutingAsAdministrator
        {
            get
            {
                WindowsIdentity identity = WindowsIdentity.GetCurrent();
                WindowsPrincipal principal = new (identity);
                return principal.IsInRole(WindowsBuiltInRole.Administrator);
            }
        }

        /// <summary>
        /// Gets a value indicating whether the current assembly is executing as a SYSTEM user.
        /// </summary>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Interoperability", "CA1416:Validate platform compatibility", Justification = "Windows only API")]
        public static bool ExecutingAsSystem
        {
            get
            {
                return WindowsIdentity.GetCurrent().IsSystem;
            }
        }

        /// <summary>
        /// Gets a value indicating whether the current execution context will use in-proc winget.
        /// </summary>
        public static bool UsesInProcWinget
        {
            get
            {
                return ExecutingAsSystem;
            }
        }

        /// <summary>
        /// Gets a value indicating whether the current thread is executing as STA.
        /// </summary>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Interoperability", "CA1416:Validate platform compatibility", Justification = "Windows only API")]
        public static bool ThreadIsSTA
        {
            get
            {
                return Thread.CurrentThread.GetApartmentState() == ApartmentState.STA;
            }
        }

        /// <summary>
        /// Gets the windows app path for local app data.
        /// </summary>
        public static string LocalDataWindowsAppPath
        {
            get
            {
                return Environment.ExpandEnvironmentVariables(@"%LOCALAPPDATA%\Microsoft\WindowsApps");
            }
        }

        /// <summary>
        /// Gets the windows app path for program files.
        /// </summary>
        public static string ProgramFilesWindowsAppPath
        {
            get
            {
                return Environment.ExpandEnvironmentVariables(@"%PROGRAMFILES%\WindowsApps");
            }
        }

        /// <summary>
        /// Adds the WindowsApp local app data path to the user environment path.
        /// </summary>
        public static void AddWindowsAppToPath()
        {
            var scope = EnvironmentVariableTarget.User;
            string envPathValue = Environment.GetEnvironmentVariable(Constants.PathEnvVar, scope);
            if (string.IsNullOrEmpty(envPathValue) || !envPathValue.Contains(Utilities.LocalDataWindowsAppPath))
            {
                Environment.SetEnvironmentVariable(
                    Constants.PathEnvVar,
                    $"{envPathValue};{Utilities.LocalDataWindowsAppPath}",
                    scope);
            }
        }
    }
}
