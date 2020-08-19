// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
{
    using Microsoft.AspNetCore.Builder;
    using Microsoft.AspNetCore.Hosting;
    using Microsoft.Extensions.Hosting;
	using System;

	public class Program
	{
		public static void Main(string[] args)
		{
			if (args.Length > 0 && !string.IsNullOrEmpty(args[0]))
			{
				TestCommon.StaticFileRoot = args[0];
            }
            else
			{
				Console.WriteLine("Usage: AppInstallerCLIE2ETests <Path to Serve Static Root Directory>");
				return;
			}
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
