// -----------------------------------------------------------------------------
// <copyright file="MCPServerTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests
{
    using System.Collections.Generic;
    using System.Linq;
    using System.Text.Json;
    using System.Threading.Tasks;
    using AppInstallerCLIE2ETests.Helpers;
    using ModelContextProtocol.Client;
    using ModelContextProtocol.Protocol;
    using NUnit.Framework;

    /// <summary>
    /// Tests that act as an MCP client to the WinGet MCP server.
    /// </summary>
    public class MCPServerTests
    {
        private const string FindPackagesToolName = "find-winget-packages";
        private const string InstallPackageToolName = "install-winget-package";

        /// <summary>
        /// Ensures the server can be launched.
        /// </summary>
        [SetUp]
        public void Setup()
        {
            if (!TestSetup.Parameters.PackagedContext)
            {
                Assert.Ignore("The MCP server requires packaged context.");
            }
        }

        /// <summary>
        /// Verifies that the server lists its tools.
        /// </summary>
        /// <returns>A task.</returns>
        [Test]
        public async Task ListTools()
        {
            await using McpClient client = await CreateClientAsync();

            var toolNames = (await client.ListToolsAsync()).Select(t => t.Name).ToList();

            Assert.That(toolNames, Does.Contain(FindPackagesToolName));
            Assert.That(toolNames, Does.Contain(InstallPackageToolName));
        }

        /// <summary>
        /// Verifies that finding packages returns a package list.
        /// </summary>
        /// <returns>A task.</returns>
        [Test]
        public async Task FindPackages()
        {
            await using McpClient client = await CreateClientAsync();

            // This client does not support elicitation, so decline any source agreements to get results from the
            // remaining sources rather than a request to present the agreements.
            CallToolResult result = await client.CallToolAsync(
                FindPackagesToolName,
                new Dictionary<string, object> { ["query"] = "PowerShell", ["acceptSourceAgreements"] = false });

            Assert.That(result.IsError, Is.Not.True, GetText(result));

            using JsonDocument text = JsonDocument.Parse(GetText(result));
            Assert.That(text.RootElement.ValueKind, Is.EqualTo(JsonValueKind.Array));

            Assert.That(result.StructuredContent, Is.Not.Null);
            Assert.That(result.StructuredContent.Value.TryGetProperty("packages", out JsonElement packages), Is.True);
            Assert.That(packages.GetArrayLength(), Is.EqualTo(text.RootElement.GetArrayLength()));
        }

        private static Task<McpClient> CreateClientAsync()
        {
            return McpClient.CreateAsync(new StdioClientTransport(new StdioClientTransportOptions()
            {
                Name = "WinGet",
                Command = TestSetup.Parameters.MCPServerPath,
            }));
        }

        private static string GetText(CallToolResult result)
        {
            return result.Content.OfType<TextContentBlock>().FirstOrDefault()?.Text ?? string.Empty;
        }
    }
}
