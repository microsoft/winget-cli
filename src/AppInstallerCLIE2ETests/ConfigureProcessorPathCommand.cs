// -----------------------------------------------------------------------------
// <copyright file="ConfigureProcessorPathCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests
{
    using System;
    using System.IO;
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

        /// <summary>
        /// Verifies that when a signed Windows binary is provided as the processor path,
        /// the audit header, file path, SHA256 hash, and Authenticode signing subject
        /// all appear in the output.
        /// </summary>
        [Test]
        public void ProcessorPath_AuditOutput_SignedBinary()
        {
            // cmd.exe is always present and is Authenticode-signed by Microsoft.
            string processorPath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.System), "cmd.exe");
            string configFile = TestCommon.GetTestDataFile(@"Configuration\ShowDetails_DSCv3.yml");

            var result = TestCommon.RunAICLICommand(
                Command,
                $"--accept-configuration-agreements --processor-path \"{processorPath}\" \"{configFile}\"");

            // Audit header must appear regardless of whether the configure succeeds or fails,
            // because audit output happens during factory setup before DSC is invoked.
            Assert.True(result.StdOut.Contains("Custom DSC processor path in use:"), $"Expected audit header in output. StdOut: {result.StdOut}");
            Assert.True(result.StdOut.Contains($"  Path: {processorPath}"), $"Expected path in audit output. StdOut: {result.StdOut}");
            Assert.True(result.StdOut.Contains("  Hash: "), $"Expected hash in audit output. StdOut: {result.StdOut}");

            // cmd.exe is signed; the output must not say "(unsigned)".
            Assert.True(result.StdOut.Contains("  Signed By: "), $"Expected signing info in audit output. StdOut: {result.StdOut}");
            Assert.False(result.StdOut.Contains("  Signed By: (unsigned)"), $"Expected signed binary, not unsigned. StdOut: {result.StdOut}");
        }

        /// <summary>
        /// Verifies that the hash in the audit output is a 64-character lowercase hex string
        /// (SHA256 of the file contents).
        /// </summary>
        [Test]
        public void ProcessorPath_AuditOutput_HashIsValidSHA256()
        {
            string processorPath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.System), "cmd.exe");
            string configFile = TestCommon.GetTestDataFile(@"Configuration\ShowDetails_DSCv3.yml");

            var result = TestCommon.RunAICLICommand(
                Command,
                $"--accept-configuration-agreements --processor-path \"{processorPath}\" \"{configFile}\"");

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
                System.Text.RegularExpressions.Regex.IsMatch(hashValue, "^[0-9a-f]{64}$"),
                $"Expected lowercase hex hash, got: '{hashValue}'");
        }
    }
}
