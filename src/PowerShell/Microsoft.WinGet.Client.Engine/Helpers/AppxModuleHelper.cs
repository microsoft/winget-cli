// -----------------------------------------------------------------------------
// <copyright file="AppxModuleHelper.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Helpers
{
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.IO.Compression;
    using System.Linq;
    using System.Management.Automation;
    using System.Runtime.InteropServices;
    using Microsoft.WinGet.Client.Engine.Common;
    using Microsoft.WinGet.Client.Engine.Properties;

    /// <summary>
    /// Helper to make calls to the Appx module.
    /// </summary>
    internal class AppxModuleHelper
    {
        // Cmdlets
        private const string ImportModule = "Import-Module";
        private const string GetAppxPackage = "Get-AppxPackage";
        private const string AddAppxPackage = "Add-AppxPackage";

        // Parameters name
        private const string Name = "Name";
        private const string Path = "Path";
        private const string ErrorAction = "ErrorAction";
        private const string WarningAction = "WarningAction";

        // Parameter Values
        private const string Appx = "Appx";
        private const string Stop = "Stop";
        private const string SilentlyContinue = "SilentlyContinue";

        // Options
        private const string UseWindowsPowerShell = "UseWindowsPowerShell";
        private const string ForceUpdateFromAnyVersion = "ForceUpdateFromAnyVersion";
        private const string Register = "Register";
        private const string DisableDevelopmentMode = "DisableDevelopmentMode";

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
        private const string UiXamlNuget = "https://globalcdn.nuget.org/packages/microsoft.ui.xaml.2.7.3.nupkg";
        private const string UiXamlNugetAppxPathFormat = @"{0}\tools\AppX\{1}\Release\Microsoft.UI.Xaml.2.7.appx";

        private readonly PSCmdlet psCmdlet;

        /// <summary>
        /// Initializes a new instance of the <see cref="AppxModuleHelper"/> class.
        /// </summary>
        /// <param name="psCmdlet">The calling cmdlet.</param>
        public AppxModuleHelper(PSCmdlet psCmdlet)
        {
            this.psCmdlet = psCmdlet;
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
        /// Installs AppInstaller froma GitHub release.
        /// If allUsers is true uses Add-AppxProvisionedPackage otherwise Add-AppxPacakge.
        /// </summary>
        /// <param name="versionTag">Version tag of GitHub release.</param>
        /// <param name="allUsers">If install for all users is needed.</param>
        public void Install(string versionTag, bool allUsers)
        {
            if (allUsers)
            {
                this.AddProvisionPackage(versionTag);
            }
            else
            {
                this.AddAppInstallerBundle(versionTag, false);
            }
        }

        /// <summary>
        /// Downgrades app installer.
        /// </summary>
        /// <param name="versionTag">Version tag of GitHub release.</param>
        /// <param name="allUsers">If install for all users is needed.</param>
        public void InstallDowngrade(string versionTag, bool allUsers)
        {
            this.AddAppInstallerBundle(versionTag, true);

            if (allUsers)
            {
                // TODO: what happen in add-provisioned with downgrade?
            }
        }

        /// <summary>
        /// Calls Add-AppxPackage to register with AppInstaller's AppxManifest.xml.
        /// </summary>
        public void RegisterAppInstaller()
        {
            string packageFullName = this.GetAppInstallerPropertyValue(PackageFullName);
            string appxManifestPath = System.IO.Path.Combine(
                Utilities.ProgramFilesWindowsAppPath,
                packageFullName,
                AppxManifest);

            _ = this.ExecuteAppxCmdlet(
                AddAppxPackage,
                new Dictionary<string, object>
                {
                    { Path, appxManifestPath },
                },
                new List<string>
                {
                    Register,
                    DisableDevelopmentMode,
                });
        }

        private void AddProvisionPackage(string versionTag)
        {
            // TODO: verify system.
            if (!Utilities.ExecutingAsAdministrator)
            {
                // Add-AppxProvisionedPackage
                throw new System.Exception("Admin bro");
            }

            using var bundle = new TempFile();
            using var licenseFile = new TempFile();

            var gitHubRelease = new GitHubRelease();
            gitHubRelease.DownloadRelease(versionTag, bundle.FullPath);
            gitHubRelease.DownloadLicense(versionTag, licenseFile.FullPath);

            this.VerifyDependencies();

            try
            {
                var ps = PowerShell.Create(RunspaceMode.CurrentRunspace);
                ps.AddCommand("Add-AppxProvisionedPackage")
                  .AddParameter("Online")
                  .AddParameter("PackagePath", bundle.FullPath)
                  .AddParameter("LicensePath", licenseFile.FullPath)
                  .AddParameter(ErrorAction, Stop)
                  .Invoke();
            }
            catch (RuntimeException e)
            {
                this.psCmdlet.WriteError(e.ErrorRecord);
                throw e;
            }
        }

        private void AddAppInstallerBundle(string versionTag, bool downgrade = false)
        {
            using var bundle = new TempFile();

            var gitHubRelease = new GitHubRelease();
            gitHubRelease.DownloadRelease(versionTag, bundle.FullPath);

            this.VerifyDependencies();

            var options = new List<string>();
            if (downgrade)
            {
                options.Add(ForceUpdateFromAnyVersion);
            }

            try
            {
                _ = this.ExecuteAppxCmdlet(
                    AddAppxPackage,
                    new Dictionary<string, object>
                    {
                        { Path, bundle.FullPath },
                        { ErrorAction, Stop },
                    },
                    options);
            }
            catch (RuntimeException e)
            {
                this.psCmdlet.WriteError(e.ErrorRecord);
                throw e;
            }
        }

        private void VerifyDependencies()
        {
            // A better implementation would use Add-AppxPackage with -DependencyPath, but
            // the Appx module needs to be remoted into Windows PowerShell. When the string[] parameter
            // gets deserialized from Core the result is a single string which breaks Add-AppxPackage.
            // Here we should: if we are in Windows Powershell then run Add-AppxPackage with -DependencyPath
            // if we are in Core, then start powershell.exe and run the same command. Right now, we just
            // do Add-AppxPackage for each one.
            this.InstallVCLibsDependencies();
            this.InstallUiXaml();
        }

        private PSObject GetAppxObject(string packageName)
        {
            return this.ExecuteAppxCmdlet(
                GetAppxPackage,
                new Dictionary<string, object>
                {
                    { Name, packageName },
                })
                .FirstOrDefault();
        }

        private IReadOnlyList<string> GetVCLibsDependencies()
        {
            var vcLibsDependencies = new List<string>();

            var result = this.ExecuteAppxCmdlet(
                GetAppxPackage,
                new Dictionary<string, object>
                {
                    { Name, VCLibsUWPDesktop },
                });

            // See if the required version is installed.
            bool isInstalled = false;
            if (result != null &&
                result.Count > 0)
            {
                foreach (dynamic psobject in result)
                {
                    if (psobject?.Version == VCLibsUWPDesktopVersion)
                    {
                        isInstalled = true;
                        break;
                    }
                }
            }

            if (!isInstalled)
            {
                this.psCmdlet.WriteDebug("Couldn't find required VCLibs package");
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
                this.AddAppxPackageAsUri(package);
            }
        }

        private void InstallUiXaml()
        {
            // TODO: We need to follow up for Microsoft.UI.Xaml.2.7
            // downloading the nuget and extracting it doesn't sound like the right thing to do.
            var uiXamlObjs = this.GetAppxObject(UiXaml27);
            if (uiXamlObjs is null)
            {
                // Download xaml nuget, extract and install.
                this.psCmdlet.WriteDebug("Downloading and installing Microsoft.UI.Xaml.2.7");
                using var tempFile = new TempFile();
                var githubRelease = new GitHubRelease();
                githubRelease.DownloadUrl(UiXamlNuget, tempFile.FullPath);

                using var tempDir = new TempDirectory();
                ZipFile.ExtractToDirectory(tempFile.FullPath, tempDir.FullDirectoryPath);

                var packagesToInstall = new List<string>();

                var arch = RuntimeInformation.OSArchitecture;
                if (arch == Architecture.X64 ||
                    arch == Architecture.X86)
                {
                    packagesToInstall.Add(string.Format(UiXamlNugetAppxPathFormat, tempDir.FullDirectoryPath, arch));
                }
                else if (arch == Architecture.Arm64)
                {
                    packagesToInstall.Add(string.Format(UiXamlNugetAppxPathFormat, tempDir.FullDirectoryPath, Architecture.X64));
                    packagesToInstall.Add(string.Format(UiXamlNugetAppxPathFormat, tempDir.FullDirectoryPath, Architecture.X86));
                    packagesToInstall.Add(string.Format(UiXamlNugetAppxPathFormat, tempDir.FullDirectoryPath, Architecture.Arm));
                    packagesToInstall.Add(string.Format(UiXamlNugetAppxPathFormat, tempDir.FullDirectoryPath, arch));
                }
                else
                {
                    throw new PSNotSupportedException(arch.ToString());
                }

                foreach (var package in packagesToInstall)
                {
                    _ = this.ExecuteAppxCmdlet(
                        AddAppxPackage,
                        new Dictionary<string, object>
                        {
                            { Path, package },
                            { ErrorAction, Stop },
                        });
                }
            }
        }

        private void AddAppxPackageAsUri(string packageUri)
        {
            try
            {
                _ = this.ExecuteAppxCmdlet(
                    AddAppxPackage,
                    new Dictionary<string, object>
                    {
                        { Path, packageUri },
                        { ErrorAction, Stop },
                    });
            }
            catch (RuntimeException e)
            {
                // If we couldn't install it via URI, try download and install.
                if (e.ErrorRecord.CategoryInfo.Category == ErrorCategory.OpenError)
                {
                    this.psCmdlet.WriteDebug($"Failed adding package [{packageUri}]. Retrying downloading it.");
                    this.DownloadPackageAndAdd(packageUri);
                }
                else
                {
                    this.psCmdlet.WriteError(e.ErrorRecord);
                    throw e;
                }
            }
        }

        private void DownloadPackageAndAdd(string packageUrl)
        {
            using var tempFile = new TempFile();

            // This is weird but easy.
            var githubRelease = new GitHubRelease();
            githubRelease.DownloadUrl(packageUrl, tempFile.FullPath);

            _ = this.ExecuteAppxCmdlet(
                AddAppxPackage,
                new Dictionary<string, object>
                {
                    { Path, tempFile.FullPath },
                    { ErrorAction, Stop },
                });
        }

        private Collection<PSObject> ExecuteAppxCmdlet(string cmdlet, Dictionary<string, object> parameters = null, IList<string> options = null)
        {
            var ps = PowerShell.Create(RunspaceMode.CurrentRunspace);

            // There's a bug in the Appx Module that it can't be loaded from Core in pre 10.0.22453.0 builds without
            // the -UseWindowsPowerShell option. In post 10.0.22453.0 builds there's really no difference between
            // using or not -UseWindowsPowerShell as it will automatically get loaded using WinPSCompatSession remoting session.
            // https://github.com/PowerShell/PowerShell/issues/13138.
            // Set warning action to silently continue to avoid the console with
            // 'Module Appx is loaded in Windows PowerShell using WinPSCompatSession remoting session'
#if !POWERSHELL_WINDOWS
            ps.AddCommand(ImportModule)
              .AddParameter(Name, Appx)
              .AddParameter(UseWindowsPowerShell)
              .AddParameter(WarningAction, SilentlyContinue)
              .AddStatement();
#endif

            string cmd = cmdlet;
            ps.AddCommand(cmdlet);

            if (parameters != null)
            {
                foreach (var p in parameters)
                {
                    cmd += $" -{p.Key} {p.Value}";
                }

                ps.AddParameters(parameters);
            }

            if (options != null)
            {
                foreach (var option in options)
                {
                    cmd += $" -{option}";
                    ps.AddParameter(option);
                }
            }

            this.psCmdlet.WriteDebug($"Executing Appx cmdlet {cmd}");
            var result = ps.Invoke();
            return result;
        }
    }
}
