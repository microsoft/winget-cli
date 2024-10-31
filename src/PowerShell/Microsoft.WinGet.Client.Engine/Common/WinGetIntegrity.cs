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

            try
            {
                // Start by calling winget without its WindowsApp PFN path.
                // If it succeeds and the exit code is 0 then we are good.
                var wingetCliWrapper = new WingetCLIWrapper(false);
                var result = wingetCliWrapper.RunCommand(pwshCmdlet, "--version");
                result.VerifyExitCode();
            }
            catch (Win32Exception e)
            {
                pwshCmdlet.Write(StreamType.Verbose, $"'winget.exe' Win32Exception {e.Message}");
                throw new WinGetIntegrityException(GetReason(pwshCmdlet));
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

            // WinGet is installed. Verify version if needed.
            if (!string.IsNullOrEmpty(expectedVersion))
            {
                // This assumes caller knows that the version exist.
                WinGetVersion expectedWinGetVersion = new WinGetVersion(expectedVersion);
                var installedVersion = WinGetVersion.InstalledWinGetVersion(pwshCmdlet);
                if (expectedWinGetVersion.CompareTo(installedVersion) != 0)
                {
                    throw new WinGetIntegrityException(
                        IntegrityCategory.UnexpectedVersion,
                        string.Format(
                            Resources.IntegrityUnexpectedVersionMessage,
                            installedVersion.TagVersion,
                            expectedVersion));
                }
            }
        }

        private static IntegrityCategory GetReason(PowerShellCmdlet pwshCmdlet)
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
                    var ps = PowerShell.Create(RunspaceMode.CurrentRunspace);
                    ps.AddCommand("winget").Invoke();
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
                // The file exists, but we couldn't call it... Well maybe winget's app execution alias is not enabled.
                // The trick is knowing that a magical file appears under WindowsApp when its enabled.
                string wingetAliasPath = Path.Combine(Utilities.LocalDataWindowsAppPath, Constants.WinGetExe);
                if (File.Exists(wingetAliasPath))
                {
                    // App execution alias is enabled. Then maybe the path?
                    string? envPath = Environment.GetEnvironmentVariable(Constants.PathEnvVar, EnvironmentVariableTarget.User);
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

            // Not under %LOCALAPPDATA%\\Microsoft\\WindowsApps\PFN\

            // Check OS version
            if (!IsSupportedOSVersion())
            {
                return IntegrityCategory.OsNotSupported;
            }

            // It could be that AppInstaller package is old or the package is not
            // registered at this point. To know that, call Get-AppxPackage.
            var appxModule = new AppxModuleHelper(pwshCmdlet);
            string? version = appxModule.GetAppInstallerPropertyValue("Version");
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

        private static bool IsSupportedOSVersion()
        {
            // Windows version has to be equal or newer than 10.0.17763.0
            var minWindowsVersion = new Version(10, 0, 17763, 0);
            var osVersion = Environment.OSVersion.Version;
            return osVersion.CompareTo(minWindowsVersion) >= 0;
        }
    }
}
