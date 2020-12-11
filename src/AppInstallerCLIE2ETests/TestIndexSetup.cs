// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Diagnostics;
using System.IO;
using Microsoft.Msix.Utils.ProcessRunner;

namespace AppInstallerCLIE2ETests
{
    public class TestIndexSetup
    {
        private const string TestDataName = "TestData";
        private const string ManifestsName = "Manifests";
        private const string PackageName = "Package";
        private const string PublicName = "Public";

        /// <summary>
        /// Generates the Local Test Index to be served by the Localhost Web Server.
        /// 1. Copies TestData to a StaticFileRootPath set in Test.runsettings file
        /// 2. Copies and signs installer files (EXE or MSIX)
        /// 3. Hashes installer Files
        /// 4. Replaces manifests with corresponding hash values
        /// 5. Generates a source package for TestData using makeappx/signtool
        /// </summary>
        public static void GenerateTestDirectory()
        {
            SetupLocalTestDirectory(TestCommon.StaticFileRootPath);

            if (!string.IsNullOrEmpty(TestCommon.ExeInstallerPath))
            {
                CopyExeInstallerToTestDirectory();
            }

            if (!string.IsNullOrEmpty(TestCommon.MsiInstallerPath))
            {
                CopyMsiInstallerToTestDirectory();
            }

            if (!string.IsNullOrEmpty(TestCommon.MsixInstallerPath))
            {
                CopyMsixInstallerToTestDirectory();
            }

            TestHashHelper.HashInstallers();

            string manifestDirectoryPath = Path.Combine(TestCommon.StaticFileRootPath, ManifestsName);
            TestHashHelper.ReplaceManifestHashToken(manifestDirectoryPath);

            SetupSourcePackage();
        }

        private static void SetupSourcePackage()
        {
            string indexDestPath = Path.Combine(TestCommon.StaticFileRootPath, PackageName, PublicName);

            DirectoryInfo parentDir = Directory.GetParent(Directory.GetCurrentDirectory());
            string indexCreationToolPath = Path.Combine(parentDir.FullName, Constants.IndexCreationTool);
            string winGetUtilPath = Path.Combine(parentDir.FullName, Constants.WinGetUtil);

            // Copy WingetUtil.dll app extension to IndexCreationTool Path
            File.Copy(Path.Combine(winGetUtilPath, @"WinGetUtil.dll"), Path.Combine(indexCreationToolPath, @"WinGetUtil.dll"), true);

            try
            {
                if (!Directory.Exists(indexDestPath))
                {
                   Directory.CreateDirectory(indexDestPath);
                }

                // Generate Index.db file using IndexCreationTool.exe
                RunCommand(Path.Combine(indexCreationToolPath, "IndexCreationTool.exe"), $"-d {TestCommon.StaticFileRootPath}", indexDestPath);

                string packageDir = Path.Combine(TestCommon.StaticFileRootPath, PackageName);
                string indexPackageDestPath = Path.Combine(TestCommon.StaticFileRootPath, Constants.IndexPackage);
                string pathToSDK = SDKDetector.Instance.LatestSDKBinPath;

                // Package Test Source and Sign With Package Certificate
                string makeappxExecutable = Path.Combine(pathToSDK, "makeappx.exe");
                RunCommand(makeappxExecutable, $"pack /nv /v /o /d {packageDir} /p {indexPackageDestPath}");
                SignFile(indexPackageDestPath);
            }
            catch (Exception e)
            {
                Console.WriteLine("Failed. Reason: " + e.Message);
            }
        }

        private static void CopyExeInstallerToTestDirectory()
        {
            // Set Exe Test Installer Path
            string exeInstallerDestPath = Path.Combine(TestCommon.StaticFileRootPath, Constants.ExeInstaller);
            DirectoryInfo exeInstallerDestDir = Directory.CreateDirectory(exeInstallerDestPath);
            string exeInstallerFullName = Path.Combine(exeInstallerDestDir.FullName, "AppInstallerTestExeInstaller.exe");

            // Copy Exe Test Installer to Destination Path
            File.Copy(TestCommon.ExeInstallerPath, exeInstallerFullName, true);
            TestCommon.ExeInstallerPath = exeInstallerFullName;

            // Sign EXE Installer File
            SignFile(TestCommon.ExeInstallerPath);
        }

        private static void CopyMsiInstallerToTestDirectory()
        {
            // Set MSI Test Installer Path
            string msiInstallerDestPath = Path.Combine(TestCommon.StaticFileRootPath, Constants.MsiInstaller);
            DirectoryInfo msiInstallerDestDir = Directory.CreateDirectory(msiInstallerDestPath);

            // Copy MSI Test Installer to Destination Path
            string msiInstallerFullName = Path.Combine(msiInstallerDestDir.FullName, "AppInstallerTestMsiInstaller.msi");

            File.Copy(TestCommon.MsiInstallerPath, msiInstallerFullName, true);
            TestCommon.MsiInstallerPath = msiInstallerFullName;

            // Sign MSI Installer File
            SignFile(TestCommon.MsiInstallerPath);
        }

        private static void CopyMsixInstallerToTestDirectory()
        {
            // Set Msix Test Installer Path
            string msixInstallerDestPath = Path.Combine(TestCommon.StaticFileRootPath, Constants.MsixInstaller);
            DirectoryInfo msixInstallerDestDir = Directory.CreateDirectory(msixInstallerDestPath);

            // Copy Msix Test Installer to Destination Path
            string msixInstallerFullName = Path.Combine(msixInstallerDestDir.FullName, "AppInstallerTestMsixInstaller.msix");

            File.Copy(TestCommon.MsixInstallerPath, msixInstallerFullName, true);
            TestCommon.MsixInstallerPath = msixInstallerFullName;

            // Sign MSIX Installer File
            SignFile(TestCommon.MsixInstallerPath);
        }

        private static void SetupLocalTestDirectory(string staticFileRootPath)
        {
            DirectoryInfo staticFileRootDir = Directory.CreateDirectory(staticFileRootPath);

            DeleteDirectoryContents(staticFileRootDir);

            string currentDirectory = Environment.CurrentDirectory;
            string sourcePath = Path.Combine(currentDirectory, TestDataName);

            CopyDirectory(sourcePath, TestCommon.StaticFileRootPath);
        }

        public static void SignFile(string filePath)
        {
            string pathToSDK = SDKDetector.Instance.LatestSDKBinPath;
            string signtoolExecutable = Path.Combine(pathToSDK, "signtool.exe");
            RunCommand(signtoolExecutable, $"sign /a /fd sha256 /f {TestCommon.PackageCertificatePath} {filePath}");
        }

        /// <summary>
        /// Deletes the contents of a given directory
        /// </summary>
        /// <param name="directory"></param>
        public static void DeleteDirectoryContents(DirectoryInfo directory)
        {
            foreach (FileInfo file in directory.GetFiles())
            {
                file.Delete();
            }

            foreach (DirectoryInfo dir in directory.GetDirectories())
            {
                dir.Delete(true);
            }
        }

        /// <summary>
        /// Copies the contents of a given directory from a source path to a destination path
        /// </summary>
        /// <param name="sourceDirName"></param>
        /// <param name="destDirName"></param>
        public static void CopyDirectory(string sourceDirName, string destDirName)
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
                CopyDirectory(subdir.FullName, temppath);
            }
        }

        public static void RunCommand(string command, string args)
        {
            Process p = new Process();
            p.StartInfo = new ProcessStartInfo(command, args);
            p.Start();
            p.WaitForExit();
        }

        public static void RunCommand (string command, string args, string workingDirectory)
        {
            Process p = new Process();
            p.StartInfo = new ProcessStartInfo(command, args);
            p.StartInfo.WorkingDirectory = workingDirectory;
            p.Start();
            p.WaitForExit();
        }
    }
}
