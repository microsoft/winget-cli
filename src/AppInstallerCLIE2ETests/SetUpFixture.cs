// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
{
    using Microsoft.Win32;
    using NUnit.Framework;
    using System;
    using System.IO;
    using System.Diagnostics;

    [SetUpFixture]
    public class SetUpFixture
    {
        private static bool ShouldDisableDevModeOnExit = true;
        private const string TestDataName = "TestData";
        private const string ManifestsName = "Manifests";
        private const string PackageName = "Package";
        private const string IndexName = @"index.db";
        private const string PublicName = "Public";
        private const string IndexCreationToolName = "IndexCreationTool";
        private const string WinGetUtilName = "WinGetUtil";

        private const string ExeInstallerName = @"AppInstallerTestExeInstaller";
        private const string MsiInstallerName = @"AppInstallerTestMsiInstaller";
        private const string MsixInstallerName = @"AppInstallerTestMsixInstaller";

        private const string IndexPackageName = @"source.msix";

        [OneTimeSetUp]
        public void Setup()
        {
            // Read TestParameters and set runtime variables
            TestCommon.PackagedContext = TestContext.Parameters.Exists(Constants.PackagedContextParameter) &&
                TestContext.Parameters.Get(Constants.PackagedContextParameter).Equals("true", StringComparison.OrdinalIgnoreCase);

            TestCommon.VerboseLogging = TestContext.Parameters.Exists(Constants.VerboseLoggingParameter) &&
                TestContext.Parameters.Get(Constants.VerboseLoggingParameter).Equals("true", StringComparison.OrdinalIgnoreCase);

            TestCommon.LooseFileRegistration = TestContext.Parameters.Exists(Constants.LooseFileRegistrationParameter) &&
                    TestContext.Parameters.Get(Constants.LooseFileRegistrationParameter).Equals("true", StringComparison.OrdinalIgnoreCase);

            TestCommon.InvokeCommandInDesktopPackage = TestContext.Parameters.Exists(Constants.InvokeCommandInDesktopPackageParameter) &&
                TestContext.Parameters.Get(Constants.InvokeCommandInDesktopPackageParameter).Equals("true", StringComparison.OrdinalIgnoreCase);

            if (TestContext.Parameters.Exists(Constants.AICLIPathParameter))
            {
                TestCommon.AICLIPath = TestContext.Parameters.Get(Constants.AICLIPathParameter);
            }
            else
            {
                if (TestCommon.PackagedContext)
                {
                    // For packaged context, default to AppExecutionAlias
                    TestCommon.AICLIPath = "WinGetDev.exe";
                }
                else
                {
                    TestCommon.AICLIPath = TestCommon.GetTestFile("AppInstallerCli.exe");
                }
            }

            if (TestContext.Parameters.Exists(Constants.AICLIPackagePathParameter))
            {
                TestCommon.AICLIPackagePath = TestContext.Parameters.Get(Constants.AICLIPackagePathParameter);
            }
            else
            {
                TestCommon.AICLIPackagePath = TestCommon.GetTestFile("AppInstallerCLIPackage.appxbundle");
            }

            if (TestCommon.LooseFileRegistration && TestCommon.InvokeCommandInDesktopPackage)
            {
                TestCommon.AICLIPath = Path.Combine(TestCommon.AICLIPackagePath, TestCommon.AICLIPath);
            }

            ShouldDisableDevModeOnExit = EnableDevMode(true);

            Assert.True(TestCommon.RunCommand("certutil.exe", "-addstore -f \"TRUSTEDPEOPLE\" " + TestCommon.GetTestDataFile(Constants.AppInstallerTestCert)), "Add AppInstallerTestCert");
            Assert.True(TestCommon.RunCommand("certutil.exe", "-addstore -f \"ROOT\" " + TestCommon.GetTestDataFile(Constants.IndexPackageRootCert)), "Add IndexPackageRootCert");

            if (TestCommon.PackagedContext)
            {
                if (TestCommon.LooseFileRegistration)
                {
                    Assert.True(TestCommon.InstallMsixRegister(TestCommon.AICLIPackagePath), "InstallMsixRegister");
                }
                else
                {
                    Assert.True(TestCommon.InstallMsix(TestCommon.AICLIPackagePath), "InstallMsix");
                }
            }

            if (TestContext.Parameters.Exists(Constants.StaticFileRootPathParameter))
            {
                TestCommon.StaticFileRootPath = TestContext.Parameters.Get(Constants.StaticFileRootPathParameter);
            }
            else
            {
                TestCommon.StaticFileRootPath = Path.GetTempPath();
            }

            if (TestContext.Parameters.Exists(Constants.PackageCertificatePathParameter))
            {
                TestCommon.PackageCertificatePath = TestContext.Parameters.Get(Constants.PackageCertificatePathParameter);
            }

            if (TestContext.Parameters.Exists(Constants.WindowsSDKPathParameter))
            {
                TestCommon.WindowsSDKPath = TestContext.Parameters.Get(Constants.WindowsSDKPathParameter);
            }

            ReadTestInstallerPaths();

            //Setup
            SetupTestLocalIndexDirectory();


            //Setup
            CopyInstallerFilesToLocalIndex();

            //Hash
            //HashInstallers();
            TestHashHelper.HashInstallers();

            //Hash
            //ReplaceManifestHashToken();
            string manifestDirectoryPath = Path.Combine(TestCommon.StaticFileRootPath, ManifestsName);
            TestHashHelper.ReplaceManifestHashToken(manifestDirectoryPath);

            //Setup
            SetupSourcePackage();
        }

        [OneTimeTearDown]
        public void TearDown()
        {
            if (ShouldDisableDevModeOnExit)
            {
                EnableDevMode(false);
            }

            TestCommon.RunCommand("certutil.exe", $"-delstore \"TRUSTEDPEOPLE\" {Constants.AppInstallerTestCertThumbprint}");
            TestCommon.RunCommand("certutil.exe", $"-delstore \"ROOT\" {Constants.IndexPackageRootCertThumbprint}");

            if (TestCommon.PackagedContext)
            {
                TestCommon.RemoveMsix(Constants.AICLIPackageName);
            }
        }

        // Returns whether there's a change to the dev mode state after execution
        private bool EnableDevMode(bool enable)
        {
            var appModelUnlockKey = Registry.LocalMachine.CreateSubKey(@"SOFTWARE\Microsoft\Windows\CurrentVersion\AppModelUnlock");

            if (enable)
            {
                var value = appModelUnlockKey.GetValue("AllowDevelopmentWithoutDevLicense");
                if (value == null || (Int32)value == 0)
                {
                    appModelUnlockKey.SetValue("AllowDevelopmentWithoutDevLicense", 1, RegistryValueKind.DWord);
                    return true;
                }
            }
            else
            {
                var value = appModelUnlockKey.GetValue("AllowDevelopmentWithoutDevLicense");
                if (value != null && ((UInt32)value) != 0)
                {
                    appModelUnlockKey.SetValue("AllowDevelopmentWithoutDevLicense", 0, RegistryValueKind.DWord);
                    return true;
                }
            }

            return false;
        }

        private void ReadTestInstallerPaths()
        {
            if (TestContext.Parameters.Exists(Constants.ExeInstallerPathParameter))
            {
                TestCommon.ExeInstallerPath = TestContext.Parameters.Get(Constants.ExeInstallerPathParameter);
            }
            else
            {
                TestCommon.ExeInstallerPath = TestCommon.GetTestFile("AppInstallerTestExeInstaller.exe");
            }

            if (TestContext.Parameters.Exists(Constants.MsiInstallerPathParameter))
            {
                TestCommon.MsiInstallerPath = TestContext.Parameters.Get(Constants.MsiInstallerPathParameter);
            }
            else
            {
                TestCommon.MsiInstallerPath = TestCommon.GetTestFile("AppInstallerTestMsiInstaller.msi");
            }

            if (TestContext.Parameters.Exists(Constants.MsixInstallerPathParameter))
            {
                TestCommon.MsixInstallerPath = TestContext.Parameters.Get(Constants.MsixInstallerPathParameter);
            }
            else
            {
                TestCommon.MsixInstallerPath = TestCommon.GetTestFile("AppInstallerTestMsixInstaller.msix");
            }
        }

        private void CopyInstallerFilesToLocalIndex()
        {
            string exeInstallerDestPath = Path.Combine(TestCommon.StaticFileRootPath, ExeInstallerName);
            DirectoryInfo exeInstallerDestDir = Directory.CreateDirectory(exeInstallerDestPath);

            string exeInstallerFullName = Path.Combine(exeInstallerDestDir.FullName, "AppInstallerTestExeInstaller.exe");

            File.Copy(TestCommon.ExeInstallerPath, exeInstallerFullName, true);
            TestCommon.ExeInstallerPath = exeInstallerFullName;
            //Sign Package
            string toolPath = @"C:\Program Files (x86)\Windows Kits\10\bin\10.0.18362.0\x64";
            RunCommand(toolPath + @"\signtool.exe", $"sign /a /fd sha256 /f {TestCommon.PackageCertificatePath} {exeInstallerFullName}");

            ////Msi Installer
            //string msiInstallerDestPath = Path.Combine(TestCommon.StaticFileRootPath, MsiInstallerName);
            //DirectoryInfo msiInstallerDestDir = Directory.CreateDirectory(msiInstallerDestPath);

            //string msiInstallerFullName = Path.Combine(msiInstallerDestDir.FullName, "AppInstallerTestMsiInstaller.msi");
            //File.Copy(TestCommon.MsiInstallerPath, msiInstallerFullName, true);
            //TestCommon.MsiInstallerPath = msiInstallerFullName;
            ////Sign Package
            //RunCommand(toolPath + @"\signtool.exe", $"sign /a /fd sha256 /f {TestCommon.PackageCertificatePath} {msiInstallerFullName}");
        }

        private void SetupSourcePackage()
        {
            string testLocalIndexRoot = TestCommon.StaticFileRootPath;
            string destIndexPath = Path.Combine(testLocalIndexRoot, PackageName, PublicName, IndexName);
            string certPath = TestCommon.PackageCertificatePath;

            DirectoryInfo parentDir = Directory.GetParent(Directory.GetCurrentDirectory());
            string indexCreationToolPath = Path.Combine(parentDir.FullName, IndexCreationToolName);
            string winGetUtilPath = Path.Combine(parentDir.FullName, WinGetUtilName);

            // Copy WingetUtil.dll app extension to IndexCreationTool Path
            File.Copy(Path.Combine(winGetUtilPath, @"WinGetUtil.dll"), Path.Combine(indexCreationToolPath, @"WinGetUtil.dll"), true);

            try
            {
                RunCommand(indexCreationToolPath + @"\IndexCreationTool.exe", $"-d {TestCommon.StaticFileRootPath}");
                File.Move(IndexName, destIndexPath, true);
                
                string packageDir = Path.Combine(TestCommon.StaticFileRootPath, PackageName);
                string indexPackageDestPath = Path.Combine(TestCommon.StaticFileRootPath, IndexPackageName);

                string toolPath = @"C:\Program Files (x86)\Windows Kits\10\bin\10.0.18362.0\x64";

                RunCommand(toolPath + @"\makeappx.exe", $"pack /l /o /d {packageDir} /p {indexPackageDestPath}");
                RunCommand(toolPath + @"\signtool.exe", $"sign /a /fd sha256 /f {certPath} {indexPackageDestPath}");
            }
            catch (Exception e)
            {
                Console.WriteLine("Failed. Reason: " + e.Message);
            }
        }

        private void SetupTestLocalIndexDirectory()
        {
            DirectoryInfo tempRootDir = Directory.CreateDirectory(TestCommon.StaticFileRootPath);

            foreach (FileInfo file in tempRootDir.GetFiles())
            {
                file.Delete();
            }

            foreach (DirectoryInfo dir in tempRootDir.GetDirectories())
            {
                dir.Delete(true);
            }

            string currentDirectory = Environment.CurrentDirectory;
            string sourcePath = Path.Combine(currentDirectory, TestDataName);

            DirectoryCopy(sourcePath, TestCommon.StaticFileRootPath);

            Console.WriteLine("TestLocalIndex Static File Root Created");
        }
        //Setup
        private void DirectoryCopy(string sourceDirName, string destDirName)
        {
            DirectoryInfo dir = new DirectoryInfo(sourceDirName);
            DirectoryInfo[] dirs = dir.GetDirectories();

            if (!Directory.Exists(destDirName))
            {
                Directory.CreateDirectory(destDirName);
            }

            FileInfo[] files = dir.GetFiles();
            foreach (FileInfo file in files)
            {
                string temppath = Path.Combine(destDirName, file.Name);
                file.CopyTo(temppath, false);
            }

            foreach (DirectoryInfo subdir in dirs)
            {
                string temppath = Path.Combine(destDirName, subdir.Name);
                DirectoryCopy(subdir.FullName, temppath);
            }
        }

        private void RunCommand(string command, string args)
        {
            Process p = new Process();
            p.StartInfo = new ProcessStartInfo(command, args);
            p.Start();
            p.WaitForExit();
        }
    }
}
