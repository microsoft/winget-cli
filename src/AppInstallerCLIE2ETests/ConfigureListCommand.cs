// -----------------------------------------------------------------------------
// <copyright file="ConfigureListCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests
{
    using System.IO;
    using AppInstallerCLIE2ETests.Helpers;
    using NUnit.Framework;

    /// <summary>
    /// `Configure list` command tests.
    /// </summary>
    public class ConfigureListCommand
    {
        private const string ConfigureWithAgreementsAndVerbose = "configure --accept-configuration-agreements --verbose";
        private const string ConfigureTestRepoFile = "Configure_TestRepo.yml";

        /// <summary>
        /// Teardown done once after all the tests here.
        /// </summary>
        [OneTimeTearDown]
        public void OneTimeTeardown()
        {
            this.DeleteTxtFiles();
        }

        /// <summary>
        /// Applies a configuration, then verifies that it is in the overall list.
        /// </summary>
        [Test]
        public void ListAllConfigurations()
        {
            var result = TestCommon.RunAICLICommand(ConfigureWithAgreementsAndVerbose, TestCommon.GetTestDataFile("Configuration\\Configure_TestRepo.yml"));
            Assert.That(result.ExitCode, Is.EqualTo(0));

            result = TestCommon.RunAICLICommand("configure list", "--verbose");
            Assert.That(result.ExitCode, Is.EqualTo(0));
            Assert.That(result.StdOut.Contains(ConfigureTestRepoFile), Is.True);
        }

        /// <summary>
        /// Applies a configuration (to ensure at least one exists), gets the overall list, then the details about the first configuration.
        /// </summary>
        [Test]
        public void ListSpecificConfiguration()
        {
            var result = TestCommon.RunAICLICommand(ConfigureWithAgreementsAndVerbose, TestCommon.GetTestDataFile("Configuration\\Configure_TestRepo.yml"));
            Assert.That(result.ExitCode, Is.EqualTo(0));

            string guid = TestCommon.GetConfigurationInstanceIdentifierFor(ConfigureTestRepoFile);
            result = TestCommon.RunAICLICommand("configure list", $"-h {guid}");
            Assert.That(result.ExitCode, Is.EqualTo(0));
            Assert.That(result.StdOut.Contains(guid), Is.True);
            Assert.That(result.StdOut.Contains(ConfigureTestRepoFile), Is.True);
        }

        /// <summary>
        /// Applies a configuration (to ensure at least one exists), gets the overall list, then the removes the first configuration.
        /// </summary>
        [Test]
        public void RemoveConfiguration()
        {
            var result = TestCommon.RunAICLICommand(ConfigureWithAgreementsAndVerbose, TestCommon.GetTestDataFile("Configuration\\Configure_TestRepo.yml"));
            Assert.That(result.ExitCode, Is.EqualTo(0));

            string guid = TestCommon.GetConfigurationInstanceIdentifierFor(ConfigureTestRepoFile);
            result = TestCommon.RunAICLICommand("configure list", $"-h {guid} --remove");
            Assert.That(result.ExitCode, Is.EqualTo(0));

            result = TestCommon.RunAICLICommand("configure list", "--verbose");
            Assert.That(result.ExitCode, Is.EqualTo(0));
            Assert.That(result.StdOut.Contains(guid), Is.False);
        }

        /// <summary>
        /// Applies a configuration (to ensure at least one exists), gets the overall list, then the outputs the first configuration.
        /// </summary>
        [Test]
        public void OutputConfiguration()
        {
            var result = TestCommon.RunAICLICommand(ConfigureWithAgreementsAndVerbose, TestCommon.GetTestDataFile("Configuration\\Configure_TestRepo.yml"));
            Assert.That(result.ExitCode, Is.EqualTo(0));

            string guid = TestCommon.GetConfigurationInstanceIdentifierFor(ConfigureTestRepoFile);
            string tempFile = TestCommon.GetRandomTestFile(".yml");
            result = TestCommon.RunAICLICommand("configure list", $"-h {guid} --output {tempFile}");
            Assert.That(result.ExitCode, Is.EqualTo(0));

            result = TestCommon.RunAICLICommand("configure validate", $"--verbose {tempFile}");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_FALSE));
        }

        private void DeleteTxtFiles()
        {
            // Delete all .txt files in the test directory; they are placed there by the tests
            foreach (string file in Directory.GetFiles(TestCommon.GetTestDataFile("Configuration"), "*.txt"))
            {
                File.Delete(file);
            }
        }
    }
}
