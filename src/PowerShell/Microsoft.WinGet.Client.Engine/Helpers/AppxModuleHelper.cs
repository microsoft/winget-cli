// -----------------------------------------------------------------------------
// <copyright file="AppxModuleHelper.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Helpers
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Linq;
    using System.Management.Automation;
    using System.Runtime.InteropServices;
    using System.Threading.Tasks;
    using Microsoft.WinGet.Client.Engine.Common;
    using Microsoft.WinGet.Client.Engine.Extensions;
    using Microsoft.WinGet.Common.Command;
    using Octokit;
    using Semver;
    using static Microsoft.WinGet.Client.Engine.Common.Constants;

    /// <summary>
    /// Helper to make calls to the Appx module.
    /// </summary>
    internal class AppxModuleHelper
    {
        // Cmdlets
        private const string ImportModule = "Import-Module";
        private const string GetAppxPackage = "Get-AppxPackage";
        private const string AddAppxPackage = "Add-AppxPackage";
        private const string AddAppxProvisionedPackage = "Add-AppxProvisionedPackage";
        private const string GetCommand = "Get-Command";

        // Parameters name
        private const string Name = "Name";
        private const string Path = "Path";
        private const string ErrorAction = "ErrorAction";
        private const string WarningAction = "WarningAction";
        private const string PackagePath = "PackagePath";
        private const string LicensePath = "LicensePath";
        private const string Module = "Module";
        private const string StubPackageOption = "StubPackageOption";

        // Parameter Values
        private const string Appx = "Appx";
        private const string Stop = "Stop";
        private const string SilentlyContinue = "SilentlyContinue";
        private const string Online = "Online";
        private const string UsePreference = "UsePreference";

        // Options
        private const string UseWindowsPowerShell = "UseWindowsPowerShell";
        private const string ForceUpdateFromAnyVersion = "ForceUpdateFromAnyVersion";
        private const string Register = "Register";
        private const string DisableDevelopmentMode = "DisableDevelopmentMode";
        private const string ForceTargetApplicationShutdown = "ForceTargetApplicationShutdown";

        private const string AppInstallerName = "Microsoft.DesktopAppInstaller";
        private const string AppxManifest = "AppxManifest.xml";
        private const string PackageFullName = "PackageFullName";

        // Assets
        private const string MsixBundleName = "Microsoft.DesktopAppInstaller_8wekyb3d8bbwe.msixbundle";
        private const string License = "License1.xml";

        // Dependencies
        // VCLibs
        private const string VCLibsUWPDesktop = "Microsoft.VCLibs.140.00.UWPDesktop";
        private const string VCLibsUWPDesktopVersion = "14.0.30704.0";
        private const string VCLibsUWPDesktopX64 = "https://aka.ms/Microsoft.VCLibs.x64.14.00.Desktop.appx";
        private const string VCLibsUWPDesktopX86 = "https://aka.ms/Microsoft.VCLibs.x86.14.00.Desktop.appx";
        private const string VCLibsUWPDesktopArm = "https://aka.ms/Microsoft.VCLibs.arm.14.00.Desktop.appx";
        private const string VCLibsUWPDesktopArm64 = "https://aka.ms/Microsoft.VCLibs.arm64.14.00.Desktop.appx";

        // Xaml
        private const string XamlPackage28 = "Microsoft.UI.Xaml.2.8";
        private const string XamlReleaseTag286 = "v2.8.6";
        private const string MinimumWinGetReleaseTagForXaml28 = "v1.7.10514";

        private const string XamlPackage27 = "Microsoft.UI.Xaml.2.7";
        private const string XamlReleaseTag273 = "v2.7.3";

        private readonly PowerShellCmdlet pwshCmdlet;
        private readonly HttpClientHelper httpClientHelper;

        /// <summary>
        /// Initializes a new instance of the <see cref="AppxModuleHelper"/> class.
        /// </summary>
        /// <param name="pwshCmdlet">The calling cmdlet.</param>
        public AppxModuleHelper(PowerShellCmdlet pwshCmdlet)
        {
            this.pwshCmdlet = pwshCmdlet;
            this.httpClientHelper = new HttpClientHelper();
        }

        /// <summary>
        /// Calls Get-AppxPackage Microsoft.DesktopAppInstaller.
        /// </summary>
        /// <returns>Result of Get-AppxPackage.</returns>
        public PSObject? GetAppInstallerObject()
        {
            return this.GetAppxObject(AppInstallerName);
        }

        /// <summary>
        /// Gets the string value a property from the Get-AppxPackage object of AppInstaller.
        /// </summary>
        /// <param name="propertyName">Property name.</param>
        /// <returns>Value, null if doesn't exist.</returns>
        public string? GetAppInstallerPropertyValue(string propertyName)
        {
            string? result = null;
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
        /// Calls Add-AppxPackage to register with AppInstaller's AppxManifest.xml.
        /// </summary>
        public void RegisterAppInstaller()
        {
            string? packageFullName = this.GetAppInstallerPropertyValue(PackageFullName);

            if (packageFullName == null)
            {
                throw new ArgumentNullException(PackageFullName);
            }

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

        /// <summary>
        /// Install AppInstaller's bundle from a GitHub release.
        /// </summary>
        /// <param name="releaseTag">Release tag of GitHub release.</param>
        /// <param name="allUsers">If install for all users is needed.</param>
        /// <param name="isDowngrade">Is downgrade.</param>
        /// <param name="force">Force application shutdown.</param>
        /// <returns>A <see cref="Task"/> representing the asynchronous operation.</returns>
        public async Task InstallFromGitHubReleaseAsync(string releaseTag, bool allUsers, bool isDowngrade, bool force)
        {
            await this.InstallDependenciesAsync(releaseTag);

            if (isDowngrade)
            {
                // Add-AppxProvisionedPackage doesn't support downgrade.
                await this.AddAppInstallerBundleAsync(releaseTag, true, force);

                if (allUsers)
                {
                    await this.AddProvisionPackageAsync(releaseTag);
                }
            }
            else
            {
                if (allUsers)
                {
                    await this.AddProvisionPackageAsync(releaseTag);
                }
                else
                {
                    await this.AddAppInstallerBundleAsync(releaseTag, false, force);
                }
            }
        }

        /// <summary>
        /// Gets the Xaml dependency package name and release tag based on the provided WinGet release tag.
        /// </summary>
        /// <param name="releaseTag">WinGet release tag.</param>
        /// <returns>A tuple in the format of (XamlPackageName, XamlReleaseTag).</returns>
        private static Tuple<string, string> GetXamlDependencyVersionInfo(string releaseTag)
        {
            var targetVersion = SemVersion.Parse(releaseTag, SemVersionStyles.AllowLowerV);

            if (targetVersion.CompareSortOrderTo(SemVersion.Parse(MinimumWinGetReleaseTagForXaml28, SemVersionStyles.AllowLowerV)) >= 0)
            {
                return Tuple.Create(XamlPackage28, XamlReleaseTag286);
            }
            else
            {
                return Tuple.Create(XamlPackage27, XamlReleaseTag273);
            }
        }

        private async Task AddProvisionPackageAsync(string releaseTag)
        {
            var githubClient = new GitHubClient(RepositoryOwner.Microsoft, RepositoryName.WinGetCli);
            var release = await githubClient.GetReleaseAsync(releaseTag);

            var bundleAsset = release.GetAsset(MsixBundleName);
            using var bundleFile = new TempFile(fileName: MsixBundleName);
            await this.httpClientHelper.DownloadUrlWithProgressAsync(
                bundleAsset.BrowserDownloadUrl, bundleFile.FullPath, this.pwshCmdlet);

            var licenseAsset = release.GetAssetEndsWith(License);
            using var licenseFile = new TempFile(fileName: licenseAsset.Name);
            await this.httpClientHelper.DownloadUrlWithProgressAsync(
                licenseAsset.BrowserDownloadUrl, licenseFile.FullPath, this.pwshCmdlet);

            try
            {
                this.pwshCmdlet.ExecuteInPowerShellThread(
                    () =>
                    {
                        var ps = PowerShell.Create(RunspaceMode.CurrentRunspace);
                        ps.AddCommand(AddAppxProvisionedPackage)
                          .AddParameter(Online)
                          .AddParameter(PackagePath, bundleFile.FullPath)
                          .AddParameter(LicensePath, licenseFile.FullPath)
                          .AddParameter(ErrorAction, Stop)
                          .Invoke();
                    });
            }
            catch (RuntimeException e)
            {
                this.pwshCmdlet.Write(StreamType.Verbose, $"Failed installing bundle via Add-AppxProvisionedPackage {e}");
                throw;
            }
        }

        private async Task AddAppInstallerBundleAsync(string releaseTag, bool downgrade, bool force)
        {
            var options = new List<string>();
            if (downgrade)
            {
                options.Add(ForceUpdateFromAnyVersion);
            }

            if (force)
            {
                options.Add(ForceTargetApplicationShutdown);
            }

            var parameters = new Dictionary<string, object>();
            if (this.IsStubPackageOptionPresent())
            {
                parameters.Add(StubPackageOption, UsePreference);
            }

            try
            {
                var githubClient = new GitHubClient(RepositoryOwner.Microsoft, RepositoryName.WinGetCli);
                var release = await githubClient.GetReleaseAsync(releaseTag);

                var bundleAsset = release.GetAsset(MsixBundleName);
                await this.AddAppxPackageAsUriAsync(bundleAsset.BrowserDownloadUrl, MsixBundleName, parameters, options);
            }
            catch (RuntimeException e)
            {
                this.pwshCmdlet.Write(StreamType.Verbose, $"Failed installing bundle via Add-AppxPackage {e}");
                throw;
            }
        }

        private PSObject? GetAppxObject(string packageName)
        {
            return this.ExecuteAppxCmdlet(
                GetAppxPackage,
                new Dictionary<string, object>
                {
                    { Name, packageName },
                })
                .FirstOrDefault();
        }

        private async Task InstallDependenciesAsync(string releaseTag)
        {
            // A better implementation would use Add-AppxPackage with -DependencyPath, but
            // the Appx module needs to be remoted into Windows PowerShell. When the string[] parameter
            // gets deserialized from Core the result is a single string which breaks Add-AppxPackage.
            // Here we should: if we are in Windows Powershell then run Add-AppxPackage with -DependencyPath
            // if we are in Core, then start powershell.exe and run the same command. Right now, we just
            // do Add-AppxPackage for each one.
            await this.InstallVCLibsDependenciesAsync();
            await this.InstallUiXamlAsync(releaseTag);
        }

        private async Task InstallVCLibsDependenciesAsync()
        {
            var result = this.ExecuteAppxCmdlet(
                GetAppxPackage,
                new Dictionary<string, object>
                {
                    { Name, VCLibsUWPDesktop },
                });

            // See if the minimum (or greater) version is installed.
            // TODO: Pull the minimum version from the target package
            // TODO: This does not check architecture of the package
            Version minimumVersion = new Version(VCLibsUWPDesktopVersion);

            bool isInstalled = false;
            if (result != null &&
                result.Count > 0)
            {
                foreach (dynamic psobject in result)
                {
                    string? versionString = psobject?.Version?.ToString();
                    if (versionString == null)
                    {
                        continue;
                    }

                    Version packageVersion = new Version(versionString);

                    if (packageVersion >= minimumVersion)
                    {
                        this.pwshCmdlet.Write(StreamType.Verbose, $"VCLibs dependency satisfied by: {psobject?.PackageFullName ?? "<null>"}");
                        isInstalled = true;
                        break;
                    }
                }
            }

            if (!isInstalled)
            {
                this.pwshCmdlet.Write(StreamType.Verbose, "Couldn't find required VCLibs package");

                var vcLibsDependencies = new List<string>();
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

                foreach (var vclib in vcLibsDependencies)
                {
                    await this.AddAppxPackageAsUriAsync(vclib, vclib.Substring(vclib.LastIndexOf('/') + 1));
                }
            }
            else
            {
                this.pwshCmdlet.Write(StreamType.Verbose, $"VCLibs are updated.");
            }
        }

        private async Task InstallUiXamlAsync(string releaseTag)
        {
            (string xamlPackageName, string xamlReleaseTag) = GetXamlDependencyVersionInfo(releaseTag);
            string xamlAssetX64 = string.Format("{0}.x64.appx", xamlPackageName);
            string xamlAssetX86 = string.Format("{0}.x86.appx", xamlPackageName);
            string xamlAssetArm = string.Format("{0}.arm.appx", xamlPackageName);
            string xamlAssetArm64 = string.Format("{0}.arm64.appx", xamlPackageName);

            var uiXamlObjs = this.GetAppxObject(xamlPackageName);
            if (uiXamlObjs is null)
            {
                var githubRelease = new GitHubClient(RepositoryOwner.Microsoft, RepositoryName.UiXaml);

                var xamlRelease = await githubRelease.GetReleaseAsync(xamlReleaseTag);

                var packagesToInstall = new List<ReleaseAsset>();
                var arch = RuntimeInformation.OSArchitecture;
                if (arch == Architecture.X64)
                {
                    packagesToInstall.Add(xamlRelease.GetAsset(xamlAssetX64));
                }
                else if (arch == Architecture.X86)
                {
                    packagesToInstall.Add(xamlRelease.GetAsset(xamlAssetX86));
                }
                else if (arch == Architecture.Arm64)
                {
                    // Deployment please figure out for me.
                    packagesToInstall.Add(xamlRelease.GetAsset(xamlAssetX64));
                    packagesToInstall.Add(xamlRelease.GetAsset(xamlAssetX86));
                    packagesToInstall.Add(xamlRelease.GetAsset(xamlAssetArm));
                    packagesToInstall.Add(xamlRelease.GetAsset(xamlAssetArm64));
                }
                else
                {
                    throw new PSNotSupportedException(arch.ToString());
                }

                foreach (var package in packagesToInstall)
                {
                    await this.AddAppxPackageAsUriAsync(package.BrowserDownloadUrl, package.Name);
                }
            }
        }

        private async Task AddAppxPackageAsUriAsync(string packageUri, string fileName, Dictionary<string, object>? parameters = null, IList<string>? options = null)
        {
            try
            {
                var thisParams = new Dictionary<string, object>
                {
                    { Path, packageUri },
                    { ErrorAction, Stop },
                };

                if (parameters != null)
                {
                    foreach (var param in parameters)
                    {
                        thisParams.Add(param.Key, param.Value);
                    }
                }

                _ = this.ExecuteAppxCmdlet(
                        AddAppxPackage,
                        thisParams,
                        options);
            }
            catch (RuntimeException e)
            {
                // If we couldn't install it via URI, try download and install.
                if (e.ErrorRecord.CategoryInfo.Category == ErrorCategory.OpenError)
                {
                    this.pwshCmdlet.Write(StreamType.Verbose, $"Failed adding package [{packageUri}]. Retrying downloading it.");
                    await this.DownloadPackageAndAddAsync(packageUri, fileName, options);
                }
                else
                {
                    this.pwshCmdlet.Write(StreamType.Error, e.ErrorRecord);
                    throw;
                }
            }
        }

        private async Task DownloadPackageAndAddAsync(string packageUrl, string fileName, IList<string>? options)
        {
            using var tempFile = new TempFile(fileName: fileName);

            await this.httpClientHelper.DownloadUrlWithProgressAsync(packageUrl, tempFile.FullPath, this.pwshCmdlet);

            _ = this.ExecuteAppxCmdlet(
                    AddAppxPackage,
                    new Dictionary<string, object>
                    {
                        { Path, tempFile.FullPath },
                        { ErrorAction, Stop },
                    },
                    options);
        }

        private Collection<PSObject> ExecuteAppxCmdlet(string cmdlet, Dictionary<string, object>? parameters = null, IList<string>? options = null)
        {
            Collection<PSObject> result = new Collection<PSObject>();

            this.pwshCmdlet.ExecuteInPowerShellThread(
                () =>
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

                    this.pwshCmdlet.Write(StreamType.Verbose, $"Executing Appx cmdlet {cmd}");
                    result = ps.Invoke();
                });

            return result;
        }

        private bool IsStubPackageOptionPresent()
        {
            bool result = false;
            this.pwshCmdlet.ExecuteInPowerShellThread(
                () =>
                {
                    var ps = PowerShell.Create(RunspaceMode.CurrentRunspace);

#if !POWERSHELL_WINDOWS
                    ps.AddCommand(ImportModule)
                      .AddParameter(Name, Appx)
                      .AddParameter(UseWindowsPowerShell)
                      .AddParameter(WarningAction, SilentlyContinue)
                      .AddStatement();
#endif

                    var cmdInfo = ps.AddCommand(GetCommand)
                                    .AddParameter(Name, AddAppxPackage)
                                    .AddParameter(Module, Appx)
                                    .Invoke<CommandInfo>()
                                    .FirstOrDefault();

                    result = cmdInfo != null && cmdInfo.Parameters.ContainsKey(StubPackageOption);
                });

            return result;
        }
    }
}
