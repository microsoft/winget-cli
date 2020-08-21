// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
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
            string PathToTestingManifests = @"C:\Users\ryfu\source\repos\winget-cli\src\AppInstallerCLIE2ETests\TestData\Manifests";

            string ExeInstallerPath = string.Empty;
            string MsiInstallerPath = string.Empty;
            string MsixInstallerPath = string.Empty;

            string ExeInstallerHash = string.Empty;
            string MsiInstallerHash = string.Empty;
            string MsixInstallerHash = string.Empty;

            for (int i = 0; i < args.Length; i++)
            {
                if (args[i] == " - e" && ++i < args.Length)
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
            }

            if (args.Length > 0 && !string.IsNullOrEmpty(args[0]))
			{
				TestCommon.StaticFileRoot = args[0];
            }
            else
			{
				Console.WriteLine("Usage: AppInstallerCLIE2ETests <Path to Serve Static Root Directory> ");
				return;
			}

            //Generate Hash values for Installers
            ExeInstallerHash = HashInstaller(ExeInstallerPath);
            MsiInstallerHash = HashInstaller(MsiInstallerPath);
            MsixInstallerHash = HashInstaller(MsixInstallerPath);

            //Modify Sha256 Hash Tokens for Manifests
            ReplaceManifestHashToken(ExeInstallerHash, MsiInstallerHash, MsixInstallerHash, PathToTestingManifests);

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

        public static string HashInstaller(string installerFilePath)
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
