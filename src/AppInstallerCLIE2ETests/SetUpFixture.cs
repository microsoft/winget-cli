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
        private const string TestDirectoryName = @"\TestData";
        private const string ManifestsDirectoryName = @"\Manifests";
        private const string TestLocalIndexName = "TestLocalIndex";
        private const string AppXManifestFileName = @"\AppxManifest.xml";

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
        }

        private void SetupSourcePackage()
        {
            string tempPath = Path.GetTempPath();
            string rootDir = tempPath + TestLocalIndexName + TestDirectoryName;
            string appxManifestPath = tempPath + TestLocalIndexName + AppXManifestFileName;
            string certPath = TestCommon.PackageCertificatePath;

            //string currDir = Directory.GetCurrentDirectory();
            //string parentDir = Directory.GetParent(currDir);
            //string indexCreationToolPath = parentDir + "IndexCreationTool";

            //Run IndexCreationTool to create source package
            string command = @"C:\Users\ryfu\source\repos\winget-cli\src\x64\Debug\IndexCreationTool\IndexCreationTool.exe";
            string args = $"-d {rootDir} -m {appxManifestPath}";
            Process p = new Process();
            p.StartInfo = new ProcessStartInfo(command, args);
            p.Start();
            p.WaitForExit();

            // TODO : Run Makeappx and Sign to create package for testing

            File.Move(Environment.CurrentDirectory + @"\source.msix", TestDirectoryName);
        }

        private void SetupTestLocalIndexDirectory()
        {
            string tempPath = Path.GetTempPath();
            string testLocalIndexTempRoot = tempPath + TestLocalIndexName;

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
            string sourcePath = currentDirectory + TestDirectoryName;

            DirectoryCopy(sourcePath, testLocalIndexTempRoot);
        }

        private void ReplaceManifestHashToken()
        {
            string manifestFullPath = Path.GetTempPath() + TestLocalIndexName + ManifestsDirectoryName;

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
    }
}
