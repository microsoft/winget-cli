// -----------------------------------------------------------------------------
// <copyright file="AppxModuleHelper.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Helpers
{
    using System.IO;
    using System.Linq;
    using System.Management.Automation;
    using System.Runtime.InteropServices;
    using System.Text;
    using Microsoft.WinGet.Client.Common;

    /// <summary>
    /// Helper to make calls to the Appx module.
    /// </summary>
    internal class AppxModuleHelper
    {
        private const string GetAppxModule = "Get-Module Appx";
        private const string ImportModuleCore = "Import-Module Appx -UseWindowsPowerShell";
        private const string GetAppxPackageCommand = "Get-AppxPackage {0}";
        private const string AddAppxPackageFormat = "Add-AppxPackage -Path {0}";
        private const string AddAppxPackageRegisterFormat = "Add-AppxPackage -Path {0} -Register -DisableDevelopmentMode";
        private const string ForceUpdateFromAnyVersion = " -ForceUpdateFromAnyVersion";

        private const string AppInstallerName = "Microsoft.DesktopAppInstaller";
        private const string AppxManifest = "AppxManifest.xml";
        private const string PackageFullName = "PackageFullName";

        // Dependencies
        private const string VCLibsUWPDesktop = "Microsoft.VCLibs.140.00.UWPDesktop";
        private const string UiXaml27 = "Microsoft.UI.Xaml.2.7";

        private readonly CommandInvocationIntrinsics commandInvocation;

        /// <summary>
        /// Initializes a new instance of the <see cref="AppxModuleHelper"/> class.
        /// </summary>
        /// <param name="commandInvocation">From calling cmdlet.</param>
        public AppxModuleHelper(CommandInvocationIntrinsics commandInvocation)
        {
            this.commandInvocation = commandInvocation;

            // There's a bug in the Appx Module that it can't be loaded from Core in pre 10.0.22453.0 builds without
            // the -UseWindowsPowerShell option. In post 10.0.22453.0 builds there's really no difference between
            // using or not -UseWindowsPowerShell as it will automatically get loaded using WinPSCompatSession remoting session.
            // https://github.com/PowerShell/PowerShell/issues/13138.
#if !POWERSHELL_WINDOWS
            var appxModule = this.commandInvocation.InvokeScript(GetAppxModule);
            if (appxModule is null)
            {
                this.commandInvocation.InvokeScript(ImportModuleCore);
            }
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
        /// Calls Add-AppxPackage with the specified path.
        /// </summary>
        /// <param name="localPath">The path of the package to add.</param>
        /// <param name="downgrade">If the package version is lower than the installed one.</param>
        public void AddAppInstallerBundle(string localPath, bool downgrade = false)
        {
            this.InstallVCLibsDependencies();
            this.InstallUiXaml();

            StringBuilder sb = new StringBuilder();
            sb.Append(string.Format(AddAppxPackageFormat, localPath));

            if (downgrade)
            {
                sb.Append(ForceUpdateFromAnyVersion);
            }

            this.commandInvocation.InvokeScript(sb.ToString());
        }

        /// <summary>
        /// Calls Add-AppxPackage to register with AppInstaller's AppxManifest.xml.
        /// </summary>
        public void RegisterAppInstaller()
        {
            string packageFullName = this.GetAppInstallerPropertyValue(PackageFullName);
            string appxManifestPath = Path.Combine(
                Utilities.ProgramFilesWindowsAppPath,
                packageFullName,
                AppxManifest);

            this.commandInvocation.InvokeScript(
                string.Format(AddAppxPackageRegisterFormat, appxManifestPath));
        }

        private PSObject GetAppxObject(string packageName)
        {
            return this.commandInvocation
                .InvokeScript(string.Format(GetAppxPackageCommand, packageName))
                .FirstOrDefault();
        }

        private void InstallVCLibsDependencies()
        {
            var vcLibsPackageObjs = this.GetAppxObject(VCLibsUWPDesktop);
            if (vcLibsPackageObjs is null)
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

        private void InstallUiXaml()
        {
            // TODO: We need to follow up for Microsoft.UI.Xaml.2.7
            // downloading the nuget and extracting it doesn't sound like the right thing to do.
            var uiXamlObjs = this.GetAppxObject(UiXaml27);
            if (uiXamlObjs is null)
            {
                throw new PSNotImplementedException("TODO: message");
            }
        }
    }
}
