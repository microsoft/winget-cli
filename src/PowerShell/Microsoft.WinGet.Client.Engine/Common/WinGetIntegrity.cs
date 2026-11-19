// -----------------------------------------------------------------------------
// <copyright file="WinGetIntegrity.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Common
{
    using System;
    using System.ComponentModel;
    using System.IO;
    using System.Management.Automation;
    using Microsoft.WinGet.Client.Engine.Exceptions;
    using Microsoft.WinGet.Client.Engine.Helpers;
    using Microsoft.WinGet.Common.Command;
    using Microsoft.WinGet.Resources;

    /// <summary>
    /// Validates winget runs correctly.
    /// </summary>
    internal static class WinGetIntegrity
    {
        private const int ErrorFileNotFound = 2;

        /// <summary>
        /// Verifies winget runs correctly. If it doesn't, tries to find the reason why it failed.
        /// </summary>
        /// <param name="pwshCmdlet">The calling cmdlet.</param>
        /// <param name="expectedVersion">Expected version.</param>
        public static void AssertWinGet(PowerShellCmdlet pwshCmdlet, string expectedVersion)
        {
            // In-proc shouldn't have other dependencies and thus should be ok.
            if (Utilities.UsesInProcWinget)
            {
                // Only check the OS version support for in-proc
                if (!IsSupportedOSVersion())
                {
                    throw new WinGetIntegrityException(IntegrityCategory.OsNotSupported);
                }

                return;
            }

            WinGetCLICommandResult? versionResult = null;
            string? wingetPath = null;

            try
            {
                // Use the installed package's winget instead of searching CWD or PATH.
                wingetPath = GetWinGetPath(pwshCmdlet) ?? throw new Win32Exception(ErrorFileNotFound);
                versionResult = WinGetVersion.RunWinGetVersionFromCLI(pwshCmdlet, wingetPath);
                versionResult.VerifyExitCode();
            }
            catch (Win32Exception e)
            {
                pwshCmdlet.Write(StreamType.Verbose, $"'winget.exe' Win32Exception {e.Message}");
                throw new WinGetIntegrityException(GetReason(pwshCmdlet, wingetPath));
            }
            catch (Exception e) when (e is WinGetCLIException || e is WinGetCLITimeoutException)
            {
                pwshCmdlet.Write(StreamType.Verbose, $"'winget.exe' WinGetCLIException {e.Message}");
                throw new WinGetIntegrityException(IntegrityCategory.Failure, e);
            }
            catch (Exception e)
            {
                pwshCmdlet.Write(StreamType.Verbose, $"'winget.exe' Exception {e.Message}");
                throw new WinGetIntegrityException(IntegrityCategory.Unknown, e);
            }

            var environmentCategory = GetEnvironmentIntegrityCategory(
                Utilities.LocalDataWindowsAppPath,
                Environment.GetEnvironmentVariable(Constants.PathEnvVar));
            if (environmentCategory != IntegrityCategory.Installed)
            {
                throw new WinGetIntegrityException(environmentCategory);
            }

            // WinGet is installed. Verify version if needed.
            if (!string.IsNullOrEmpty(expectedVersion))
            {
                // This assumes caller knows that the version exist.
                WinGetVersion expectedWinGetVersion = new WinGetVersion(expectedVersion);
                var installedVersion = WinGetVersion.InstalledWinGetVersion(pwshCmdlet, versionResult);
                if (expectedWinGetVersion.CompareTo(installedVersion) != 0)
                {
                    throw new WinGetIntegrityException(
                        IntegrityCategory.UnexpectedVersion,
                        string.Format(
                            Resources.IntegrityUnexpectedVersionMessage,
                            installedVersion.TagVersion,
                            expectedVersion))
                    { InstalledVersion = installedVersion };
                }
            }

            // Verify that the winget source is installed.
            var appxModule = new AppxModuleHelper(pwshCmdlet);
            if (!appxModule.IsWinGetSourceInstalled())
            {
                throw new WinGetIntegrityException(IntegrityCategory.WinGetSourceNotInstalled);
            }
        }

        /// <summary>Checks app-execution alias and PATH availability without running a command.</summary>
        /// <param name="windowsAppsPath">The WindowsApps directory.</param>
        /// <param name="envPath">The current process PATH.</param>
        /// <returns>The environment's integrity category.</returns>
        internal static IntegrityCategory GetEnvironmentIntegrityCategory(string windowsAppsPath, string? envPath)
        {
            if (!File.Exists(Path.Combine(windowsAppsPath, Constants.WinGetExe)))
            {
                return IntegrityCategory.AppExecutionAliasDisabled;
            }

            if (!Utilities.PathContainsDirectory(envPath, windowsAppsPath))
            {
                return IntegrityCategory.NotInPath;
            }

            return IntegrityCategory.Installed;
        }

        private static IntegrityCategory GetReason(PowerShellCmdlet pwshCmdlet, string? wingetPath)
        {
            // Ok, so you are here because calling winget --version failed. Lets try to figure out why.
            var category = IntegrityCategory.Unknown;
            pwshCmdlet.ExecuteInPowerShellThread(() =>
            {
                // When running winget.exe on PowerShell the message of the Win32Exception will distinguish between
                // 'The system cannot find the file specified' and 'No applicable app licenses found' but of course
                // the HRESULT is the same (E_FAIL).
                // To not compare strings let Powershell handle it. If calling winget throws an
                // ApplicationFailedException then is most likely that the license is not there.
                try
                {
                    if (wingetPath != null)
                    {
                        var ps = PowerShell.Create(RunspaceMode.CurrentRunspace);
                        ps.AddCommand(wingetPath).Invoke();
                    }
                }
                catch (ApplicationFailedException e)
                {
                    pwshCmdlet.Write(StreamType.Verbose, e.Message);
                    category = IntegrityCategory.AppInstallerNoLicense;
                }
                catch (Exception)
                {
                }
            });

            if (category != IntegrityCategory.Unknown)
            {
                return category;
            }

            // First lets check if the file is there, which means it is installed or someone is taking our place.
            if (File.Exists(WingetCLIWrapper.WinGetFullPath))
            {
                var environmentCategory = GetEnvironmentIntegrityCategory(
                    Utilities.LocalDataWindowsAppPath,
                    Environment.GetEnvironmentVariable(Constants.PathEnvVar));
                if (environmentCategory != IntegrityCategory.Installed)
                {
                    return environmentCategory;
                }
            }

            // Not under %LOCALAPPDATA%\\Microsoft\\WindowsApps\PFN\

            // Check OS version
            if (!IsSupportedOSVersion())
            {
                return IntegrityCategory.OsNotSupported;
            }

            // It could be that AppInstaller package is old or the package is not
            // registered at this point. To know that, call Get-AppxPackage.
            var appxModule = new AppxModuleHelper(pwshCmdlet);
#if USE_PROD_CLSIDS
            string? version = appxModule.GetStoreSignedAppInstallerPropertyValue("Version");
#else
            string? version = appxModule.GetAppInstallerPropertyValue("Version");
#endif
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

        private static string? GetWinGetPath(PowerShellCmdlet pwshCmdlet)
        {
#if USE_PROD_CLSIDS
            string? location = new AppxModuleHelper(pwshCmdlet).GetStoreSignedAppInstallerPropertyValue("InstallLocation");
            return string.IsNullOrEmpty(location) ? null : Path.Combine(location, Constants.WinGetExe);
#else
            // Development packages are not Store signed.
            return WingetCLIWrapper.WinGetFullPath;
#endif
        }

        private static bool IsSupportedOSVersion()
        {
            // Windows version has to be equal or newer than 10.0.17763.0
            var minWindowsVersion = new Version(10, 0, 17763, 0);
            var osVersion = Environment.OSVersion.Version;
            return osVersion.CompareTo(minWindowsVersion) >= 0;
        }
    }
}
