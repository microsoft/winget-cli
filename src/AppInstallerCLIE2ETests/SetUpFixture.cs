// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
{
    using Microsoft.Win32;
    using NUnit.Framework;
    using System;
    using System.IO;
    using System.Diagnostics;
    using System.Security.Cryptography;
    using IndexCreationTool;

    [SetUpFixture]
    public class SetUpFixture
    {
        private static bool ShouldDisableDevModeOnExit = true;
        private const string TestDataName = "TestData";
        private const string ManifestsName = "Manifests";
        private const string TestLocalIndexName = "TestLocalIndex";
        private const string PackageName = "Package";
        private const string IndexName = @"index.db";
        private const string PublicName = "Public";
        private const string IndexCreationToolName = "IndexCreationTool";

        private const string ExeInstallerName = @"AppInstallerTestExeInstaller";
        private const string IndexPackageName = @"source.msix";

        private string ExeInstallerHashValue { get; set; }

        private string MsiInstallerHashValue { get; set; }

        private string MsixInstallerHashValue { get; set; }


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

            ReadTestInstallerPaths();

            SetupTestLocalIndexDirectory();

            CopyInstallerFilesToLocalIndex();

            HashInstallers();

            ReplaceManifestHashToken();

            SetupSourcePackage();

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
            string exeInstallerDestPath = Path.Combine(Path.GetTempPath(), TestLocalIndexName, ExeInstallerName);
            DirectoryInfo exeInstallerDestDir = Directory.CreateDirectory(exeInstallerDestPath);

            string exeInstallerFullName = Path.Combine(exeInstallerDestDir.FullName, "AppInstallerTestExeInstaller.exe");

            File.Copy(TestCommon.ExeInstallerPath, exeInstallerFullName, true);
        }

        private void SetupSourcePackage()
        {
            string testRootDir = Path.Combine(Path.GetTempPath(), TestLocalIndexName, TestDataName);
            string testLocalIndexRoot = Path.Combine(Path.GetTempPath(), TestLocalIndexName);
            string destIndexPath = Path.Combine(testLocalIndexRoot, PackageName, PublicName, IndexName);
            string certPath = TestCommon.PackageCertificatePath;

            DirectoryInfo parentDir = Directory.GetParent(Directory.GetCurrentDirectory());
            string indexCreationToolPath = Path.Combine(parentDir.FullName, IndexCreationToolName);

            try
            {
                RunCommand(indexCreationToolPath + @"\IndexCreationTool.exe", $"-d {testRootDir}");

                //Move Index.db to Package/Index 
                File.Move(IndexName, destIndexPath, true);

                string packageDir = Path.Combine(Path.GetTempPath(), TestLocalIndexName, PackageName);

                // Create Index Package and Sign With Certificate
                RunCommand("makeappx.exe", $"pack /l /d {packageDir} /p {IndexPackageName}");
                RunCommand("signtool.exe", $"sign / a / fd sha256 / f {certPath} {IndexPackageName}");

                // Move Package to TestLocalIndex 
                File.Move(IndexPackageName, testLocalIndexRoot, true);
            }
            catch (Exception e)
            {
                Console.WriteLine("Failed. Reason: " + e.Message);
            }
        }

        private void SetupTestLocalIndexDirectory()
        {
            string testLocalIndexTempRoot = Path.Combine(Path.GetTempPath(), TestLocalIndexName);

            DirectoryInfo tempRootDir = Directory.CreateDirectory(testLocalIndexTempRoot);

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

            DirectoryCopy(sourcePath, testLocalIndexTempRoot);
        }

        private void ReplaceManifestHashToken()
        {
            string manifestFullPath = Path.Combine(Path.GetTempPath(), TestLocalIndexName, ManifestsName);

            var dir = new DirectoryInfo(manifestFullPath);
            FileInfo[] files = dir.GetFiles();

            foreach (FileInfo file in files)
            {
                string text = File.ReadAllText(file.FullName);

                if (text.Contains("<EXEHASH>"))
                {
                    text = text.Replace("<EXEHASH>", ExeInstallerHashValue);
                    File.WriteAllText(file.FullName, text);
                }
                else if (text.Contains("<MSIHASH>"))
                {
                    text = text.Replace("<MSIHASH>", MsiInstallerHashValue);
                    File.WriteAllText(file.FullName, text);
                }
                else if (text.Contains("<MSIXHASH>"))
                {
                    text = text.Replace("<MSIXHASH>", MsixInstallerHashValue);
                    File.WriteAllText(file.FullName, text);
                }
            }
        }

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

        private void HashInstallers()
        {
            ExeInstallerHashValue = HashInstallerFile(TestCommon.ExeInstallerPath);
            //HashInstallerFile(TestCommon.MsiInstallerPath);
            //HashInstallerFile(TestCommon.MsixInstallerPath);
        }

        private string HashInstallerFile(string installerFilePath)
        {
            FileInfo installerFile = new FileInfo(installerFilePath);
            string hash = string.Empty;

            using (SHA256 mySHA256 = SHA256.Create())
            {
                try
                {
                    FileStream fileStream = installerFile.Open(FileMode.Open);
                    fileStream.Position = 0;
                    byte[] hashValue = mySHA256.ComputeHash(fileStream);
                    hash = ConvertHashByteToString(hashValue);
                    fileStream.Close();
                }
                catch (IOException e)
                {
                    Console.WriteLine($"I/O Exception: {e.Message}");
                }
                catch (UnauthorizedAccessException e)
                {
                    Console.WriteLine($"Access Exception: {e.Message}");
                }
            }

            return hash;
        }

        private string ConvertHashByteToString(byte[] array)
        {
            string hashValue = string.Empty;

            for (int i = 0; i < array.Length; i++)
            {
                hashValue = hashValue + $"{array[i]:X2}";
            }

            return hashValue;
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
