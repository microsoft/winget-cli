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
        const string CertificateProviderString = "Microsoft.PowerShell.Security\\Certificate::";
        const string StoreLocationCurrentUser = "CurrentUser";
        const string StoreLocationLocalMachine = "LocalMachine";

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
            Startup.ExitBeforeRun = config.GetValue<bool>("ExitBeforeRun");

            if (string.IsNullOrEmpty(Startup.StaticFileRoot) || 
                string.IsNullOrEmpty(Startup.CertPath))
            {
                Console.WriteLine("Usage: LocalhostWebServer.exe StaticFileRoot=<Path to Serve Static Root Directory> " +
                    "CertPath=<Path to HTTPS Developer Certificate> CertPassword=<Certificate Password> <Port=Port Number> <PutCertInRoot=Boolean>");
                return;
            }

            Directory.CreateDirectory(Startup.StaticFileRoot);

            if (Startup.CertPath.StartsWith(CertificateProviderString))
            {
                string certPath = Startup.CertPath.Substring(CertificateProviderString.Length);
                string[] pathParts = certPath.Split('\\');

                if (pathParts.Length != 3)
                {
                    throw new InvalidDataException($"Don't know how to handle: {Startup.CertPath}");
                }

                StoreLocation storeLocation = StoreLocation.CurrentUser;
                if (pathParts[0] == StoreLocationCurrentUser)
                {
                    // The default
                }
                else if (pathParts[0] == StoreLocationLocalMachine)
                {
                    storeLocation = StoreLocation.LocalMachine;
                }
                else
                {
                    throw new InvalidDataException($"Unknown store scope: {Startup.CertPath}");
                }

                X509Store x509Store = new X509Store(pathParts[1], storeLocation);
                x509Store.Open(OpenFlags.ReadOnly | OpenFlags.OpenExistingOnly);
                X509Certificate2Collection collection = x509Store.Certificates;

                if (collection.Count == 0)
                {
                    throw new InvalidDataException($"Found {collection.Count} certificates in store '{pathParts[0]}' [{storeLocation}] \\ '{pathParts[1]}': {Startup.CertPath}");
                }

                X509Certificate2Collection results = collection.Find(X509FindType.FindByThumbprint, pathParts[2], true);

                if (results.Count != 1)
                {
                    throw new InvalidDataException($"Found {results.Count} matches for '{pathParts[2]}': {Startup.CertPath}");
                }

                ServerCertificate = results[0];
            }
            else
            {
                ServerCertificate = new X509Certificate2(Startup.CertPath, Startup.CertPassword);
            }

            if (!string.IsNullOrEmpty(Startup.OutCertFile))
            {
                string parent = Path.GetDirectoryName(Startup.OutCertFile);
                if (!string.IsNullOrEmpty(parent))
                {
                    Directory.CreateDirectory(parent);
                }

                File.WriteAllBytes(Startup.OutCertFile, ServerCertificate.Export(X509ContentType.Cert));
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

            if (Startup.ExitBeforeRun)
            {
                return;
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
                            listOpt.UseHttps(ServerCertificate);
                        });
                    });
                    webBuilder.UseContentRoot(Startup.StaticFileRoot);
                    webBuilder.UseStartup<Startup>();
                });

        public static X509Certificate2 ServerCertificate { get; private set; }

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
                File.Copy(file, dest, overwrite: true);
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
