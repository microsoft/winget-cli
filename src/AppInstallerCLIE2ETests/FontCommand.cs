// -----------------------------------------------------------------------------
// <copyright file="FontCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests
{
    using System.IO;
    using AppInstallerCLIE2ETests.Helpers;
    using Microsoft.Win32;
    using NUnit.Framework;

    /// <summary>
    /// Test font command.
    /// </summary>
    public class FontCommand : BaseCommand
    {
        /// <summary>
        /// One time set up.
        /// </summary>
        [OneTimeSetUp]
        public void OneTimeSetup()
        {
            WinGetSettingsHelper.ConfigureFeature("fonts", true);
        }

        /// <summary>
        /// Test install a font with user scope.
        /// </summary>
        [Test]
        public void InstallFont_UserScope()
        {
            var fontPackageName = "AppInstallerTest.TestFont";
            var fontPackageVersion = "1.0.0.0";
            var installResult = TestCommon.RunAICLICommand("install", $"{fontPackageName} --scope User");
            Assert.That(installResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            try
            {
                Assert.That(installResult.StdOut, Does.Contain("Successfully installed"));
                TestCommon.VerifyFontPackage(fontPackageName, fontPackageVersion, new[] { Constants.FontFileName }, TestCommon.Scope.User);
            }
            finally
            {
                var uninstallResult = TestCommon.RunAICLICommand("uninstall", $"{fontPackageName} --scope User");
                Assert.That(uninstallResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
                TestCommon.VerifyFontPackage(fontPackageName, fontPackageVersion, new[] { Constants.FontFileName }, TestCommon.Scope.User, false);
            }
        }

        /// <summary>
        /// Test install a font with machine scope.
        /// </summary>
        [Test]
        public void InstallFont_MachineScope()
        {
            var fontPackageName = "AppInstallerTest.TestFont";
            var fontPackageVersion = "1.0.0.0";
            var result = TestCommon.RunAICLICommand("install", $"{fontPackageName} --scope Machine");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("Successfully installed"));
            TestCommon.VerifyFontPackage(fontPackageName, fontPackageVersion, new[] { Constants.FontFileName }, TestCommon.Scope.Machine);

            var uninstallResult = TestCommon.RunAICLICommand("uninstall", fontPackageName);
            Assert.That(uninstallResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            TestCommon.VerifyFontPackage(fontPackageName, fontPackageVersion, new[] { Constants.FontFileName }, TestCommon.Scope.Machine, false);
        }

        /// <summary>
        /// Test repair of a user font's discovery registration.
        /// </summary>
        [Test]
        public void RepairFont_UserScope()
        {
            var fontPackageName = "AppInstallerTest.TestFont";
            var fontPackageVersion = "1.0.0.0";
            var installResult = TestCommon.RunAICLICommand("install", $"{fontPackageName} --scope User");
            Assert.That(installResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            try
            {
                using (var fontsKey = Registry.CurrentUser.OpenSubKey(Constants.FontsSubKey, true))
                using (var packageKey = fontsKey.OpenSubKey(
                    $"Microsoft.DesktopAppInstaller_8wekyb3d8bbwe\\{fontPackageName}\\{fontPackageVersion}"))
                {
                    Assert.That(packageKey, Is.Not.Null);
                    var names = packageKey.GetValueNames();
                    Assert.That(names, Is.Not.Empty);
                    foreach (var name in names)
                    {
                        var installedPath = packageKey.GetValue(name).ToString();
                        var registrationName = Path.GetFileName(installedPath);
                        Assert.That(fontsKey.GetValue(registrationName), Is.EqualTo(installedPath));
                        fontsKey.DeleteValue(registrationName);
                    }
                }

                var repairResult = TestCommon.RunAICLICommand("repair", $"{fontPackageName} --scope User");
                Assert.That(repairResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
                Assert.That(repairResult.StdOut, Does.Contain("Repair operation completed successfully."));
                TestCommon.VerifyFontPackage(fontPackageName, fontPackageVersion, new[] { Constants.FontFileName }, TestCommon.Scope.User);
            }
            finally
            {
                var uninstallResult = TestCommon.RunAICLICommand("uninstall", $"{fontPackageName} --scope User");
                Assert.That(uninstallResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
                TestCommon.VerifyFontPackage(fontPackageName, fontPackageVersion, new[] { Constants.FontFileName }, TestCommon.Scope.User, false);
            }
        }

        /// <summary>
        /// Test install an invalid font file.
        /// </summary>
        [Test]
        public void InstallInvalidFont()
        {
            var result = TestCommon.RunAICLICommand("install", "AppInstallerTest.TestInvalidFont");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.ERROR_FONT_FILE_NOT_SUPPORTED));
            Assert.That(result.StdOut, Does.Contain("One or more fonts in the font package is not supported and cannot be installed."));
        }
    }
}
