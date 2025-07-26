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

        static void Main(string[] args)
        {
            var builder = Host.CreateApplicationBuilder();
            builder.Logging.AddConsole(consoleOptions => { consoleOptions.LogToStandardErrorThreshold = LogLevel.Trace; });

            builder.Services
                .AddMcpServer(configureOptions =>
                {
                    // TODO: More options setup?
                    configureOptions.ServerInfo = new Implementation() { Name = ServerName, Version = ServerConnection.Instance.Version };
                })
                .WithStdioServerTransport()
                .WithTools<WingetPackageTools>();

            builder.Build().Run();
        }
    }
}
