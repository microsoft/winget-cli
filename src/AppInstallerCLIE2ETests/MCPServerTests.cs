// -----------------------------------------------------------------------------
// <copyright file="MCPServerTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using System.Text.Json;
    using System.Threading;
    using System.Threading.Tasks;
    using AppInstallerCLIE2ETests.Helpers;
    using ModelContextProtocol;
    using ModelContextProtocol.Client;
    using ModelContextProtocol.Protocol;
    using NUnit.Framework;

    /// <summary>
    /// Tests that act as an MCP client to the WinGet MCP server.
    /// Each test starts its own server, as the server keeps per-process consent state.
    /// </summary>
    public class MCPServerTests : BaseCommand
    {
        private const string FindPackagesToolName = "find-winget-packages";
        private const string InstallPackageToolName = "install-winget-package";

        private const string AgreementsPackageId = "AppInstallerTest.CatalogPackageMetadata";
        private const string PackageAgreementLabel = "testAgreementLabel";
        private const string PackageAgreementText = "testAgreementText";
        private const string PackageAgreementUrl = "https://testAgreementUrl.net";

        private const string AgreementsSourceName = "TestMCPAgreementsRestSource";

        private readonly AgreementsRestSource agreementsSource = new AgreementsRestSource(AgreementsSourceName);

        /// <summary>
        /// Ensures the server can be launched and that the test packages are not installed.
        /// </summary>
        [SetUp]
        public void Setup()
        {
            if (!TestSetup.Parameters.PackagedContext)
            {
                Assert.Ignore("The MCP server requires packaged context.");
            }

            CleanupTestPackages();
            this.agreementsSource.Remove();
        }

        /// <summary>
        /// Removes anything a test may have left behind.
        /// </summary>
        [TearDown]
        public void TearDown()
        {
            CleanupTestPackages();
            this.agreementsSource.Remove();
        }

        /// <summary>
        /// Verifies that the server lists its tools.
        /// </summary>
        /// <returns>A task.</returns>
        [Test]
        public async Task ListTools()
        {
            await using McpClient client = await CreateClientAsync();

            var tools = await client.ListToolsAsync();

            var find = tools.Single(t => t.Name == FindPackagesToolName);
            var install = tools.Single(t => t.Name == InstallPackageToolName);

            Assert.That(find.ProtocolTool.Annotations?.ReadOnlyHint, Is.True);
            Assert.That(install.ProtocolTool.Annotations?.ReadOnlyHint, Is.Not.True);
        }

        /// <summary>
        /// Verifies that finding packages returns the matching package from the test source.
        /// </summary>
        /// <returns>A task.</returns>
        [Test]
        public async Task FindPackages()
        {
            await using McpClient client = await CreateClientAsync();

            CallToolResult result = await client.CallToolAsync(
                FindPackagesToolName,
                new Dictionary<string, object> { ["query"] = Constants.ExeInstallerPackageId });

            Assert.That(result.IsError, Is.Not.True, GetText(result));

            using JsonDocument text = JsonDocument.Parse(GetText(result));
            Assert.That(text.RootElement.ValueKind, Is.EqualTo(JsonValueKind.Array));

            JsonElement package = text.RootElement.EnumerateArray().Single(p => p.GetProperty("identifier").GetString() == Constants.ExeInstallerPackageId);
            Assert.That(package.GetProperty("source").GetString(), Is.EqualTo(Constants.TestSourceName));
            Assert.That(package.GetProperty("isInstalled").GetBoolean(), Is.False);

            Assert.That(result.StructuredContent, Is.Not.Null);
            Assert.That(result.StructuredContent.Value.TryGetProperty("packages", out JsonElement packages), Is.True);
            Assert.That(packages.GetArrayLength(), Is.EqualTo(text.RootElement.GetArrayLength()));
            Assert.That(result.StructuredContent.Value.TryGetProperty("excludedSources", out _), Is.False);
        }

        /// <summary>
        /// Verifies that a query that matches nothing returns an empty list rather than an error.
        /// </summary>
        /// <returns>A task.</returns>
        [Test]
        public async Task FindPackages_NoMatches()
        {
            await using McpClient client = await CreateClientAsync();

            CallToolResult result = await client.CallToolAsync(
                FindPackagesToolName,
                new Dictionary<string, object> { ["query"] = "MCPTestQueryThatMatchesNothing" });

            Assert.That(result.IsError, Is.Not.True, GetText(result));

            using JsonDocument text = JsonDocument.Parse(GetText(result));
            Assert.That(text.RootElement.GetArrayLength(), Is.EqualTo(0));
        }

        /// <summary>
        /// Verifies that a query is required unless searching for upgradeable packages.
        /// </summary>
        /// <returns>A task.</returns>
        [Test]
        public async Task FindPackages_NoQuery()
        {
            await using McpClient client = await CreateClientAsync();

            CallToolResult result = await client.CallToolAsync(
                FindPackagesToolName,
                new Dictionary<string, object> { ["upgradeable"] = false });

            Assert.That(result.IsError, Is.True);
        }

        /// <summary>
        /// Verifies that a package is installed, reporting progress.
        /// </summary>
        /// <returns>A task.</returns>
        [Test]
        public async Task InstallPackage()
        {
            await using McpClient client = await CreateClientAsync();

            int progressCount = 0;
            CallToolResult result = await client.CallToolAsync(
                InstallPackageToolName,
                new Dictionary<string, object> { ["identifier"] = Constants.ExeInstallerPackageId, ["source"] = Constants.TestSourceName },
                progress: new Progress<ProgressNotificationValue>(_ => Interlocked.Increment(ref progressCount)));

            AssertInstalled(result);
            Assert.That(() => Volatile.Read(ref progressCount), Is.GreaterThan(0).After(5000, 100));
        }

        /// <summary>
        /// Verifies the response for an identifier that does not match any package.
        /// </summary>
        /// <returns>A task.</returns>
        [Test]
        public async Task InstallPackage_NotFound()
        {
            await using McpClient client = await CreateClientAsync();

            CallToolResult result = await client.CallToolAsync(
                InstallPackageToolName,
                new Dictionary<string, object> { ["identifier"] = "AppInstallerTest.MCPDoesNotExist", ["source"] = Constants.TestSourceName });

            Assert.That(GetStructuredString(result, "message"), Does.Contain("Did not find a package"));
            Assert.That(GetStructuredString(result, "identifier"), Is.EqualTo("AppInstallerTest.MCPDoesNotExist"));
        }

        /// <summary>
        /// Verifies that an upgrade is not attempted for a package that is not installed.
        /// </summary>
        /// <returns>A task.</returns>
        [Test]
        public async Task InstallPackage_UpgradeOnlyNotInstalled()
        {
            await using McpClient client = await CreateClientAsync();

            CallToolResult result = await client.CallToolAsync(
                InstallPackageToolName,
                new Dictionary<string, object> { ["identifier"] = Constants.ExeInstallerPackageId, ["source"] = Constants.TestSourceName, ["upgradeOnly"] = true });

            Assert.That(result.IsError, Is.True);
            Assert.That(GetStructuredString(result, "message"), Does.Contain("not installed"));
        }

        /// <summary>
        /// Verifies that package agreements are returned to a client without elicitation, and that the
        /// package is installed once the caller conveys the user's acceptance.
        /// </summary>
        /// <returns>A task.</returns>
        [Test]
        public async Task PackageAgreements_Fallback_Accepted()
        {
            await using McpClient client = await CreateClientAsync();

            CallToolResult result = await InstallAgreementsPackageAsync(client, acceptPackageAgreements: null);
            AssertPackageAgreementsRequired(result);

            result = await InstallAgreementsPackageAsync(client, acceptPackageAgreements: true);
            AssertInstalled(result);
        }

        /// <summary>
        /// Verifies that a caller cannot accept package agreements that it was never given.
        /// </summary>
        /// <returns>A task.</returns>
        [Test]
        public async Task PackageAgreements_Fallback_AcceptedWithoutPresentation()
        {
            await using McpClient client = await CreateClientAsync();

            CallToolResult result = await InstallAgreementsPackageAsync(client, acceptPackageAgreements: true);
            AssertPackageAgreementsRequired(result);
        }

        /// <summary>
        /// Verifies that the package is not installed when the user declines its agreements.
        /// </summary>
        /// <returns>A task.</returns>
        [Test]
        public async Task PackageAgreements_Fallback_Declined()
        {
            await using McpClient client = await CreateClientAsync();

            CallToolResult result = await InstallAgreementsPackageAsync(client, acceptPackageAgreements: false);

            Assert.That(result.IsError, Is.True);
            Assert.That(GetStructuredString(result, "message"), Does.Contain("did not accept"));
            Assert.That(TestCommon.VerifyTestExeInstalled(Path.GetTempPath()), Is.False);
        }

        /// <summary>
        /// Verifies that the user is asked to accept package agreements through elicitation, even when the caller claims acceptance.
        /// </summary>
        /// <returns>A task.</returns>
        [Test]
        public async Task PackageAgreements_Elicitation_Accepted()
        {
            var elicitation = new ElicitationRecorder(ElicitationRecorder.Accept);
            await using McpClient client = await CreateClientAsync(elicitation);

            CallToolResult result = await InstallAgreementsPackageAsync(client, acceptPackageAgreements: true);

            Assert.That(elicitation.Requests.Count, Is.EqualTo(1));
            Assert.That(elicitation.Requests[0].Message, Does.Contain(PackageAgreementLabel));
            Assert.That(elicitation.Requests[0].Message, Does.Contain(PackageAgreementText));
            Assert.That(elicitation.Requests[0].Message, Does.Contain(PackageAgreementUrl));
            AssertInstalled(result);
        }

        /// <summary>
        /// Verifies that the package is not installed unless the user explicitly accepts its agreements through elicitation.
        /// </summary>
        /// <param name="action">The elicitation action taken by the user.</param>
        /// <param name="checkedBox">Whether the user checked the acceptance box.</param>
        /// <returns>A task.</returns>
        [TestCase("decline", false)]
        [TestCase("cancel", false)]
        [TestCase("accept", false)]
        public async Task PackageAgreements_Elicitation_NotAccepted(string action, bool checkedBox)
        {
            var elicitation = new ElicitationRecorder(ElicitationRecorder.Respond(action, checkedBox));
            await using McpClient client = await CreateClientAsync(elicitation);

            CallToolResult result = await InstallAgreementsPackageAsync(client, acceptPackageAgreements: null);

            Assert.That(elicitation.Requests.Count, Is.EqualTo(1));
            Assert.That(result.IsError, Is.True);
            Assert.That(GetStructuredString(result, "message"), Does.Contain("did not accept"));
            Assert.That(TestCommon.VerifyTestExeInstalled(Path.GetTempPath()), Is.False);
        }

        /// <summary>
        /// Verifies that source agreements are returned to a client without elicitation, and that the acceptance is persisted.
        /// </summary>
        /// <returns>A task.</returns>
        [Test]
        public async Task SourceAgreements_Fallback_Accepted()
        {
            this.agreementsSource.AddWithUnacceptedAgreements();

            await using (McpClient client = await CreateClientAsync())
            {
                CallToolResult result = await FindTestPackageAsync(client, acceptSourceAgreements: null);
                AssertSourceAgreementsRequired(result);

                result = await FindTestPackageAsync(client, acceptSourceAgreements: true);
                Assert.That(HasAgreements(result), Is.False, GetText(result));
            }

            // A new server must find the acceptance persisted.
            await using (McpClient client = await CreateClientAsync())
            {
                CallToolResult result = await FindTestPackageAsync(client, acceptSourceAgreements: null);
                Assert.That(HasAgreements(result), Is.False, GetText(result));
            }
        }

        /// <summary>
        /// Verifies that declining source agreements excludes that source while still returning results from the others.
        /// </summary>
        /// <returns>A task.</returns>
        [Test]
        public async Task SourceAgreements_Fallback_Declined()
        {
            this.agreementsSource.AddWithUnacceptedAgreements();

            await using McpClient client = await CreateClientAsync();

            CallToolResult result = await FindTestPackageAsync(client, acceptSourceAgreements: false);
            AssertExcludedAgreementsSource(result);
        }

        /// <summary>
        /// Verifies that the user is asked to accept source agreements through elicitation, and that the acceptance is persisted.
        /// </summary>
        /// <returns>A task.</returns>
        [Test]
        public async Task SourceAgreements_Elicitation_Accepted()
        {
            this.agreementsSource.AddWithUnacceptedAgreements();

            var elicitation = new ElicitationRecorder(ElicitationRecorder.Accept);
            await using (McpClient client = await CreateClientAsync(elicitation))
            {
                CallToolResult result = await FindTestPackageAsync(client, acceptSourceAgreements: null);

                Assert.That(elicitation.Requests.Count, Is.EqualTo(1));
                Assert.That(elicitation.Requests[0].Message, Does.Contain(AgreementsSourceName));
                Assert.That(elicitation.Requests[0].Message, Does.Contain(AgreementsRestSource.AgreementLabel));
                Assert.That(elicitation.Requests[0].Message, Does.Contain(AgreementsRestSource.AgreementText));
                Assert.That(elicitation.Requests[0].Message, Does.Contain(AgreementsRestSource.AgreementUrl));
                Assert.That(GetExcludedSources(result), Does.Not.Contain(AgreementsSourceName));
            }

            // A new server must not ask again.
            elicitation = new ElicitationRecorder(ElicitationRecorder.Respond("decline", false));
            await using (McpClient client = await CreateClientAsync(elicitation))
            {
                CallToolResult result = await FindTestPackageAsync(client, acceptSourceAgreements: null);

                Assert.That(elicitation.Requests.Count, Is.EqualTo(0));
                Assert.That(GetExcludedSources(result), Does.Not.Contain(AgreementsSourceName));
            }
        }

        /// <summary>
        /// Verifies that declining source agreements through elicitation excludes that source, and that the user
        /// is not asked again in the same session unless the caller indicates the user has changed their mind.
        /// </summary>
        /// <returns>A task.</returns>
        [Test]
        public async Task SourceAgreements_Elicitation_Declined()
        {
            this.agreementsSource.AddWithUnacceptedAgreements();

            var elicitation = new ElicitationRecorder(ElicitationRecorder.Respond("decline", false));
            await using McpClient client = await CreateClientAsync(elicitation);

            CallToolResult result = await FindTestPackageAsync(client, acceptSourceAgreements: null);
            Assert.That(elicitation.Requests.Count, Is.EqualTo(1));
            AssertExcludedAgreementsSource(result);

            result = await FindTestPackageAsync(client, acceptSourceAgreements: null);
            Assert.That(elicitation.Requests.Count, Is.EqualTo(1), "A declined source is not asked about again.");
            AssertExcludedAgreementsSource(result);

            result = await FindTestPackageAsync(client, acceptSourceAgreements: true);
            Assert.That(elicitation.Requests.Count, Is.EqualTo(2), "The user is asked again when the caller indicates acceptance.");
            AssertExcludedAgreementsSource(result);
        }

        private static void CleanupTestPackages()
        {
            // Other tests may have installed these packages elsewhere, and the test installer's uninstaller
            // does not remove the marker file that is used to detect an install to the temp directory.
            TestCommon.RunAICLICommand("uninstall", Constants.ExeInstallerPackageId);
            TestCommon.RunAICLICommand("uninstall", AgreementsPackageId);
            File.Delete(Path.Combine(Path.GetTempPath(), Constants.TestExeInstalledFileName));
            File.Delete(Path.Combine(Path.GetTempPath(), Constants.TestExeUninstallerFileName));
        }

        private static Task<McpClient> CreateClientAsync(ElicitationRecorder elicitation = null)
        {
            McpClientOptions options = new McpClientOptions();
            if (elicitation != null)
            {
                options.Handlers.ElicitationHandler = elicitation.HandleAsync;
            }

            return McpClient.CreateAsync(
                new StdioClientTransport(new StdioClientTransportOptions()
                {
                    Name = "WinGet",
                    Command = TestSetup.Parameters.MCPServerPath,
                }),
                options);
        }

        private static async Task<CallToolResult> InstallAgreementsPackageAsync(McpClient client, bool? acceptPackageAgreements)
        {
            var arguments = new Dictionary<string, object> { ["identifier"] = AgreementsPackageId, ["source"] = Constants.TestSourceName };
            if (acceptPackageAgreements.HasValue)
            {
                arguments["acceptPackageAgreements"] = acceptPackageAgreements.Value;
            }

            return await client.CallToolAsync(InstallPackageToolName, arguments);
        }

        private static async Task<CallToolResult> FindTestPackageAsync(McpClient client, bool? acceptSourceAgreements)
        {
            var arguments = new Dictionary<string, object> { ["query"] = Constants.ExeInstallerPackageId };
            if (acceptSourceAgreements.HasValue)
            {
                arguments["acceptSourceAgreements"] = acceptSourceAgreements.Value;
            }

            return await client.CallToolAsync(FindPackagesToolName, arguments);
        }

        private static void AssertInstalled(CallToolResult result)
        {
            Assert.That(result.IsError, Is.Not.True, GetText(result));
            Assert.That(GetStructuredString(result, "message"), Is.EqualTo("Install completed successfully"));
            Assert.That(TestCommon.VerifyTestExeInstalledAndCleanup(Path.GetTempPath()), Is.True);
        }

        private static void AssertPackageAgreementsRequired(CallToolResult result)
        {
            Assert.That(result.IsError, Is.Not.True, GetText(result));

            JsonElement agreement = GetAgreements(result).Single();
            Assert.That(agreement.GetProperty("kind").GetString(), Is.EqualTo("package"));
            Assert.That(agreement.GetProperty("package").GetString(), Does.Contain(AgreementsPackageId));
            Assert.That(agreement.GetProperty("label").GetString(), Is.EqualTo(PackageAgreementLabel));
            Assert.That(agreement.GetProperty("text").GetString(), Is.EqualTo(PackageAgreementText));
            Assert.That(agreement.GetProperty("url").GetString(), Is.EqualTo(PackageAgreementUrl));
            Assert.That(TestCommon.VerifyTestExeInstalled(Path.GetTempPath()), Is.False);
        }

        private static void AssertSourceAgreementsRequired(CallToolResult result)
        {
            Assert.That(result.IsError, Is.Not.True, GetText(result));

            var agreements = GetAgreements(result);
            Assert.That(agreements.All(a => a.GetProperty("kind").GetString() == "source" && a.GetProperty("source").GetString() == AgreementsSourceName), Is.True, GetText(result));

            JsonElement agreement = agreements.Single(a => a.TryGetProperty("label", out JsonElement label) && label.GetString() == AgreementsRestSource.AgreementLabel);
            Assert.That(agreement.GetProperty("text").GetString(), Is.EqualTo(AgreementsRestSource.AgreementText));
            Assert.That(agreement.GetProperty("url").GetString(), Is.EqualTo(AgreementsRestSource.AgreementUrl));
        }

        private static void AssertExcludedAgreementsSource(CallToolResult result)
        {
            Assert.That(result.IsError, Is.Not.True, GetText(result));
            Assert.That(GetExcludedSources(result), Is.EquivalentTo(new[] { AgreementsSourceName }));

            using JsonDocument text = JsonDocument.Parse(GetText(result));
            Assert.That(text.RootElement.EnumerateArray().Any(p => p.GetProperty("identifier").GetString() == Constants.ExeInstallerPackageId), Is.True, "Results from the remaining sources are returned.");
            Assert.That(result.Content.OfType<TextContentBlock>().Skip(1).Single().Text, Does.Contain(AgreementsSourceName));
        }

        private static string GetText(CallToolResult result)
        {
            return result.Content.OfType<TextContentBlock>().FirstOrDefault()?.Text ?? string.Empty;
        }

        private static string GetStructuredString(CallToolResult result, string propertyName)
        {
            Assert.That(result.StructuredContent, Is.Not.Null, GetText(result));
            Assert.That(result.StructuredContent.Value.TryGetProperty(propertyName, out JsonElement value), Is.True, GetText(result));
            return value.GetString();
        }

        private static bool HasAgreements(CallToolResult result)
        {
            return result.StructuredContent?.TryGetProperty("agreements", out _) == true;
        }

        private static List<JsonElement> GetAgreements(CallToolResult result)
        {
            Assert.That(HasAgreements(result), Is.True, GetText(result));
            return result.StructuredContent.Value.GetProperty("agreements").EnumerateArray().ToList();
        }

        private static List<string> GetExcludedSources(CallToolResult result)
        {
            if (result.StructuredContent?.TryGetProperty("excludedSources", out JsonElement excluded) == true)
            {
                return excluded.EnumerateArray().Select(e => e.GetString()).ToList();
            }

            return new List<string>();
        }

        /// <summary>
        /// Records elicitation requests and responds to them as configured.
        /// </summary>
        private sealed class ElicitationRecorder
        {
            private readonly Func<ElicitResult> respond;

            /// <summary>
            /// Initializes a new instance of the <see cref="ElicitationRecorder"/> class.
            /// </summary>
            /// <param name="respond">Creates the response to each request.</param>
            public ElicitationRecorder(Func<ElicitResult> respond)
            {
                this.respond = respond;
            }

            /// <summary>
            /// Gets a responder that accepts with the acceptance box checked.
            /// </summary>
            public static Func<ElicitResult> Accept => Respond("accept", true);

            /// <summary>
            /// Gets the requests received.
            /// </summary>
            public List<ElicitRequestParams> Requests { get; } = new List<ElicitRequestParams>();

            /// <summary>
            /// Creates a responder.
            /// </summary>
            /// <param name="action">The action taken by the user.</param>
            /// <param name="checkedBox">Whether the user checked the acceptance box.</param>
            /// <returns>The responder.</returns>
            public static Func<ElicitResult> Respond(string action, bool checkedBox)
            {
                return () => new ElicitResult()
                {
                    Action = action,
                    Content = action == "accept" ? new Dictionary<string, JsonElement> { ["accept"] = JsonSerializer.SerializeToElement(checkedBox) } : null,
                };
            }

            /// <summary>
            /// Handles an elicitation request.
            /// </summary>
            /// <param name="request">The request.</param>
            /// <param name="cancellationToken">The cancellation token.</param>
            /// <returns>The response.</returns>
            public ValueTask<ElicitResult> HandleAsync(ElicitRequestParams request, CancellationToken cancellationToken)
            {
                lock (this.Requests)
                {
                    this.Requests.Add(request);
                }

                return ValueTask.FromResult(this.respond());
            }
        }
    }
}
