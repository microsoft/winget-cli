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
    using WinGetMCPServer.Extensions;

    internal class Program
    {
        private const string ServerName = "winget-mcp";

        /// <summary>
        /// Guidance sent to clients during initialization. Per the protocol this should describe how to
        /// use the server as a whole and must not restate the tool descriptions, which clients already
        /// receive from tools/list.
        /// </summary>
        private const string ServerInstructions = """
            This server manages software on this machine using the Windows Package Manager (WinGet).

            Search before installing. Use find-winget-packages to obtain an exact package identifier and
            pass that identifier to install-winget-package. Installing from a guessed identifier risks
            installing the wrong software, which is not trivially reversible.

            A separate lookup is not needed to determine whether something is already present:
            find-winget-packages reports installed state, installed version, and upgrade availability
            for every match.

            install-winget-package changes the state of this machine. It runs a real installer, which
            may require elevation, may take a long time, and may report that a reboot is required.
            Only install or upgrade packages the user has actually asked for, and report the status it
            returns rather than assuming that it succeeded.

            Tool results are JSON, provided both as text and as structured content.
            """;

        static void Main(string[] args)
        {
            var builder = Host.CreateApplicationBuilder();
            builder.Logging.AddConsole(consoleOptions => { consoleOptions.LogToStandardErrorThreshold = LogLevel.Trace; });

            var icons = ServerIcons.Icons;

            builder.Services
                .AddMcpServer(configureOptions =>
                {
                    configureOptions.ServerInfo = new Implementation() { Name = ServerName, Version = ServerConnection.Instance.Version, Icons = icons };
                    configureOptions.ServerInstructions = ServerInstructions;
                })
                .WithStdioServerTransport()
                .WithToolsAndIcons<WingetPackageTools>(icons);

            builder.Build().Run();
        }
    }
}
