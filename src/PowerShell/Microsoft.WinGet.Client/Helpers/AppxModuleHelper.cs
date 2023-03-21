// -----------------------------------------------------------------------------
// <copyright file="AppxModuleHelper.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Helpers
{
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using System.Management.Automation;
    using System.Management.Automation.Runspaces;
    using System.Runtime.InteropServices;
    using System.Text;
    using Microsoft.WinGet.Client.Common;
    using Microsoft.WinGet.Client.Properties;

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
        private const string GetAppxPackageByVersionCommand = "Get-AppxPackage {0} | Where-Object -Property Version -eq {1}";

        private const string AppInstallerName = "Microsoft.DesktopAppInstaller";
        private const string AppxManifest = "AppxManifest.xml";
        private const string PackageFullName = "PackageFullName";

        // Dependencies
        private const string VCLibsUWPDesktop = "Microsoft.VCLibs.140.00.UWPDesktop";
        private const string VCLibsUWPDesktopVersion = "14.0.30704.0";
        private const string VCLibsUWPDesktopX64 = "https://aka.ms/Microsoft.VCLibs.x64.14.00.Desktop.appx";
        private const string VCLibsUWPDesktopX86 = "https://aka.ms/Microsoft.VCLibs.x86.14.00.Desktop.appx";
        private const string VCLibsUWPDesktopArm = "https://aka.ms/Microsoft.VCLibs.arm.14.00.Desktop.appx";
        private const string VCLibsUWPDesktopArm64 = "https://aka.ms/Microsoft.VCLibs.arm64.14.00.Desktop.appx";

        private const string UiXaml27 = "Microsoft.UI.Xaml.2.7";

        private readonly PSCmdlet psCmdlet;

        /// <summary>
        /// Initializes a new instance of the <see cref="AppxModuleHelper"/> class.
        /// </summary>
        /// <param name="psCmdlet">The calling cmdlet.</param>
        public AppxModuleHelper(PSCmdlet psCmdlet)
        {
            this.psCmdlet = psCmdlet;

            // There's a bug in the Appx Module that it can't be loaded from Core in pre 10.0.22453.0 builds without
            // the -UseWindowsPowerShell option. In post 10.0.22453.0 builds there's really no difference between
            // using or not -UseWindowsPowerShell as it will automatically get loaded using WinPSCompatSession remoting session.
            // https://github.com/PowerShell/PowerShell/issues/13138.
#if !POWERSHELL_WINDOWS
            var appxModule = this.psCmdlet.InvokeCommand.InvokeScript(GetAppxModule);
            if (appxModule is null)
            {
                this.psCmdlet.InvokeCommand.InvokeScript(ImportModuleCore);
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
            // A better implementation would use Add-AppxPackage with -DependencyPath, but
            // the Appx module needs to be remoted into Windows PowerShell. When the string[] parameter
            // gets deserialized from Core the result is a single string which breaks Add-AppxPackage.
            // Here we should: if we are in Windows Powershell then run Add-AppxPackage with -DependencyPath
            // if we are in Core, then start powershell.exe and run the same command. Right now, we just
            // do Add-AppxPackage for each one.
            this.InstallVCLibsDependencies();
            this.InstallUiXaml();

            StringBuilder sb = new StringBuilder();
            sb.Append(string.Format(AddAppxPackageFormat, localPath));

            if (downgrade)
            {
                sb.Append(ForceUpdateFromAnyVersion);
            }

            // Using this method simplifies a lot of things, but the error is not propagated with
            // the default parameters. PipelineResultTypes.Error will at least output it in the terminal.
            this.psCmdlet.InvokeCommand.InvokeScript(
                sb.ToString(),
                useNewScope: true,
                PipelineResultTypes.Error,
                input: null);
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

            this.psCmdlet.InvokeCommand.InvokeScript(
                string.Format(AddAppxPackageRegisterFormat, appxManifestPath));
        }

        private PSObject GetAppxObject(string packageName)
        {
            return this.psCmdlet.InvokeCommand
                .InvokeScript(string.Format(GetAppxPackageCommand, packageName))
                .FirstOrDefault();
        }

        private IReadOnlyList<string> GetVCLibsDependencies()
        {
            var vcLibsDependencies = new List<string>();
            var vcLibsPackageObjs = this.psCmdlet.InvokeCommand
                .InvokeScript(string.Format(GetAppxPackageByVersionCommand, VCLibsUWPDesktop, VCLibsUWPDesktopVersion));
            if (vcLibsPackageObjs is null ||
                vcLibsPackageObjs.Count == 0)
            {
                var arch = RuntimeInformation.OSArchitecture;
                if (arch == Architecture.X64)
                {
                    vcLibsDependencies.Add(VCLibsUWPDesktopX64);
                }
                else if (arch == Architecture.X86)
                {
                    vcLibsDependencies.Add(VCLibsUWPDesktopX86);
                }
                else if (arch == Architecture.Arm64)
                {
                    // Deployment please figure out for me.
                    vcLibsDependencies.Add(VCLibsUWPDesktopX64);
                    vcLibsDependencies.Add(VCLibsUWPDesktopX86);
                    vcLibsDependencies.Add(VCLibsUWPDesktopArm);
                    vcLibsDependencies.Add(VCLibsUWPDesktopArm64);
                }
                else
                {
                    throw new PSNotSupportedException(arch.ToString());
                }
            }
            else
            {
                this.psCmdlet.WriteDebug($"VCLibs are updated.");
            }

            return vcLibsDependencies;
        }

        private void InstallVCLibsDependencies()
        {
            var packages = this.GetVCLibsDependencies();
            foreach (var package in packages)
            {
                this.psCmdlet.WriteDebug($"Installing VCLibs {package}");
                this.psCmdlet.InvokeCommand.InvokeScript(string.Format(AddAppxPackageFormat, package));
            }
        }

        private void InstallUiXaml()
        {
            // TODO: We need to follow up for Microsoft.UI.Xaml.2.7
            // downloading the nuget and extracting it doesn't sound like the right thing to do.
            var uiXamlObjs = this.GetAppxObject(UiXaml27);
            if (uiXamlObjs is null)
            {
                throw new PSNotImplementedException(Resources.MicrosoftUIXaml27Message);
            }
        }
    }
}
