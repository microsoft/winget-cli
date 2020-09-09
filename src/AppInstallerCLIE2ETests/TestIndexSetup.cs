using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Text;

namespace AppInstallerCLIE2ETests
{
    public class TestIndexSetup
    {
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

        private void SetupSourcePackage()
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

                string toolPath = @"C:\Program Files (x86)\Windows Kits\10\bin\10.0.18362.0\x64";

                RunCommand(toolPath + @"\makeappx.exe", $"pack /l /o /d {packageDir} /p {indexPackageDestPath}");
                RunCommand(toolPath + @"\signtool.exe", $"sign /a /fd sha256 /f {TestCommon.PackageCertificatePath} {indexPackageDestPath}");
            }
            catch (Exception e)
            {
                Console.WriteLine("Failed. Reason: " + e.Message);
            }
        }

        public static void CopyInstallerFilesToLocalIndex()
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

        public static void SetupTestLocalIndexDirectory(string staticFileRootPath)
        {
            DirectoryInfo tempRootDir = Directory.CreateDirectory(staticFileRootPath);

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
