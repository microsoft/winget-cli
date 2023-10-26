// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace LocalhostWebServer
{
    using Microsoft.AspNetCore.Builder;
    using Microsoft.AspNetCore.Hosting;
    using Microsoft.Extensions.Hosting;
    using System;
    using System.IO;
    using Microsoft.Extensions.Configuration;
    using System.Security.Cryptography.X509Certificates;
    using System.Text.Json.Serialization;
    using System.Text.Json;
    using WinGetSourceCreator.Model;
    using Microsoft.WinGetSourceCreator;
    using System.Runtime.InteropServices;

    public class Program
    {    
        static void Main(string[] args)
        {
            IConfiguration config = new ConfigurationBuilder()
                .AddCommandLine(args)
                .Build();

            Startup.StaticFileRoot = config.GetValue<string>("StaticFileRoot");
            Startup.CertPath = config.GetValue<string>("CertPath");
            Startup.CertPassword = config.GetValue<string>("CertPassword");
            Startup.Port = config.GetValue<Int32>("Port", 5001);
            Startup.OutCertFile = config.GetValue<string>("OutCertFile");
            Startup.LocalSourceJson = config.GetValue<string>("LocalSourceJson");
            Startup.TestDataPath = config.GetValue<string>("TestDataPath");

            if (string.IsNullOrEmpty(Startup.StaticFileRoot) || 
                string.IsNullOrEmpty(Startup.CertPath))
            {
                Console.WriteLine("Usage: LocalhostWebServer.exe StaticFileRoot=<Path to Serve Static Root Directory> " +
                    "CertPath=<Path to HTTPS Developer Certificate> CertPassword=<Certificate Password> <Port=Port Number> <PutCertInRoot=Boolean>");
                return;
            }

            Directory.CreateDirectory(Startup.StaticFileRoot);

            if (!string.IsNullOrEmpty(Startup.OutCertFile))
            {
                string parent = Path.GetDirectoryName(Startup.OutCertFile);
                if (!string.IsNullOrEmpty(parent))
                {
                    Directory.CreateDirectory(parent);
                }

                X509Certificate2 serverCertificate = new X509Certificate2(Startup.CertPath, Startup.CertPassword, X509KeyStorageFlags.EphemeralKeySet);
                File.WriteAllBytes(Startup.OutCertFile, serverCertificate.Export(X509ContentType.Cert));
            }

            if (!string.IsNullOrEmpty(Startup.LocalSourceJson))
            {
                if (!File.Exists(Startup.LocalSourceJson))
                {
                    throw new FileNotFoundException(Startup.LocalSourceJson);
                }

                WinGetLocalSource.CreateFromLocalSourceFile(Startup.LocalSourceJson);
            }

            if (!string.IsNullOrEmpty(Startup.TestDataPath))
            {
                if (!Directory.Exists(Startup.TestDataPath))
                {
                    throw new DirectoryNotFoundException(Startup.TestDataPath);
                }

                var testDataDirectory = Path.Combine(Startup.StaticFileRoot, "TestData");
                Directory.CreateDirectory(testDataDirectory);

                CopyDirectoryRecursive(Startup.TestDataPath, testDataDirectory);
            }

            CreateHostBuilder(args).Build().Run();
        }

        public static IHostBuilder CreateHostBuilder(string[] args) =>
            Host.CreateDefaultBuilder(args)
                .ConfigureWebHostDefaults(webBuilder =>
                {
                    webBuilder.UseKestrel(opt =>
                    {
                        opt.ListenAnyIP(Startup.Port, listOpt =>
                        {
                            listOpt.UseHttps(Startup.CertPath, Startup.CertPassword);
                        });
                    });
                    webBuilder.UseContentRoot(Startup.StaticFileRoot);
                    webBuilder.UseStartup<Startup>();
                });

        private static void CopyDirectoryRecursive(string sourceDir, string destDir)
        {
            if (!Directory.Exists(destDir))
            {
                Directory.CreateDirectory(destDir);
            }

            string[] files = Directory.GetFiles(sourceDir);
            foreach (string file in files)
            {
                string dest = Path.Combine(destDir, Path.GetFileName(file));
                File.Copy(file, dest);
            }

            string[] directories = Directory.GetDirectories(sourceDir);
            foreach (string dir in directories)
            {
                string dest = Path.Combine(destDir, Path.GetFileName(dir));
                CopyDirectoryRecursive(dir, dest);
            }
        }
    }
}