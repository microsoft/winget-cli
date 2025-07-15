// -----------------------------------------------------------------------------
// <copyright file="Program.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace WinGetMCPServer
{
    using Microsoft.Extensions.DependencyInjection;
    using Microsoft.Extensions.Hosting;
    using Microsoft.Extensions.Logging;
    using ModelContextProtocol.Protocol;

    internal class Program
    {
        private const string ServerName = "winget-mcp";
        // TODO: Same version as winget
        private const string ServerVersion = "0.0.1";

        static void Main(string[] args)
        {
            var builder = Host.CreateApplicationBuilder();
            builder.Logging.AddConsole(consoleOptions => { consoleOptions.LogToStandardErrorThreshold = LogLevel.Trace; });

            builder.Services
                .AddMcpServer(configureOptions =>
                {
                    // TODO: More options setup?
                    configureOptions.ServerInfo = new Implementation() { Name = ServerName, Version = ServerVersion };
                })
                .WithStdioServerTransport()
                .WithTools<WingetPackageTools>();

            builder.Build().Run();
        }
    }
}
