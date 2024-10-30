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
    using System.IO;
    using System.IO.Compression;
    using System.Linq;
    using System.Management.Automation;
    using System.Runtime.InteropServices;
    using System.Threading.Tasks;
    using Microsoft.WinGet.Client.Engine.Common;
    using Microsoft.WinGet.Client.Engine.Exceptions;
    using Microsoft.WinGet.Client.Engine.Extensions;
    using Microsoft.WinGet.Common.Command;
    using Newtonsoft.Json;
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
        private const string Version = "Version";

        // Assets
        private const string MsixBundleName = "Microsoft.DesktopAppInstaller_8wekyb3d8bbwe.msixbundle";
        private const string DependenciesJsonName = "DesktopAppInstaller_Dependencies.json";
        private const string DependenciesZipName = "DesktopAppInstaller_Dependencies.zip";
        private const string License = "License1.xml";

        // Format of a dependency package such as 'x64\Microsoft.VCLibs.140.00.UWPDesktop_14.0.33728.0_x64.appx'
        private const string ExtractedDependencyPath = "{0}\\{1}_{2}_{0}.appx";

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
        /// <param name="releaseTag">Release tag of GitHub release.</param>
        public void RegisterAppInstaller(string releaseTag)
        {
            if (string.IsNullOrEmpty(releaseTag))
            {
                string? versionFromLocalPackage = this.GetAppInstallerPropertyValue(Version);

                if (versionFromLocalPackage == null)
                {
                    throw new ArgumentNullException(Version);
                }

                var packageVersion = new Version(versionFromLocalPackage);
                if (packageVersion.Major == 1 && packageVersion.Minor > 15)
                {
                    releaseTag = $"1.{packageVersion.Minor - 15}.{packageVersion.Build}";
                }
                else
                {
                    releaseTag = $"{packageVersion.Major}.{packageVersion.Minor}.{packageVersion.Build}";
                }
            }

            // Ensure that all dependencies are present when attempting to register.
            // If dependencies are missing, a provisioned package can appear to only need registration,
            // but will fail to register. `InstallDependenciesAsync` checks for the packages before
            // acting, so it should be mostly a no-op if they are already available.
            this.InstallDependenciesAsync(releaseTag).Wait();

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
            bool result = await this.InstallDependenciesFromGitHubArchive(releaseTag);

            if (!result)
            {
                // A better implementation would use Add-AppxPackage with -DependencyPath, but
                // the Appx module needs to be remoted into Windows PowerShell. When the string[] parameter
                // gets deserialized from Core the result is a single string which breaks Add-AppxPackage.
                // Here we should: if we are in Windows Powershell then run Add-AppxPackage with -DependencyPath
                // if we are in Core, then start powershell.exe and run the same command. Right now, we just
                // do Add-AppxPackage for each one.
                // This method no longer works for versions >1.9 as the vclibs url has been deprecated.
                await this.InstallVCLibsDependenciesFromUriAsync();
                await this.InstallUiXamlAsync(releaseTag);
            }
        }

        private Dictionary<string, string> GetDependenciesByArch(PackageDependency dependencies)
        {
            Dictionary<string, string> appxPackages = new Dictionary<string, string>();
            var arch = RuntimeInformation.OSArchitecture;

            string appxPackageX64 = string.Format(ExtractedDependencyPath, "x64", dependencies.Name, dependencies.Version);
            string appxPackageX86 = string.Format(ExtractedDependencyPath, "x86", dependencies.Name, dependencies.Version);
            string appxPackageArm = string.Format(ExtractedDependencyPath, "arm", dependencies.Name, dependencies.Version);
            string appxPackageArm64 = string.Format(ExtractedDependencyPath, "arm", dependencies.Name, dependencies.Version);

            if (arch == Architecture.X64)
            {
                appxPackages.Add("x64", appxPackageX64);
            }
            else if (arch == Architecture.X86)
            {
                appxPackages.Add("x86", appxPackageX86);
            }
            else if (arch == Architecture.Arm64)
            {
                // Deployment please figure out for me.
                appxPackages.Add("x64", appxPackageX64);
                appxPackages.Add("x86", appxPackageX86);
                appxPackages.Add("arm", appxPackageArm);
                appxPackages.Add("arm64", appxPackageArm64);
            }
            else
            {
                throw new PSNotSupportedException(arch.ToString());
            }

            return appxPackages;
        }

        private void FindMissingDependencies(Dictionary<string, string> dependencies, string packageName, string requiredVersion)
        {
            var result = this.ExecuteAppxCmdlet(
                GetAppxPackage,
                new Dictionary<string, object>
                {
                    { Name, packageName },
                });

            Version minimumVersion = new Version(requiredVersion);

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
                        string? architectureString = psobject?.Architecture?.ToString();
                        if (architectureString == null)
                        {
                            this.pwshCmdlet.Write(StreamType.Verbose, $"{packageName} dependency has no architecture value: {psobject?.PackageFullName ?? "<null>"}");
                            continue;
                        }

                        architectureString = architectureString.ToLower();

                        if (dependencies.ContainsKey(architectureString))
                        {
                            this.pwshCmdlet.Write(StreamType.Verbose, $"{packageName} {architectureString} dependency satisfied by: {psobject?.PackageFullName ?? "<null>"}");
                            dependencies.Remove(architectureString);
                        }
                    }
                    else
                    {
                        this.pwshCmdlet.Write(StreamType.Verbose, $"{packageName} is lower than minimum required version [{minimumVersion}]: {psobject?.PackageFullName ?? "<null>"}");
                    }
                }
            }
        }

        private async Task InstallVCLibsDependenciesFromUriAsync()
        {
            Dictionary<string, string> vcLibsDependencies = this.GetVCLibsDependencies();
            this.FindMissingDependencies(vcLibsDependencies, VCLibsUWPDesktop, VCLibsUWPDesktopVersion);

            if (vcLibsDependencies.Count != 0)
            {
                this.pwshCmdlet.Write(StreamType.Verbose, "Couldn't find required VCLibs packages");

                foreach (var vclibPair in vcLibsDependencies)
                {
                    string vclib = vclibPair.Value;
                    await this.AddAppxPackageAsUriAsync(vclib, vclib.Substring(vclib.LastIndexOf('/') + 1));
                }
            }
            else
            {
                this.pwshCmdlet.Write(StreamType.Verbose, $"VCLibs are updated.");
            }
        }

        // Returns a boolean value indicating whether dependencies were successfully installed from the GitHub release assets.
        private async Task<bool> InstallDependenciesFromGitHubArchive(string releaseTag)
        {
            var githubClient = new GitHubClient(RepositoryOwner.Microsoft, RepositoryName.WinGetCli);
            var release = await githubClient.GetReleaseAsync(releaseTag);

            ReleaseAsset? dependenciesJsonAsset = release.TryGetAsset(DependenciesJsonName);
            if (dependenciesJsonAsset is null)
            {
                return false;
            }

            using var dependenciesJsonFile = new TempFile();
            await this.httpClientHelper.DownloadUrlWithProgressAsync(dependenciesJsonAsset.BrowserDownloadUrl, dependenciesJsonFile.FullPath, this.pwshCmdlet);

            using StreamReader r = new StreamReader(dependenciesJsonFile.FullPath);
            string json = r.ReadToEnd();
            WingetDependencies? wingetDependencies = JsonConvert.DeserializeObject<WingetDependencies>(json);

            if (wingetDependencies is null)
            {
                this.pwshCmdlet.Write(StreamType.Verbose, $"Failed to deserialize dependencies json file.");
                return false;
            }

            List<string> missingDependencies = new List<string>();
            foreach (var dependency in wingetDependencies.Dependencies)
            {
                Dictionary<string, string> dependenciesByArch = this.GetDependenciesByArch(dependency);
                this.FindMissingDependencies(dependenciesByArch, dependency.Name, dependency.Version);

                foreach (var pair in dependenciesByArch)
                {
                    missingDependencies.Add(pair.Value);
                }
            }

            if (missingDependencies.Count != 0)
            {
                using var dependenciesZipFile = new TempFile();
                using var extractedDirectory = new TempDirectory();

                ReleaseAsset? dependenciesZipAsset = release.TryGetAsset(DependenciesZipName);
                if (dependenciesZipAsset is null)
                {
                    this.pwshCmdlet.Write(StreamType.Verbose, $"Dependencies zip asset not found on GitHub asset.");
                    return false;
                }

                await this.httpClientHelper.DownloadUrlWithProgressAsync(dependenciesZipAsset.BrowserDownloadUrl, dependenciesZipFile.FullPath, this.pwshCmdlet);
                ZipFile.ExtractToDirectory(dependenciesZipFile.FullPath, extractedDirectory.FullDirectoryPath);

                foreach (var entry in missingDependencies)
                {
                    string fullPath = System.IO.Path.Combine(extractedDirectory.FullDirectoryPath, entry);
                    if (!File.Exists(fullPath))
                    {
                        this.pwshCmdlet.Write(StreamType.Verbose, $"Package dependency not found in archive: {fullPath}");
                        return false;
                    }

                    _ = this.ExecuteAppxCmdlet(
                            AddAppxPackage,
                            new Dictionary<string, object>
                            {
                            { Path, fullPath },
                            { ErrorAction, Stop },
                            });
                }
            }

            return true;
        }

        private Dictionary<string, string> GetVCLibsDependencies()
        {
            Dictionary<string, string> vcLibsDependencies = new Dictionary<string, string>();
            var arch = RuntimeInformation.OSArchitecture;
            if (arch == Architecture.X64)
            {
                vcLibsDependencies.Add("x64", VCLibsUWPDesktopX64);
            }
            else if (arch == Architecture.X86)
            {
                vcLibsDependencies.Add("x86", VCLibsUWPDesktopX86);
            }
            else if (arch == Architecture.Arm64)
            {
                // Deployment please figure out for me.
                vcLibsDependencies.Add("x64", VCLibsUWPDesktopX64);
                vcLibsDependencies.Add("x86", VCLibsUWPDesktopX86);
                vcLibsDependencies.Add("arm", VCLibsUWPDesktopArm);
                vcLibsDependencies.Add("arm64", VCLibsUWPDesktopArm64);
            }
            else
            {
                throw new PSNotSupportedException(arch.ToString());
            }

            return vcLibsDependencies;
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
