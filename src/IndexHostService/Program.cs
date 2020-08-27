﻿// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace IndexHostService
{
    using Microsoft.AspNetCore.Builder;
    using Microsoft.AspNetCore.Hosting;
    using Microsoft.Extensions.Hosting;
    using System;
    using System.IO;

    public class Program
    {
        static void Main(string[] args)
        {
            if (args.Length > 0 && !string.IsNullOrEmpty(args[0]))
            {
                Startup.StaticFileRoot = args[0];
            }
            else
            {
                Console.WriteLine("Usage: IndexHostService.exe <Path to Serve Static Root Directory>");
                return;
            }

            Directory.CreateDirectory(Startup.StaticFileRoot);
            Console.WriteLine("Begin Serving:" + Startup.StaticFileRoot + "Through HTTPS");

            CreateHostBuilder(args).Build().Run();
        }

        public static IHostBuilder CreateHostBuilder(string[] args) =>
            Host.CreateDefaultBuilder(args)
                .ConfigureWebHostDefaults(webBuilder =>
                {
                    webBuilder.UseStartup<Startup>();
                });
    }
}
