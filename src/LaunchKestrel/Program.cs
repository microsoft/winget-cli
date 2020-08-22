// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace LaunchKestrel
{
    using Microsoft.AspNetCore.Builder;
    using Microsoft.AspNetCore.Hosting;
    using Microsoft.Extensions.Hosting;
    using System;
    using System.IO;
    using System.Security.Cryptography;

    public class Program
    {
        public static void Main(string[] args)
        {
            string ManifestDirectory = string.Empty;
            string ExeInstallerPath = string.Empty;
            string MsiInstallerPath = string.Empty;
            string MsixInstallerPath = string.Empty;

            for (int i = 0; i < args.Length; i++)
            {
                if (args[i] == "-d" && ++i < args.Length)
                {
                    ManifestDirectory = args[i];
                }
                else if (args[i] == " -e" && ++i < args.Length)
                {
                    ExeInstallerPath = args[i];
                }
                else if (args[i] == "-m" && ++i < args.Length)
                { 
                    MsiInstallerPath = args[i];
                }
                else if (args[i] == "-x" && ++i < args.Length)
                {
                    MsixInstallerPath = args[i];
                }
                else if (args[i] == "-s" && ++i < args.Length)
                {
                    Startup.StaticFileRoot = args[i];
                }
            }

            if (string.IsNullOrEmpty(ManifestDirectory))
            {
                Console.WriteLine("Usage: LaunchKestrel.exe -d <Testing Manifest Directory> -e <EXE Installer Path> " +
                    "-m <MSI Installer Path> -x <MSIX Installer Path> -s <Static File Root Served Through Kestrel> ");
                return;
            }

            //Generate Hash values for Installers
            string ExeInstallerHash = HashInstallerFile(ExeInstallerPath);
            string MsiInstallerHash = HashInstallerFile(MsiInstallerPath);
            string MsixInstallerHash = HashInstallerFile(MsixInstallerPath);

            //Modify Sha256 Hash Tokens for Manifests
            ReplaceManifestHashToken(ExeInstallerHash, MsiInstallerHash, MsixInstallerHash, ManifestDirectory);

            CreateHostBuilder(args).Build().Run();
        }

        public static IHostBuilder CreateHostBuilder(string[] args) =>
            Host.CreateDefaultBuilder(args)
                .ConfigureWebHostDefaults(webBuilder =>
                {
                    webBuilder.UseStartup<Startup>();
                });

        public static void ReplaceManifestHashToken(string ExeHashValue, string MsiHashValue, string MsixHashValue, string ManifestDirectory)
        {
            var dir = new DirectoryInfo(ManifestDirectory);
            FileInfo[] files = dir.GetFiles();

            foreach (FileInfo file in files)
            {
                string text = File.ReadAllText(file.FullName);

                if (text.Contains("<EXEHASH>"))
                {
                    text = text.Replace("<EXEHASH>", ExeHashValue);
                    File.WriteAllText(file.FullName, text);
                }
                else if (text.Contains("<MSIHASH>"))
                {
                    text = text.Replace("<MSIHASH>", MsiHashValue);
                    File.WriteAllText(file.FullName, text);
                }
                else if (text.Contains("<MSIXHASH>"))
                {
                    text = text.Replace("<MSIXHASH>", MsixHashValue);
                    File.WriteAllText(file.FullName, text);
                }
            }
        }

        public static string HashInstallerFile(string installerFilePath)
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
                    hash = BitConverter.ToString(hashValue);
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
    }
}
