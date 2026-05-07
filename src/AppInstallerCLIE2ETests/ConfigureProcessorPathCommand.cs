// -----------------------------------------------------------------------------
// <copyright file="ConfigureProcessorPathCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests
{
    using System;
    using System.IO;
    using System.Text.RegularExpressions;
    using AppInstallerCLIE2ETests.Helpers;
    using NUnit.Framework;

    /// <summary>
    /// E2E tests for the `--processor-path` argument of the configure command.
    /// These tests verify that audit information (path, hash, signing) is shown
    /// when a custom DSC processor path is provided.
    /// </summary>
    public class ConfigureProcessorPathCommand
    {
        private const string Command = "configure";
        private const string ConfigurationProcessorPath = "ConfigurationProcessorPath";

        // DSC app execution alias paths (stable then preview).
        private static readonly string[] DscAliasCandidates = new[]
        {
            Path.Combine(
                Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
                @"Microsoft\WindowsApps\Microsoft.DesiredStateConfiguration_8wekyb3d8bbwe\dsc.exe"),
            Path.Combine(
                Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
                @"Microsoft\WindowsApps\Microsoft.DesiredStateConfiguration-Preview_8wekyb3d8bbwe\dsc.exe"),
        };

        /// <summary>
        /// Enables the LocalProcessorPath admin setting before each test.
        /// </summary>
        [SetUp]
        public void SetUp()
        {
            WinGetSettingsHelper.EnableAdminSetting(ConfigurationProcessorPath);
        }

        /// <summary>
        /// Disables the LocalProcessorPath admin setting after each test.
        /// </summary>
        [TearDown]
        public void TearDown()
        {
            WinGetSettingsHelper.DisableAdminSetting(ConfigurationProcessorPath);
        }

        /// <summary>
        /// Verifies that when the DSC executable is provided as the processor path,
        /// the audit header, file path, SHA256 hash, and app-execution-alias marker
        /// all appear in the output.
        /// </summary>
        [Test]
        public void ProcessorPath_AuditOutput_ShowsPathAndHash()
        {
            string processorPath = FindDscExePath();
            if (processorPath == null)
            {
                Assert.Ignore("DSC is not installed; skipping processor path audit test.");
                return;
            }

            string configFile = TestCommon.GetTestDataFile(@"Configuration\ShowDetails_DSCv3.yml");

            var result = TestCommon.RunAICLICommand(
                Command,
                $"--accept-configuration-agreements --processor-path \"{processorPath}\" \"{configFile}\" --no-progress");

            // Audit header must appear regardless of whether the configure succeeds or fails,
            // because audit output happens during factory setup before DSC is invoked.
            Assert.True(result.StdOut.Contains("Custom processor path:"), $"Expected audit header in output. StdOut: {result.StdOut}");
            Assert.True(result.StdOut.Contains($"  Path: {processorPath}"), $"Expected path in audit output. StdOut: {result.StdOut}");
            Assert.True(result.StdOut.Contains("  Hash: "), $"Expected hash in audit output. StdOut: {result.StdOut}");

            // dsc.exe is an app execution alias; the alias marker must be present.
            Assert.True(result.StdOut.Contains("Type: App execution alias"), $"Expected app execution alias marker. StdOut: {result.StdOut}");
        }

        /// <summary>
        /// Verifies that the hash in the audit output is a 64-character lowercase hex string
        /// (SHA256 of the file or reparse buffer contents).
        /// </summary>
        [Test]
        public void ProcessorPath_AuditOutput_HashIsValidSHA256()
        {
            string processorPath = FindDscExePath();
            if (processorPath == null)
            {
                Assert.Ignore("DSC is not installed; skipping processor path hash format test.");
                return;
            }

            string configFile = TestCommon.GetTestDataFile(@"Configuration\ShowDetails_DSCv3.yml");

            var result = TestCommon.RunAICLICommand(
                Command,
                $"--accept-configuration-agreements --processor-path \"{processorPath}\" \"{configFile}\" --no-progress");

            Assert.True(result.StdOut.Contains("  Hash: "), $"Expected hash in audit output. StdOut: {result.StdOut}");

            // Extract the hash value from "  Hash: <value>"
            int hashLabelIndex = result.StdOut.IndexOf("  Hash: ");
            Assert.That(hashLabelIndex, Is.GreaterThanOrEqualTo(0));

            int hashStart = hashLabelIndex + "  Hash: ".Length;
            int hashEnd = result.StdOut.IndexOfAny(new[] { '\r', '\n' }, hashStart);
            string hashValue = hashEnd > hashStart
                ? result.StdOut.Substring(hashStart, hashEnd - hashStart).Trim()
                : result.StdOut.Substring(hashStart).Trim();

            Assert.AreEqual(64, hashValue.Length, $"Expected 64-character SHA256 hash, got: '{hashValue}'");
            Assert.True(
                Regex.IsMatch(hashValue, "^[0-9a-f]{64}$"),
                $"Expected lowercase hex hash, got: '{hashValue}'");
        }

        /// <summary>
        /// Gets the path to dsc.exe (app execution alias), or null if not installed.
        /// </summary>
        private static string FindDscExePath()
        {
            foreach (string candidate in DscAliasCandidates)
            {
                if (File.Exists(candidate))
                {
                    return candidate;
                }
            }

            return null;
        }
    }
}
