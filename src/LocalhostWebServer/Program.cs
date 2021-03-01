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
            
            if (string.IsNullOrEmpty(Startup.StaticFileRoot) || 
                string.IsNullOrEmpty(Startup.CertPath) || 
                string.IsNullOrEmpty(Startup.CertPassword))
            {
                Console.WriteLine("Usage: LocalhostWebServer.exe StaticFileRoot=<Path to Serve Static Root Directory> " +
                    "CertPath=<Path to HTTPS Developer Certificate> CertPassword=<Certificate Password> <Port=Port Number>");
                return;
            }

            Directory.CreateDirectory(Startup.StaticFileRoot);
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
    }
}