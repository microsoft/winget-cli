// -----------------------------------------------------------------------------
// <copyright file="WinGetIntegrity.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Common
{
    using System;
    using System.ComponentModel;
    using System.IO;
    using System.Linq;
    using System.Management.Automation;
    using Microsoft.WinGet.Client.Exceptions;
    using Microsoft.WinGet.Client.Helpers;
    using Microsoft.WinGet.Client.Properties;

    /// <summary>
    /// Validates winget runs correctly.
    /// </summary>
    internal static class WinGetIntegrity
    {
        /// <summary>
        /// Verifies winget runs correctly. If it doesn't, tries to find the reason why it failed.
        /// </summary>
        /// <param name="pSCmdlet">The calling cmdlet.</param>
        /// <param name="expectedVersion">Expected version.</param>
        public static void AssertWinGet(PSCmdlet pSCmdlet, string expectedVersion)
        {
            try
            {
                // Start by calling winget without its WindowsApp PFN path.
                // If it succeeds and the exit code is 0 then we are good.
                var wingetCliWrapper = new WingetCLIWrapper(false);
                var result = wingetCliWrapper.RunCommand("--version");
                result.VerifyExitCode();

                if (!string.IsNullOrEmpty(expectedVersion))
                {
                    // Verify version
                    var installedVersion = WinGetVersionHelper.InstalledWinGetVersion;
                    if (expectedVersion != installedVersion)
                    {
                        throw new WinGetIntegrityException(
                            IntegrityCategory.UnexpectedVersion,
                            string.Format(
                                Resources.IntegrityUnexpectedVersionMessage,
                                installedVersion,
                                expectedVersion));
                    }
                }
            }
            catch (Win32Exception)
            {
                throw new WinGetIntegrityException(GetReason(pSCmdlet));
            }
            catch (Exception e) when (e is WinGetCLIException || e is WinGetCLITimeoutException)
            {
                throw new WinGetIntegrityException(IntegrityCategory.Failure, e);
            }
            catch (Exception e)
            {
                throw new WinGetIntegrityException(IntegrityCategory.Unknown, e);
            }
        }

        /// <summary>
        /// Verifies winget runs correctly.
        /// </summary>
        /// <param name="pSCmdlet">The calling cmdlet.</param>
        /// <param name="expectedVersion">Expected version.</param>
        /// <returns>Integrity category.</returns>
        public static IntegrityCategory GetIntegrityCategory(PSCmdlet pSCmdlet, string expectedVersion)
        {
            try
            {
                AssertWinGet(pSCmdlet, expectedVersion);
            }
            catch (WinGetIntegrityException e)
            {
                return e.Category;
            }

            return IntegrityCategory.Installed;
        }

        private static IntegrityCategory GetReason(PSCmdlet pSCmdlet)
        {
            // Ok, so you are here because calling winget --version failed. Lets try to figure out why.

            // First lets check if the file is there, which means it is installed or someone is taking our place.
            if (File.Exists(WingetCLIWrapper.WinGetFullPath))
            {
                // The file exists, but we couldn't call it... We'll maybe winget's app execution alias is not enabled.
                // The trick is knowing that a magical file appears under WindowsApp when its enabled.
                string wingetAliasPath = Path.Combine(Utilities.LocalDataWindowsAppPath, Constants.WinGetExe);
                if (File.Exists(wingetAliasPath))
                {
                    // App execution alias is enabled. Then maybe the path?
                    string envPath = Environment.GetEnvironmentVariable(Constants.PathEnvVar, EnvironmentVariableTarget.User);
                    if (string.IsNullOrEmpty(envPath) ||
                        !envPath.EndsWith(Utilities.LocalDataWindowsAppPath) ||
                        !envPath.Contains($"{Utilities.LocalDataWindowsAppPath};"))
                    {
                        return IntegrityCategory.NotInPath;
                    }
                }
                else
                {
                    return IntegrityCategory.AppExecutionAliasDisabled;
                }
            }

            // Not under %LOCALAPPDATA%\\Microsoft\\WindowsApps\PFM\

            // Windows version has to be equal or newer than 10.0.17763.0
            var minWindowsVersion = new Version(10, 0, 17763, 0);
            var osVersion = Environment.OSVersion.Version;
            if (osVersion.CompareTo(minWindowsVersion) < 0)
            {
                return IntegrityCategory.OsNotSupported;
            }

            // It could be that AppInstaller package is old or the package is not
            // registered at this point. To know that, call Get-AppxPackage.
            var appxModule = new AppxModuleHelper(pSCmdlet);
            string version = appxModule.GetAppInstallerPropertyValue("Version");
            if (version is null)
            {
                // This can happen in Windows Sandbox.
                return IntegrityCategory.AppInstallerNotInstalled;
            }

            // Now AppInstaller version has to be greater than 1.11.11451
            var minAppInstallerVersion = new Version(1, 11, 11451);
            var appInstallerVersion = new Version(version);
            if (appInstallerVersion.CompareTo(minAppInstallerVersion) < 0)
            {
                return IntegrityCategory.AppInstallerNotSupported;
            }

            // If we get here, we know the package is in the machine but not registered for the user.
            return IntegrityCategory.AppInstallerNotRegistered;
        }
    }
}
