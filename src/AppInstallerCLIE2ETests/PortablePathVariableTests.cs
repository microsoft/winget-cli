// -----------------------------------------------------------------------------
// <copyright file="PortablePathVariableTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests
{
    using System;
    using System.Diagnostics;
    using System.IO;
    using System.Linq;
    using System.Threading.Tasks;
    using AppInstallerCLIE2ETests.Helpers;
    using Microsoft.Management.Deployment;
    using Microsoft.Management.Deployment.Projection;
    using Microsoft.Win32;
    using NUnit.Framework;

    /// <summary>
    /// End-to-end tests for PATH persistence behavior, verifying environment-variable
    /// unexpansion, scope isolation, deduplication, legacy migration, refresh, and archive portables.
    /// </summary>
    public class PortablePathVariableTests : BaseCommand
    {
        /// <summary>
        /// Scenario 1: User-scope install stores an environment-variable PATH entry.
        /// Verify raw value contains %LOCALAPPDATA%\Microsoft\WinGet\Links; and not expanded profile path.
        /// Verify expanded registry read resolves to Links dir and portable command is available via PATH.
        /// </summary>
        [Test]
        public void UserScopeInstall_StoresEnvironmentVariablePathEntry()
        {
            string packageId = "AppInstallerTest.TestPortableExe";
            string userLinksDir = TestCommon.GetPortableSymlinkDirectory(TestCommon.Scope.User);
            string userLinksDirClean = userLinksDir.TrimEnd('\\') + ';';
            string expectedRawEntry = TestCommon.GetUnexpandedPath(userLinksDir, TestCommon.Scope.User).TrimEnd('\\') + ';';
            string symlinkPath = Path.Combine(userLinksDir, Constants.AppInstallerTestExeInstallerExe);

            string originalRawPath = TestCommon.GetRawPathValue(TestCommon.Scope.User);
            var originalKind = TestCommon.GetPathRegisterValueKind(TestCommon.Scope.User);

            try
            {
                var installResult = TestCommon.RunAICLICommand("install", packageId);
                Assert.That(installResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
                Assert.That(installResult.StdOut, Does.Contain("Successfully installed"));

                // Inspect HKCU\Environment\Path with environment-variable expansion disabled
                string rawPath = TestCommon.GetRawPathValue(TestCommon.Scope.User);
                Assert.That(rawPath, Does.Contain(expectedRawEntry), $"Raw PATH value should contain {expectedRawEntry}");
                Assert.That(rawPath, Does.Not.Contain(userLinksDirClean), "Raw PATH value should not contain the expanded absolute profile path.");

                // Verify registry value kind is REG_EXPAND_SZ
                RegistryValueKind valueKind = TestCommon.GetPathRegisterValueKind(TestCommon.Scope.User);
                Assert.That(valueKind, Is.EqualTo(RegistryValueKind.ExpandString), "PATH registry value kind should be REG_EXPAND_SZ");

                // Verify normal expanded registry read resolves to the current Links directory
                string expandedPath = TestCommon.GetExpandedPathValue(TestCommon.Scope.User);
                Assert.That(expandedPath, Does.Contain(userLinksDirClean), "Expanded PATH should contain the resolved Links directory.");

                // Verify portable command is available on disk
                Assert.That(File.Exists(symlinkPath), Is.True, $"Portable command symlink should exist at: {symlinkPath}");

                // Verify command is available and executable via PATH lookup
                string refreshedPath = TestCommon.GetExpandedPathValue(TestCommon.Scope.Machine).TrimEnd(';') + ";" +
                                       TestCommon.GetExpandedPathValue(TestCommon.Scope.User);
                ProcessStartInfo startInfo = new ProcessStartInfo("cmd.exe", $"/c {Constants.AppInstallerTestExeInstallerExe} /NoOperation")
                {
                    UseShellExecute = false,
                    CreateNoWindow = true,
                };
                startInfo.Environment["PATH"] = refreshedPath;
                using Process process = Process.Start(startInfo);
                Assert.That(process, Is.Not.Null, "Process should start successfully.");
                process.WaitForExit();
                Assert.That(process.ExitCode, Is.EqualTo(0), "Portable command should be resolved and executed successfully via refreshed PATH.");
            }
            finally
            {
                TestCommon.RunAICLICommand("uninstall", $"{packageId} --force");
                TestCommon.SetPathRegisterValue(originalRawPath, TestCommon.Scope.User, originalKind);
            }
        }

        /// <summary>
        /// Scenario 2: Machine-scope install does not store user-scoped variables.
        /// Verify raw entry does not use %LOCALAPPDATA%, %APPDATA%, or %USERPROFILE%.
        /// Verify appropriate system variable (%ProgramFiles% or %ProgramData%) is used.
        /// </summary>
        [Test]
        public void MachineScopeInstall_DoesNotStoreUserScopedVariables()
        {
            if (!TestCommon.ExecutingAsAdministrator)
            {
                Assert.Ignore("Machine scope test requires administrator privilege.");
            }

            string packageId = "AppInstallerTest.TestPortableExe";
            string machineLinksDir = TestCommon.GetPortableSymlinkDirectory(TestCommon.Scope.Machine);
            string machineLinksDirClean = machineLinksDir.TrimEnd('\\') + ';';
            string expectedRawEntry = TestCommon.GetUnexpandedPath(machineLinksDir, TestCommon.Scope.Machine).TrimEnd('\\') + ';';

            string originalRawPath = TestCommon.GetRawPathValue(TestCommon.Scope.Machine);
            var originalKind = TestCommon.GetPathRegisterValueKind(TestCommon.Scope.Machine);

            try
            {
                var installResult = TestCommon.RunAICLICommand("install", $"{packageId} --scope machine");
                Assert.That(installResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
                Assert.That(installResult.StdOut, Does.Contain("Successfully installed"));

                // Inspect HKLM\System\CurrentControlSet\Control\Session Manager\Environment\Path
                string rawMachinePath = TestCommon.GetRawPathValue(TestCommon.Scope.Machine);
                Assert.That(rawMachinePath, Does.Not.Contain("%LOCALAPPDATA%"), "Machine PATH must not contain %LOCALAPPDATA%");
                Assert.That(rawMachinePath, Does.Not.Contain("%APPDATA%"), "Machine PATH must not contain %APPDATA%");
                Assert.That(rawMachinePath, Does.Not.Contain("%USERPROFILE%"), "Machine PATH must not contain %USERPROFILE%");

                // Verify an appropriate system variable is used
                Assert.That(rawMachinePath, Does.Contain(expectedRawEntry), $"Machine PATH should contain derived system variable entry {expectedRawEntry}");

                // Verify expanded PATH resolves to machine Links directory
                string expandedMachinePath = TestCommon.GetExpandedPathValue(TestCommon.Scope.Machine);
                Assert.That(expandedMachinePath, Does.Contain(machineLinksDirClean), "Expanded machine PATH should contain resolved Links directory.");
            }
            finally
            {
                TestCommon.RunAICLICommand("uninstall", $"{packageId} --scope machine --force");
                TestCommon.SetPathRegisterValue(originalRawPath, TestCommon.Scope.Machine, originalKind);
            }
        }

        /// <summary>
        /// Scenario 3: Uninstall and cleanup.
        /// Verify that uninstall removes the variable-form PATH entry and leaves unrelated PATH entries intact.
        /// Inspect both raw and expanded registry values.
        /// </summary>
        [Test]
        public void Uninstall_RemovesVariableFormPathEntryAndPreservesUnrelatedEntries()
        {
            string packageId = "AppInstallerTest.TestPortableExe";
            string userLinksDir = TestCommon.GetPortableSymlinkDirectory(TestCommon.Scope.User);
            string expectedRawEntry = TestCommon.GetUnexpandedPath(userLinksDir, TestCommon.Scope.User).TrimEnd('\\') + ';';
            string sentinel1 = @"C:\TestSentinelUnrelated_A";
            string sentinel2 = @"C:\TestSentinelUnrelated_B";

            string originalRawPath = TestCommon.GetRawPathValue(TestCommon.Scope.User);
            var originalKind = TestCommon.GetPathRegisterValueKind(TestCommon.Scope.User);

            // Precondition: ensure user Links entry is not present initially
            if (originalRawPath.Contains(expectedRawEntry, StringComparison.OrdinalIgnoreCase))
            {
                TestCommon.RunAICLICommand("uninstall", $"{packageId} --force");
                originalRawPath = TestCommon.GetRawPathValue(TestCommon.Scope.User);
            }

            // Pre-seed user PATH with sentinels
            string seededPath = originalRawPath.TrimEnd(';') + $";{sentinel1};{sentinel2};";
            TestCommon.SetPathRegisterValue(seededPath, TestCommon.Scope.User, originalKind);

            try
            {
                // Install portable package
                var installResult = TestCommon.RunAICLICommand("install", packageId);
                Assert.That(installResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));

                string postInstallRaw = TestCommon.GetRawPathValue(TestCommon.Scope.User);
                Assert.That(postInstallRaw, Does.Contain(expectedRawEntry), "Raw PATH should contain Links entry after install.");
                Assert.That(postInstallRaw, Does.Contain(sentinel1), "Raw PATH should contain sentinel1 after install.");
                Assert.That(postInstallRaw, Does.Contain(sentinel2), "Raw PATH should contain sentinel2 after install.");

                // Uninstall package
                var uninstallResult = TestCommon.RunAICLICommand("uninstall", packageId);
                Assert.That(uninstallResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
                Assert.That(uninstallResult.StdOut, Does.Contain("Successfully uninstalled"));

                // Inspect both raw and expanded registry values
                string postUninstallRaw = TestCommon.GetRawPathValue(TestCommon.Scope.User);
                string postUninstallExpanded = TestCommon.GetExpandedPathValue(TestCommon.Scope.User);

                Assert.That(postUninstallRaw, Does.Not.Contain(expectedRawEntry), "Raw PATH should not contain Links entry after uninstall.");
                Assert.That(postUninstallRaw, Does.Contain(sentinel1), "Raw PATH should preserve sentinel1 after uninstall.");
                Assert.That(postUninstallRaw, Does.Contain(sentinel2), "Raw PATH should preserve sentinel2 after uninstall.");

                string expandedLinks = Environment.ExpandEnvironmentVariables(expectedRawEntry).TrimEnd('\\', ';') + ';';
                Assert.That(postUninstallExpanded, Does.Not.Contain(expandedLinks), "Expanded PATH should not contain Links entry after uninstall.");
                Assert.That(postUninstallExpanded, Does.Contain(sentinel1), "Expanded PATH should preserve sentinel1 after uninstall.");
                Assert.That(postUninstallExpanded, Does.Contain(sentinel2), "Expanded PATH should preserve sentinel2 after uninstall.");
            }
            finally
            {
                TestCommon.SetPathRegisterValue(originalRawPath, TestCommon.Scope.User, originalKind);
            }
        }

        /// <summary>
        /// Scenario 4: Shared Links directory and deduplication.
        /// Install two portable packages using same Links directory. Confirm only one %LOCALAPPDATA%\Microsoft\WinGet\Links; entry exists.
        /// Uninstall first package, verify entry remains; uninstall second package, verify entry is removed.
        /// </summary>
        [Test]
        public void SharedLinksDirectory_DeduplicatesAndCleansUp()
        {
            string package1 = "AppInstallerTest.TestPortableExe";
            string package2 = "AppInstallerTest.TestZipInstallerWithPortable";
            string userLinksDir = TestCommon.GetPortableSymlinkDirectory(TestCommon.Scope.User);
            string expectedRawEntry = TestCommon.GetUnexpandedPath(userLinksDir, TestCommon.Scope.User);

            string originalRawPath = TestCommon.GetRawPathValue(TestCommon.Scope.User);
            var originalKind = TestCommon.GetPathRegisterValueKind(TestCommon.Scope.User);

            try
            {
                // Install first package
                var install1 = TestCommon.RunAICLICommand("install", package1);
                Assert.That(install1.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));

                string rawPath1 = TestCommon.GetRawPathValue(TestCommon.Scope.User);
                int count1 = TestCommon.CountPathEntryOccurrences(rawPath1, expectedRawEntry);
                Assert.That(count1, Is.EqualTo(1), "Links entry should exist exactly once after first install.");

                // Install second package
                var install2 = TestCommon.RunAICLICommand("install", package2);
                Assert.That(install2.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));

                string rawPath2 = TestCommon.GetRawPathValue(TestCommon.Scope.User);
                int count2 = TestCommon.CountPathEntryOccurrences(rawPath2, expectedRawEntry);
                Assert.That(count2, Is.EqualTo(1), "Links entry should remain deduplicated (exactly one occurrence) after second install.");

                // Uninstall first package
                var uninstall1 = TestCommon.RunAICLICommand("uninstall", package1);
                Assert.That(uninstall1.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));

                string rawPath3 = TestCommon.GetRawPathValue(TestCommon.Scope.User);
                Assert.That(TestCommon.CountPathEntryOccurrences(rawPath3, expectedRawEntry), Is.EqualTo(1), "Links entry should remain while second package is still installed.");

                // Uninstall second package
                var uninstall2 = TestCommon.RunAICLICommand("uninstall", package2);
                Assert.That(uninstall2.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));

                string rawPath4 = TestCommon.GetRawPathValue(TestCommon.Scope.User);
                Assert.That(TestCommon.CountPathEntryOccurrences(rawPath4, expectedRawEntry), Is.EqualTo(0), "Links entry should be removed after second package is uninstalled.");
            }
            finally
            {
                TestCommon.RunAICLICommand("uninstall", $"{package1} --force");
                TestCommon.RunAICLICommand("uninstall", $"{package2} --force");
                TestCommon.SetPathRegisterValue(originalRawPath, TestCommon.Scope.User, originalKind);
            }
        }

        /// <summary>
        /// Scenario 5: Mixed user- and machine-scope installs.
        /// Install one user-scope and one machine-scope package.
        /// HKCU contains only user Links directory (%LOCALAPPDATA%\Microsoft\WinGet\Links).
        /// HKLM contains only machine Links directory (%ProgramFiles%\WinGet\Links).
        /// User Links not written to machine PATH, machine Links not written to user PATH.
        /// Uninstalling user removes only user Links; uninstalling machine removes machine Links.
        /// </summary>
        [Test]
        public void MixedUserAndMachineScopeInstalls_NoScopeLeakage()
        {
            if (!TestCommon.ExecutingAsAdministrator)
            {
                Assert.Ignore("Machine scope test requires administrator privilege.");
            }

            string userPackage = "AppInstallerTest.TestPortableExe";
            string machinePackage = "AppInstallerTest.TestPortableExeWithCommand";
            string userLinksDir = TestCommon.GetPortableSymlinkDirectory(TestCommon.Scope.User);
            string machineLinksDir = TestCommon.GetPortableSymlinkDirectory(TestCommon.Scope.Machine);
            string userRawLinks = TestCommon.GetUnexpandedPath(userLinksDir, TestCommon.Scope.User);
            string machineRawLinks = TestCommon.GetUnexpandedPath(machineLinksDir, TestCommon.Scope.Machine);

            string originalUserRawPath = TestCommon.GetRawPathValue(TestCommon.Scope.User);
            var originalUserKind = TestCommon.GetPathRegisterValueKind(TestCommon.Scope.User);
            string originalMachineRawPath = TestCommon.GetRawPathValue(TestCommon.Scope.Machine);
            var originalMachineKind = TestCommon.GetPathRegisterValueKind(TestCommon.Scope.Machine);

            try
            {
                // Install user package
                var userInstall = TestCommon.RunAICLICommand("install", $"{userPackage} --scope user");
                Assert.That(userInstall.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));

                // Install machine package
                var machineInstall = TestCommon.RunAICLICommand("install", $"{machinePackage} --scope machine");
                Assert.That(machineInstall.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));

                // Inspect HKCU (raw and expanded)
                string hkcuRaw = TestCommon.GetRawPathValue(TestCommon.Scope.User);
                string hkcuExpanded = TestCommon.GetExpandedPathValue(TestCommon.Scope.User);
                Assert.That(TestCommon.CountPathEntryOccurrences(hkcuRaw, userRawLinks), Is.EqualTo(1), "HKCU PATH should contain user Links entry.");
                Assert.That(TestCommon.CountPathEntryOccurrences(hkcuRaw, machineRawLinks), Is.EqualTo(0), "HKCU PATH should NOT contain machine Links entry.");
                Assert.That(TestCommon.CountPathEntryOccurrences(hkcuExpanded, machineLinksDir), Is.EqualTo(0), "HKCU expanded PATH should NOT contain machine Links directory.");

                // Inspect HKLM (raw and expanded)
                string hklmRaw = TestCommon.GetRawPathValue(TestCommon.Scope.Machine);
                string hklmExpanded = TestCommon.GetExpandedPathValue(TestCommon.Scope.Machine);
                Assert.That(TestCommon.CountPathEntryOccurrences(hklmRaw, machineRawLinks), Is.EqualTo(1), "HKLM PATH should contain machine Links entry.");
                Assert.That(TestCommon.CountPathEntryOccurrences(hklmRaw, userRawLinks), Is.EqualTo(0), "HKLM PATH should NOT contain user Links entry.");
                Assert.That(hklmRaw, Does.Not.Contain("%LOCALAPPDATA%"), "HKLM PATH should NOT contain %LOCALAPPDATA%.");
                Assert.That(TestCommon.CountPathEntryOccurrences(hklmExpanded, userLinksDir), Is.EqualTo(0), "HKLM expanded PATH should NOT contain user Links directory.");

                // Uninstall user package
                var userUninstall = TestCommon.RunAICLICommand("uninstall", $"{userPackage} --scope user");
                Assert.That(userUninstall.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));

                // Verify user entry removed and machine entry intact
                string hkcuRawAfterUserUninstall = TestCommon.GetRawPathValue(TestCommon.Scope.User);
                string hklmRawAfterUserUninstall = TestCommon.GetRawPathValue(TestCommon.Scope.Machine);
                Assert.That(TestCommon.CountPathEntryOccurrences(hkcuRawAfterUserUninstall, userRawLinks), Is.EqualTo(0), "User Links entry should be removed from HKCU.");
                Assert.That(TestCommon.CountPathEntryOccurrences(hklmRawAfterUserUninstall, machineRawLinks), Is.EqualTo(1), "Machine Links entry should remain intact in HKLM.");

                // Uninstall machine package
                var machineUninstall = TestCommon.RunAICLICommand("uninstall", $"{machinePackage} --scope machine");
                Assert.That(machineUninstall.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));

                // Verify machine entry removed
                string hklmRawAfterMachineUninstall = TestCommon.GetRawPathValue(TestCommon.Scope.Machine);
                Assert.That(TestCommon.CountPathEntryOccurrences(hklmRawAfterMachineUninstall, machineRawLinks), Is.EqualTo(0), "Machine Links entry should be removed from HKLM.");
            }
            finally
            {
                TestCommon.RunAICLICommand("uninstall", $"{userPackage} --scope user --force");
                TestCommon.RunAICLICommand("uninstall", $"{machinePackage} --scope machine --force");
                TestCommon.SetPathRegisterValue(originalUserRawPath, TestCommon.Scope.User, originalUserKind);
                TestCommon.SetPathRegisterValue(originalMachineRawPath, TestCommon.Scope.Machine, originalMachineKind);
            }
        }

        /// <summary>
        /// Scenario 6: Legacy expanded-entry compatibility.
        /// Seed user PATH with old fully expanded Links path, then run install/repair and uninstall.
        /// Verify WinGet recognizes existing entry, adds no duplicate variable entry, and removes legacy entry on uninstall.
        /// </summary>
        [Test]
        public void LegacyExpandedEntry_CompatibilityAndCleanup()
        {
            string packageId = "AppInstallerTest.TestPortableExe";
            string userLinksDir = TestCommon.GetPortableSymlinkDirectory(TestCommon.Scope.User);
            string legacyExpandedLinks = userLinksDir.TrimEnd('\\');
            string userRawLinks = TestCommon.GetUnexpandedPath(userLinksDir, TestCommon.Scope.User);
            string sentinelBefore = @"C:\TestSentinel_Before";
            string sentinelAfter = @"C:\TestSentinel_After";

            string originalRawPath = TestCommon.GetRawPathValue(TestCommon.Scope.User);
            var originalKind = TestCommon.GetPathRegisterValueKind(TestCommon.Scope.User);

            // Pre-seed user PATH with sentinelBefore;legacyExpandedLinks;sentinelAfter;
            string seededPath = originalRawPath.TrimEnd(';') + $";{sentinelBefore};{legacyExpandedLinks};{sentinelAfter};";
            TestCommon.SetPathRegisterValue(seededPath, TestCommon.Scope.User, originalKind);

            try
            {
                // Install portable package - WinGet should recognize that the Links directory is already present
                var installResult = TestCommon.RunAICLICommand("install", packageId);
                Assert.That(installResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));

                // Verify that no duplicate variable-form entry was added because legacy entry was recognized
                string postInstallRaw = TestCommon.GetRawPathValue(TestCommon.Scope.User);
                Assert.That(TestCommon.CountPathEntryOccurrences(postInstallRaw, userRawLinks), Is.EqualTo(0), "WinGet should recognize existing legacy entry and not add a duplicate variable-form entry.");
                Assert.That(TestCommon.CountPathEntryOccurrences(postInstallRaw, legacyExpandedLinks), Is.EqualTo(1), "Legacy entry should remain present after install.");
                Assert.That(postInstallRaw, Does.Contain(sentinelBefore), "Sentinel before should remain intact.");
                Assert.That(postInstallRaw, Does.Contain(sentinelAfter), "Sentinel after should remain intact.");

                // Note: Portable packages do not support the 'repair' command (returns APPINSTALLER_CLI_ERROR_REPAIR_NOT_SUPPORTED).
                // Reinstall/repair for portable packages is performed via 'install --force'.
                // Production code behavior: PathVariable::Append uses ContainsInternal which compares expanded paths,
                // so it recognizes that the legacy expanded entry already covers the Links directory and skips adding a duplicate variable entry.
                var reinstallResult = TestCommon.RunAICLICommand("install", $"{packageId} --force");
                Assert.That(reinstallResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));

                string postReinstallRaw = TestCommon.GetRawPathValue(TestCommon.Scope.User);
                Assert.That(TestCommon.CountPathEntryOccurrences(postReinstallRaw, userRawLinks), Is.EqualTo(0), "Reinstall should recognize existing legacy entry and not add a duplicate variable-form entry.");
                Assert.That(TestCommon.CountPathEntryOccurrences(postReinstallRaw, legacyExpandedLinks), Is.EqualTo(1), "Legacy entry should remain present after reinstall.");

                // Uninstall package: PathVariable::Remove normalizes and removes the legacy expanded entry while preserving adjacent sentinels.
                var uninstallResult = TestCommon.RunAICLICommand("uninstall", packageId);
                Assert.That(uninstallResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));

                // WinGet should remove the legacy expanded entry while preserving adjacent sentinels
                string postUninstallRaw = TestCommon.GetRawPathValue(TestCommon.Scope.User);
                Assert.That(TestCommon.CountPathEntryOccurrences(postUninstallRaw, legacyExpandedLinks), Is.EqualTo(0), "Legacy expanded Links entry should be removed after uninstall.");
                Assert.That(postUninstallRaw, Does.Contain(sentinelBefore), "Sentinel before should remain intact after uninstall.");
                Assert.That(postUninstallRaw, Does.Contain(sentinelAfter), "Sentinel after should remain intact after uninstall.");
            }
            finally
            {
                TestCommon.RunAICLICommand("uninstall", $"{packageId} --force");
                TestCommon.SetPathRegisterValue(originalRawPath, TestCommon.Scope.User, originalKind);
            }
        }

        /// <summary>
        /// Scenario 7: PATH refresh expansion and ordering.
        /// Exercise the install path that refreshes current process PATH with values containing environment-variable references.
        /// Verify process environment contains expanded absolute paths rather than %...% references,
        /// with machine PATH entries preceding user PATH entries.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Test]
        public async Task PathVariableRefresh_ExpansionAndOrdering()
        {
            // Snapshot original process PATH and user registry PATH
            string originalProcessPath = Environment.GetEnvironmentVariable("Path");
            string originalUserPath = TestCommon.GetRawPathValue(TestCommon.Scope.User);
            var originalUserKind = TestCommon.GetPathRegisterValueKind(TestCommon.Scope.User);

            string userLinksDir = TestCommon.GetPortableSymlinkDirectory(TestCommon.Scope.User);
            string userRawLinks = TestCommon.GetUnexpandedPath(userLinksDir, TestCommon.Scope.User).TrimEnd('\\') + ';';

            // Pre-seed user PATH with variable-form links entry if not present
            if (!originalUserPath.Contains(userRawLinks, StringComparison.OrdinalIgnoreCase))
            {
                TestCommon.SetPathRegisterValue(originalUserPath.TrimEnd(';') + $";{userRawLinks}", TestCommon.Scope.User, originalUserKind);
            }

            try
            {
                // Use in-process PackageManager so RefreshPathVariableForCurrentProcess runs within this process
                var testFactory = new WinGetProjectionFactory(new ActivationFactoryInstanceInitializer());
                var packageManager = testFactory.CreatePackageManager();
                var testSource = packageManager.GetPackageCatalogByName(Constants.TestSourceName);

                // Find package that requires PATH refresh across dependency installation
                var filter = testFactory.CreatePackageMatchFilter();
                filter.Field = PackageMatchField.Id;
                filter.Option = PackageFieldMatchOption.Equals;
                filter.Value = "AppInstallerTest.PackageDependencyRequiresPathRefresh";

                var findPackageOptions = testFactory.CreateFindPackagesOptions();
                findPackageOptions.Filters.Add(filter);

                var source = testSource.Connect().PackageCatalog;
                var catalogPackage = source.FindPackages(findPackageOptions).Matches.FirstOrDefault()?.CatalogPackage;

                Assert.That(catalogPackage, Is.Not.Null, "Package AppInstallerTest.PackageDependencyRequiresPathRefresh should be found in test source.");

                var installOptions = testFactory.CreateInstallOptions();
                installOptions.PackageInstallMode = PackageInstallMode.Silent;

                var installResult = await packageManager.InstallPackageAsync(catalogPackage, installOptions);
                Assert.That(installResult.Status, Is.EqualTo(InstallResultStatus.Ok), "Installation with PATH refresh dependency should succeed.");

                // Verify the refreshed current process PATH
                string processPath = Environment.GetEnvironmentVariable("Path");
                Assert.That(processPath, Is.Not.Null.And.Not.Empty, "Process PATH should not be empty after refresh.");
                Assert.That(processPath, Does.Contain(userLinksDir.TrimEnd('\\')), "Refreshed process PATH should contain the expanded user Links directory.");

                // Split process PATH into individual entries
                string[] entries = processPath.Split(';', StringSplitOptions.RemoveEmptyEntries);

                // Verify that no entry contains unexpanded %...% references
                foreach (string entry in entries)
                {
                    Assert.That(entry, Does.Not.Contain("%"), $"Process PATH entry '{entry}' must be fully expanded and not contain '%' references.");
                }

                // Verify ordering: machine PATH entries must precede user PATH entries
                string machinePathRaw = TestCommon.GetRawPathValue(TestCommon.Scope.Machine);
                string firstMachineEntry = machinePathRaw.Split(';', StringSplitOptions.RemoveEmptyEntries)
                    .Select(e => Environment.ExpandEnvironmentVariables(e).TrimEnd('\\'))
                    .FirstOrDefault(e => entries.Contains(e, StringComparer.OrdinalIgnoreCase));

                Assert.That(firstMachineEntry, Is.Not.Null, "Expected to find at least one machine PATH entry in refreshed process PATH.");

                int machineIndex = Array.FindIndex(entries, e => string.Equals(e, firstMachineEntry, StringComparison.OrdinalIgnoreCase));
                int userIndex = Array.FindIndex(entries, e => string.Equals(e, userLinksDir.TrimEnd('\\'), StringComparison.OrdinalIgnoreCase));

                Assert.That(machineIndex, Is.GreaterThanOrEqualTo(0), "Machine PATH entry should exist in process PATH.");
                Assert.That(userIndex, Is.GreaterThanOrEqualTo(0), "User Links entry should exist in process PATH.");
                Assert.That(machineIndex, Is.LessThan(userIndex), $"Machine PATH entry '{firstMachineEntry}' (index {machineIndex}) must precede user Links entry (index {userIndex}).");
            }
            finally
            {
                TestCommon.RunAICLICommand("uninstall", "AppInstallerTest.PackageDependencyRequiresPathRefresh --force");
                TestCommon.RunAICLICommand("uninstall", "AppInstallerTest.TestPortableExeWithCommand --force");
                TestCommon.SetPathRegisterValue(originalUserPath, TestCommon.Scope.User, originalUserKind);
                Environment.SetEnvironmentVariable("Path", originalProcessPath);
            }
        }

        /// <summary>
        /// Scenario 8: Archive binaries that depend on PATH.
        /// Install AppInstallerTest.ArchivePortableWithBinariesDependentOnPath:
        /// - Verify install directory added to PATH rather than shared Links directory.
        /// - Inspect raw registry value uses environment-variable form when under known folder.
        /// - Verify archive's dependent binaries can run through installed package via refreshed PATH.
        /// - Uninstall: verify only install-directory PATH entry removed and unrelated PATH entries intact.
        /// - Covers both user-scope and machine-scope.
        /// </summary>
        [Test]
        public void ArchiveBinariesDependentOnPath_UserScope()
        {
            string packageId = "AppInstallerTest.ArchivePortableWithBinariesDependentOnPath";
            string packageDirName = $"{packageId}_{Constants.TestSourceIdentifier}";
            string installDir = Path.Combine(TestCommon.GetPortablePackagesDirectory(), packageDirName);
            string expectedRawEntry = TestCommon.GetUnexpandedPath(installDir, TestCommon.Scope.User).TrimEnd('\\') + ';';
            string userLinksDir = TestCommon.GetPortableSymlinkDirectory(TestCommon.Scope.User);
            string expectedLinksRaw = TestCommon.GetUnexpandedPath(userLinksDir, TestCommon.Scope.User);
            string sentinel = @"C:\TestSentinelUnrelated_Archive";

            string originalRawPath = TestCommon.GetRawPathValue(TestCommon.Scope.User);
            var originalKind = TestCommon.GetPathRegisterValueKind(TestCommon.Scope.User);

            string seededPath = originalRawPath.TrimEnd(';') + $";{sentinel};";
            TestCommon.SetPathRegisterValue(seededPath, TestCommon.Scope.User, originalKind);

            try
            {
                var installResult = TestCommon.RunAICLICommand("install", packageId);
                Assert.That(installResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
                Assert.That(installResult.StdOut, Does.Contain("Successfully installed"));

                // Inspect raw registry value
                string postInstallRaw = TestCommon.GetRawPathValue(TestCommon.Scope.User);
                Assert.That(postInstallRaw, Does.Contain(expectedRawEntry), $"Raw PATH should contain the package install directory {expectedRawEntry}.");
                Assert.That(postInstallRaw, Does.Contain(sentinel), "Raw PATH should preserve sentinel.");

                // Verify package install dir is added, not shared Links dir (dedup/isolated)
                int linksCountBefore = TestCommon.CountPathEntryOccurrences(originalRawPath, expectedLinksRaw);
                int linksCountAfter = TestCommon.CountPathEntryOccurrences(postInstallRaw, expectedLinksRaw);
                Assert.That(linksCountAfter, Is.EqualTo(linksCountBefore), "Shared Links directory should not be added when archive binaries depend on PATH.");

                // The package AppInstallerTest.ArchivePortableWithBinariesDependentOnPath declares NestedInstallerFiles
                // with PortableCommandAlias: TestPortable and entrypoint AppInstallerTestExeInstaller.exe.
                // Because its archive binaries depend on each other via PATH, WinGet adds the package installation directory
                // directly to PATH rather than creating symlinks in the shared Links directory.
                // Executing TestPortable.exe via the refreshed PATH verifies PATH resolution to the package directory.
                string refreshedPath = TestCommon.GetExpandedPathValue(TestCommon.Scope.Machine).TrimEnd(';') + ";" +
                                       TestCommon.GetExpandedPathValue(TestCommon.Scope.User);
                ProcessStartInfo startInfo = new ProcessStartInfo("cmd.exe", $"/c {Constants.TestPortableExe} /NoOperation")
                {
                    UseShellExecute = false,
                    CreateNoWindow = true,
                };
                startInfo.Environment["PATH"] = refreshedPath;
                using Process process = Process.Start(startInfo);
                Assert.That(process, Is.Not.Null, "Process should start successfully.");
                process.WaitForExit();
                Assert.That(process.ExitCode, Is.EqualTo(0), "Archive dependent binary should run successfully via refreshed PATH.");

                // Uninstall
                var uninstallResult = TestCommon.RunAICLICommand("uninstall", packageId);
                Assert.That(uninstallResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
                Assert.That(uninstallResult.StdOut, Does.Contain("Successfully uninstalled"));

                // Verify cleanup in both raw and expanded registry values
                string postUninstallRaw = TestCommon.GetRawPathValue(TestCommon.Scope.User);
                string postUninstallExpanded = TestCommon.GetExpandedPathValue(TestCommon.Scope.User);
                Assert.That(postUninstallRaw, Does.Not.Contain(expectedRawEntry), "Install directory PATH entry should be removed after uninstall.");
                Assert.That(postUninstallExpanded, Does.Not.Contain(installDir.TrimEnd('\\') + ';'), "Expanded install directory PATH entry should be removed after uninstall.");
                Assert.That(postUninstallRaw, Does.Contain(sentinel), "Sentinel entry should remain intact after uninstall.");
            }
            finally
            {
                TestCommon.RunAICLICommand("uninstall", $"{packageId} --force");
                TestCommon.SetPathRegisterValue(originalRawPath, TestCommon.Scope.User, originalKind);
            }
        }

        /// <summary>
        /// Scenario 8 (Machine-scope variant): Archive binaries that depend on PATH.
        /// Verify machine-scope install adds package directory to HKLM using derived system variable,
        /// does not store user-scoped variables, runs binaries via refreshed PATH, and cleanly uninstalls.
        /// </summary>
        [Test]
        public void ArchiveBinariesDependentOnPath_MachineScope()
        {
            if (!TestCommon.ExecutingAsAdministrator)
            {
                Assert.Ignore("Machine scope test requires administrator privilege.");
            }

            string packageId = "AppInstallerTest.ArchivePortableWithBinariesDependentOnPath";
            string packageDirName = $"{packageId}_{Constants.TestSourceIdentifier}";
            string machinePackagesDir = Path.Combine(Environment.GetEnvironmentVariable("ProgramFiles") ?? @"C:\Program Files", "WinGet", "Packages");
            string installDir = Path.Combine(machinePackagesDir, packageDirName);
            string expectedRawEntry = TestCommon.GetUnexpandedPath(installDir, TestCommon.Scope.Machine).TrimEnd('\\') + ';';
            string sentinel = @"C:\TestSentinelUnrelated_ArchiveMachine";

            string originalRawPath = TestCommon.GetRawPathValue(TestCommon.Scope.Machine);
            var originalKind = TestCommon.GetPathRegisterValueKind(TestCommon.Scope.Machine);

            string seededPath = originalRawPath.TrimEnd(';') + $";{sentinel};";
            TestCommon.SetPathRegisterValue(seededPath, TestCommon.Scope.Machine, originalKind);

            try
            {
                var installResult = TestCommon.RunAICLICommand("install", $"{packageId} --scope machine");
                Assert.That(installResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
                Assert.That(installResult.StdOut, Does.Contain("Successfully installed"));

                // Inspect raw machine registry value
                string postInstallRaw = TestCommon.GetRawPathValue(TestCommon.Scope.Machine);
                Assert.That(postInstallRaw, Does.Contain(expectedRawEntry), $"Machine PATH should contain package install directory {expectedRawEntry}.");
                Assert.That(postInstallRaw, Does.Not.Contain("%LOCALAPPDATA%"), "Machine PATH must not contain %LOCALAPPDATA%");
                Assert.That(postInstallRaw, Does.Not.Contain("%APPDATA%"), "Machine PATH must not contain %APPDATA%");
                Assert.That(postInstallRaw, Does.Not.Contain("%USERPROFILE%"), "Machine PATH must not contain %USERPROFILE%");
                Assert.That(postInstallRaw, Does.Contain(sentinel), "Machine PATH should preserve sentinel.");

                // The package AppInstallerTest.ArchivePortableWithBinariesDependentOnPath declares NestedInstallerFiles
                // with PortableCommandAlias: TestPortable and entrypoint AppInstallerTestExeInstaller.exe.
                // Because its archive binaries depend on each other via PATH, WinGet adds the package installation directory
                // directly to PATH rather than creating symlinks in the shared Links directory.
                // Executing TestPortable.exe via the refreshed PATH verifies PATH resolution to the package directory in machine scope.
                string refreshedPath = TestCommon.GetExpandedPathValue(TestCommon.Scope.Machine).TrimEnd(';') + ";" +
                                       TestCommon.GetExpandedPathValue(TestCommon.Scope.User);
                ProcessStartInfo startInfo = new ProcessStartInfo("cmd.exe", $"/c {Constants.TestPortableExe} /NoOperation")
                {
                    UseShellExecute = false,
                    CreateNoWindow = true,
                };
                startInfo.Environment["PATH"] = refreshedPath;
                using Process process = Process.Start(startInfo);
                Assert.That(process, Is.Not.Null, "Process should start successfully.");
                process.WaitForExit();
                Assert.That(process.ExitCode, Is.EqualTo(0), "Archive dependent binary should run successfully in machine scope via refreshed PATH.");

                // Uninstall
                var uninstallResult = TestCommon.RunAICLICommand("uninstall", $"{packageId} --scope machine");
                Assert.That(uninstallResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
                Assert.That(uninstallResult.StdOut, Does.Contain("Successfully uninstalled"));

                // Verify cleanup
                string postUninstallRaw = TestCommon.GetRawPathValue(TestCommon.Scope.Machine);
                string postUninstallExpanded = TestCommon.GetExpandedPathValue(TestCommon.Scope.Machine);
                Assert.That(postUninstallRaw, Does.Not.Contain(expectedRawEntry), "Install directory PATH entry should be removed from machine PATH.");
                Assert.That(postUninstallExpanded, Does.Not.Contain(installDir.TrimEnd('\\') + ';'), "Expanded install directory PATH entry should be removed from machine PATH.");
                Assert.That(postUninstallRaw, Does.Contain(sentinel), "Sentinel entry should remain intact in machine PATH.");
            }
            finally
            {
                TestCommon.RunAICLICommand("uninstall", $"{packageId} --scope machine --force");
                TestCommon.SetPathRegisterValue(originalRawPath, TestCommon.Scope.Machine, originalKind);
            }
        }
    }
}
