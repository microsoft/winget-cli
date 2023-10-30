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
                X509Certificate2Collection collection = x509Store.Certificates;
                X509Certificate2Collection results = collection.Find(X509FindType.FindByThumbprint, pathParts[2], true);

                if (results.Count != 1)
                {
                    throw new InvalidDataException($"Found {results.Count} matches for: {Startup.CertPath}");
                }

                ServerCertificate = results[0];
            }
            else
            {
                ServerCertificate = new X509Certificate2(Startup.CertPath, Startup.CertPassword, X509KeyStorageFlags.EphemeralKeySet);
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
    }
}