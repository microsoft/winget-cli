// -----------------------------------------------------------------------------
// <copyright file="AppxModuleHelper.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Helpers
{
    using System;
    using System.IO;
    using System.Linq;
    using System.Management.Automation;
    using System.Runtime.InteropServices;
    using System.Text;
    using Microsoft.WinGet.Client.Common;

    /// <summary>
    /// Helper to make calls to the Appx module.
    /// There's a bug in the Appx Module that it can't be loaded from Core in pre 10.0.22453.0 builds without
    /// the -UseWindowsPowerShell option. In post 10.0.22453.0 builds there's really no difference between
    /// using or not -UseWindowsPowerShell as it will automatically get loaded using WinPSCompatSession remoting session.
    /// https://github.com/PowerShell/PowerShell/issues/13138.
    /// </summary>
    internal class AppxModuleHelper
    {
        private const string ImportModuleCore = "Import-Module Appx -UseWindowsPowerShell";
        private const string GetAppxPackageCommand = "Get-AppxPackage {0}";
        private const string AddAppxPackageFormat = "Add-AppxPackage -Path {0}";
        private const string AppxManifest = "AppxManifest.xml";

        private const string ForceUpdateFromAnyVersion = " -ForceUpdateFromAnyVersion";
        private const string Register = "-Register";
        private const string DisableDevelopmentMode = "-DisableDevelopmentMode";

        private const string AppInstallerName = "Microsoft.DesktopAppInstaller";

        private readonly CommandInvocationIntrinsics commandInvocation;

        /// <summary>
        /// Initializes a new instance of the <see cref="AppxModuleHelper"/> class.
        /// </summary>
        /// <param name="commandInvocation">From calling cmdlet.</param>
        public AppxModuleHelper(CommandInvocationIntrinsics commandInvocation)
        {
            this.commandInvocation = commandInvocation;

#if !POWERSHELL_WINDOWS
            this.commandInvocation.InvokeScript(ImportModuleCore);
#endif
        }

        /// <summary>
        /// Calls Get-AppxPackage Microsoft.DesktopAppInstaller.
        /// </summary>
        /// <returns>Result of Get-AppxPackage.</returns>
        public PSObject GetAppInstallerObject()
        {
            return this.GetAppxObject(AppInstallerName);
        }

        /// <summary>
        /// Gets the string value a property from the Get-AppxPackage object of AppInstaller.
        /// </summary>
        /// <param name="propertyName">Property name.</param>
        /// <returns>Value, null if doesn't exist.</returns>
        public string GetAppInstallerPropertyValue(string propertyName)
        {
            string result = null;
            var packageObj = this.GetAppInstallerObject();
            if (packageObj is not null)
            {
                var property = packageObj.Properties.Where(p => p.Name == propertyName).FirstOrDefault();
                if (property is not null)
                {
                    result = property.Value as string;
                }
            }

            return result;
        }

        /// <summary>
        /// Calls Add-AppxPackage with the path specified.
        /// </summary>
        /// <param name="localPath">The path of the package to add.</param>
        /// <param name="downgrade">If the package version is lower than the installed one.</param>
        public void AddAppInstallerBundle(string localPath, bool downgrade = false)
        {
            // TODO: We need to follow up for Microsoft.UI.Xaml.2.7
            // downloading the nuget and extracting it doesn't sound like the right thing to do.
            this.InstallVCLibsDependencies();

            StringBuilder sb = new StringBuilder();
            sb.Append(string.Format(AddAppxPackageFormat, localPath));

            if (downgrade)
            {
                sb.Append(ForceUpdateFromAnyVersion);
            }

            this.commandInvocation.InvokeScript(sb.ToString());
        }

        /// <summary>
        /// Calls Add-AppxPackage to register.
        /// </summary>
        public void RegisterAppInstaller()
        {
            string packageFullName = this.GetAppInstallerPropertyValue("PackageFullName");
            string appxManifestPath = Path.Combine(
                Utilities.ProgramFilesWindowsAppPath,
                packageFullName,
                AppxManifest);

            this.commandInvocation.InvokeScript(
                $"{string.Format(AddAppxPackageFormat, appxManifestPath)} {Register} {DisableDevelopmentMode}");
        }

        private PSObject GetAppxObject(string packageName)
        {
            return this.commandInvocation
                .InvokeScript(string.Format(GetAppxPackageCommand, packageName))
                .FirstOrDefault();
        }

        private void InstallVCLibsDependencies()
        {
            var vcLibsPackageObj = this.GetAppxObject("Microsoft.VCLibs.140.00.UWPDesktop_8wekyb3d8bbwe");
            if (vcLibsPackageObj is null)
            {
                var arch = RuntimeInformation.OSArchitecture;
                string url;
                if (arch == Architecture.X64)
                {
                    url = "https://aka.ms/Microsoft.VCLibs.x64.14.00.Desktop.appx";
                }
                else if (arch == Architecture.X86)
                {
                    url = "https://aka.ms/Microsoft.VCLibs.x86.14.00.Desktop.appx";
                }
                else
                {
                    throw new PSNotSupportedException(arch.ToString());
                }

                var tmpFile = Path.GetTempFileName();

                // This is weird but easy.
                var githubRelease = new GitHubRelease();
                githubRelease.DownloadUrl(url, tmpFile);

                this.commandInvocation.InvokeScript(string.Format(AddAppxPackageFormat, tmpFile));
            }
        }
    }
}
