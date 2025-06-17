using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Hosting;
using Microsoft.Extensions.Logging;
using ModelContextProtocol.Protocol;
using ModelContextProtocol.Server;

namespace WinGetMCPServer
{
    internal class Program
    {
        private const string ServerName = "winget-mcp";
        // TODO: Same version as winget
        private const string ServerVersion = "0.0.1";

        static void Main(string[] args)
        {
            RunWithToolsType<WingetPackage_Reflection>();
        }

        private static void RunWithToolsType<ToolsType>()
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
                .WithTools<ToolsType>();

            builder.Build().Run();
        }
    }
}
