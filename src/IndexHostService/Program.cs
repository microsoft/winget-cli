// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace IndexHostService
{
    using Microsoft.AspNetCore.Builder;
    using Microsoft.AspNetCore.Hosting;
    using Microsoft.Extensions.Hosting;
    using Microsoft.Extensions.Logging;
    using System;
    using System.IO;

    public class Program
    {    

        static void Main(string[] args)
        {
            for (int i = 0; i < args.Length; i++)
            {
                if (args[i] == "-d" && ++i < args.Length)
                {
                    Startup.StaticFileRoot = args[i];
                }
                else if (args[i] == "-c" && ++i < args.Length)
                {
                    Startup.CertPath = args[i];
                }
                else if (args[i] == "-p" && ++i < args.Length)
                {
                    Startup.CertPassword = args[i];
                }
            }

            if (string.IsNullOrEmpty(Startup.StaticFileRoot))
            {
                Console.WriteLine("Usage: IndexHostService.exe <Path to Serve Static Root Directory>");
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
                        opt.ListenAnyIP(5001, listOpt =>
                        {
                            listOpt.UseHttps(Startup.CertPath, Startup.CertPassword);
                        });
                    });
                    webBuilder.UseContentRoot(Startup.StaticFileRoot);
                    webBuilder.UseStartup<Startup>();
                });
    }
}