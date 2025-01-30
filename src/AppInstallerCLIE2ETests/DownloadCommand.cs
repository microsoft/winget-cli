// -----------------------------------------------------------------------------
// <copyright file="DownloadCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests
{
    using System.IO;
    using AppInstallerCLIE2ETests.Helpers;
    using Microsoft.Management.Deployment;
    using NUnit.Framework;
    using Windows.System;

    /// <summary>
    /// Test download command.
    /// </summary>
    public class DownloadCommand : BaseCommand
    {
        /// <summary>
        /// Downloads the test installer and its package dependencies.
        /// </summary>
        [Test]
        public void DownloadDependencies()
        {
            var downloadDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("download", $"AppInstallerTest.PackageDependency --download-directory {downloadDir}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            var dependenciesDir = Path.Combine(downloadDir, Constants.Dependencies);
            TestCommon.AssertInstallerDownload(dependenciesDir, "TestPortableExe", "3.0.0.0", ProcessorArchitecture.X64, TestCommon.Scope.Unknown, PackageInstallerType.Portable, "en-US");
            TestCommon.AssertInstallerDownload(downloadDir, "TestPackageDependency", "1.0.0.0", ProcessorArchitecture.X64, TestCommon.Scope.Unknown, PackageInstallerType.Exe, "en-US");
        }

        /// <summary>
        /// Downloads the test installer and skips dependencies.
        /// </summary>
        [Test]
        public void DownloadDependencies_Skip()
        {
            var downloadDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("download", $"AppInstallerTest.PackageDependency --skip-dependencies --download-directory {downloadDir}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Dependencies skipped."));
            Assert.IsFalse(Directory.Exists(Path.Combine(downloadDir, Constants.Dependencies)));
            TestCommon.AssertInstallerDownload(downloadDir, "TestPackageDependency", "1.0.0.0", ProcessorArchitecture.X64, TestCommon.Scope.Unknown, PackageInstallerType.Exe, "en-US");
        }

        /// <summary>
        /// Downloads the test installer to the default downloads directory.
        /// </summary>
        [Test]
        public void DownloadToDefaultDirectory()
        {
            var packageVersion = "2.0.0.0";
            var result = TestCommon.RunAICLICommand("download", $"{Constants.ExeInstallerPackageId} --version {packageVersion}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            string downloadDir = Path.Combine(TestCommon.GetDefaultDownloadDirectory(), $"{Constants.ExeInstallerPackageId}_{packageVersion}");
            TestCommon.AssertInstallerDownload(downloadDir, "TestExeInstaller", packageVersion, ProcessorArchitecture.X86, TestCommon.Scope.Unknown, PackageInstallerType.Exe);
        }

        /// <summary>
        /// Downloads the test installer to a specified directory.
        /// </summary>
        [Test]
        public void DownloadToDirectory()
        {
            var downloadDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("download", $"{Constants.ExeInstallerPackageId} --download-directory {downloadDir}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            TestCommon.AssertInstallerDownload(downloadDir, "TestExeInstaller", "2.0.0.0", ProcessorArchitecture.X86, TestCommon.Scope.Unknown, PackageInstallerType.Exe);
        }

        /// <summary>
        /// Downloads the test installer with Arm64.
        /// </summary>
        [Test]
        public void DownloadWithArm64()
        {
            var downloadDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("download", $"AppInstallerTest.TestMultipleInstallers --scope user --download-directory {downloadDir} --architecture Arm64");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
#pragma warning disable CA1416 // Validate platform compatibility. Arm64 is not reachable.
            TestCommon.AssertInstallerDownload(downloadDir, "TestMultipleInstallers", "1.0.0.0", ProcessorArchitecture.Arm64, TestCommon.Scope.User, PackageInstallerType.Nullsoft, "en-US");
#pragma warning restore CA1416
        }

        /// <summary>
        /// Downloads the test installer using the user scope argument.
        /// </summary>
        [Test]
        public void DownloadWithUserScope()
        {
            var downloadDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("download", $"AppInstallerTest.TestMultipleInstallers --scope user --download-directory {downloadDir}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            TestCommon.AssertInstallerDownload(downloadDir, "TestMultipleInstallers", "1.0.0.0", ProcessorArchitecture.X64, TestCommon.Scope.User, PackageInstallerType.Nullsoft, "en-US");
        }

        /// <summary>
        /// Downloads the test installer using the machine scope argument.
        /// </summary>
        [Test]
        public void DownloadWithMachineScope()
        {
            var downloadDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("download", $"AppInstallerTest.TestMultipleInstallers --scope machine --download-directory {downloadDir}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            TestCommon.AssertInstallerDownload(downloadDir, "TestMultipleInstallers", "1.0.0.0", ProcessorArchitecture.X86, TestCommon.Scope.Machine, PackageInstallerType.Msi, "en-US");
        }

        /// <summary>
        /// Downloads the test installer using the 'zip' installer type argument. Verifies that base installer types such as 'zip' are still supported.
        /// </summary>
        [Test]
        public void DownloadWithZipInstallerTypeArg()
        {
            var downloadDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("download", $"AppInstallerTest.TestMultipleInstallers --installer-type zip --download-directory {downloadDir}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            TestCommon.AssertInstallerDownload(downloadDir, "TestMultipleInstallers", "1.0.0.0", ProcessorArchitecture.X64, TestCommon.Scope.Unknown, PackageInstallerType.Exe, "zh-CN", true);
        }

        /// <summary>
        /// Downloads the test installer using the installer type argument.
        /// </summary>
        [Test]
        public void DownloadWithInstallerTypeArg()
        {
            var downloadDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("download", $"AppInstallerTest.TestMultipleInstallers --installer-type msi --download-directory {downloadDir}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            TestCommon.AssertInstallerDownload(downloadDir, "TestMultipleInstallers", "1.0.0.0", ProcessorArchitecture.X86, TestCommon.Scope.Machine, PackageInstallerType.Msi, "en-US");
        }

        /// <summary>
        /// Downloads the test installer using the architecture argument.
        /// </summary>
        [Test]
        public void DownloadWithArchitectureArg()
        {
            var downloadDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("download", $"AppInstallerTest.TestMultipleInstallers --architecture x86 --download-directory {downloadDir}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            TestCommon.AssertInstallerDownload(downloadDir, "TestMultipleInstallers", "1.0.0.0", ProcessorArchitecture.X86, TestCommon.Scope.Machine, PackageInstallerType.Msi, "en-US");
        }

        /// <summary>
        /// Downloads the test installer using the locale argument.
        /// </summary>
        [Test]
        public void DownloadWithLocaleArg()
        {
            var downloadDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("download", $"AppInstallerTest.TestMultipleInstallers --locale zh-CN --download-directory {downloadDir}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            TestCommon.AssertInstallerDownload(downloadDir, "TestMultipleInstallers", "1.0.0.0", ProcessorArchitecture.X64, TestCommon.Scope.Unknown, PackageInstallerType.Exe, "zh-CN", true);
        }

        /// <summary>
        /// Downloads the test installer with a hash mismatch.
        /// </summary>
        [Test]
        public void DownloadWithHashMismatch()
        {
            var downloadDir = TestCommon.GetRandomTestDir();
            var errorResult = TestCommon.RunAICLICommand("download", $"AppInstallerTest.TestExeSha256Mismatch --download-directory {downloadDir}");
            Assert.AreEqual(Constants.ErrorCode.ERROR_INSTALLER_HASH_MISMATCH, errorResult.ExitCode);
        }

        /// <summary>
        /// Downloads the zero byte test installer with a hash mismatch.
        /// </summary>
        [Test]
        public void DownloadZeroByteFileWithHashMismatch()
        {
            var downloadDir = TestCommon.GetRandomTestDir();
            var errorResult = TestCommon.RunAICLICommand("download", $"ZeroByteFile.IncorrectHash --download-directory {downloadDir}");
            Assert.AreEqual(Constants.ErrorCode.ERROR_INSTALLER_ZERO_BYTE_FILE, errorResult.ExitCode);
        }

        /// <summary>
        /// Downloads the zero byte test installer.
        /// </summary>
        [Test]
        public void DownloadZeroByteFile()
        {
            var downloadDir = TestCommon.GetRandomTestDir();
            var result = TestCommon.RunAICLICommand("download", $"ZeroByteFile.CorrectHash --download-directory {downloadDir}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            TestCommon.AssertInstallerDownload(downloadDir, "ZeroByteFile-CorrectHash", "1.0.0.0", ProcessorArchitecture.X86, TestCommon.Scope.Unknown, PackageInstallerType.Exe, "en-US");
        }
    }
}
