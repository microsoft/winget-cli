// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Diagnostics;
using System.IO;

namespace AppInstallerCLIE2ETests
{
    public class TestIndexSetup
    {
        private const string TestDataName = "TestData";
        private const string ManifestsName = "Manifests";
        private const string PackageName = "Package";
        private const string IndexName = @"index.db";
        private const string PublicName = "Public";

        /// <summary>
        /// Generates the Local Test Index to be served by the Localhost Web Server.
        /// Copies TestData to a StaticFileRootPath set in Test.runsettings
        /// Copies and signs installer Files 
        /// Hashes installer Files
        /// Replaces manifests with corresponding hash values
        /// Generates a source package for TestData using makeappx/signtool
        /// </summary>
        public static void GenerateTestIndex()
        {
            SetupTestLocalIndexDirectory(TestCommon.StaticFileRootPath);

            CopyInstallerFilesToLocalIndex();

            TestHashHelper.HashInstallers();

            string manifestDirectoryPath = Path.Combine(TestCommon.StaticFileRootPath, ManifestsName);
            TestHashHelper.ReplaceManifestHashToken(manifestDirectoryPath);

            SetupSourcePackage();
        }

        private static void SetupSourcePackage()
        {
            string indexDestPath = Path.Combine(TestCommon.StaticFileRootPath, PackageName, PublicName, IndexName);

            DirectoryInfo parentDir = Directory.GetParent(Directory.GetCurrentDirectory());

            string indexCreationToolPath = Path.Combine(parentDir.FullName, Constants.IndexCreationTool);
            string winGetUtilPath = Path.Combine(parentDir.FullName, Constants.WinGetUtil);

            // Copy WingetUtil.dll app extension to IndexCreationTool Path
            File.Copy(Path.Combine(winGetUtilPath, @"WinGetUtil.dll"), Path.Combine(indexCreationToolPath, @"WinGetUtil.dll"), true);

            try
            {
                RunCommand(Path.Combine(indexCreationToolPath, "IndexCreationTool.exe"), $"-d {TestCommon.StaticFileRootPath}");
                File.Move(IndexName, indexDestPath, true);

                string packageDir = Path.Combine(TestCommon.StaticFileRootPath, PackageName);
                string indexPackageDestPath = Path.Combine(TestCommon.StaticFileRootPath, Constants.IndexPackage);

                string makeappxExecutable = Path.Combine(TestCommon.WindowsSDKPath, "makeappx.exe");
                string signtoolExecutable = Path.Combine(TestCommon.WindowsSDKPath, "signtool.exe");

                RunCommand(makeappxExecutable, $"pack /l /o /d {packageDir} /p {indexPackageDestPath}");
                RunCommand(signtoolExecutable, $"sign /a /fd sha256 /f {TestCommon.PackageCertificatePath} {indexPackageDestPath}");
            }
            catch (Exception e)
            {
                Console.WriteLine("Failed. Reason: " + e.Message);
            }
        }

        private static void CopyInstallerFilesToLocalIndex()
        {
            string exeInstallerDestPath = Path.Combine(TestCommon.StaticFileRootPath, Constants.ExeInstaller);
            DirectoryInfo exeInstallerDestDir = Directory.CreateDirectory(exeInstallerDestPath);

            string exeInstallerFullName = Path.Combine(exeInstallerDestDir.FullName, "AppInstallerTestExeInstaller.exe");

            File.Copy(TestCommon.ExeInstallerPath, exeInstallerFullName, true);
            TestCommon.ExeInstallerPath = exeInstallerFullName;

            string signtoolExecutable = Path.Combine(TestCommon.WindowsSDKPath, "signtool.exe");

            RunCommand(signtoolExecutable, $"sign /a /fd sha256 /f {TestCommon.PackageCertificatePath} {exeInstallerFullName}");
        }

        private static void SetupTestLocalIndexDirectory(string staticFileRootPath)
        {
            DirectoryInfo staticFileRootDir = Directory.CreateDirectory(staticFileRootPath);

            foreach (FileInfo file in staticFileRootDir.GetFiles())
            {
                file.Delete();
            }

            foreach (DirectoryInfo dir in staticFileRootDir.GetDirectories())
            {
                dir.Delete(true);
            }

            string currentDirectory = Environment.CurrentDirectory;
            string sourcePath = Path.Combine(currentDirectory, TestDataName);

            DirectoryCopy(sourcePath, TestCommon.StaticFileRootPath);
        }

        public static void DirectoryCopy(string sourceDirName, string destDirName)
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

        public static void RunCommand(string command, string args)
        {
            Process p = new Process();
            p.StartInfo = new ProcessStartInfo(command, args);
            p.Start();
            p.WaitForExit();
        }
    }
}
