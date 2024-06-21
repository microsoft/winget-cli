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
            Assert.AreEqual(0, result.ExitCode);

            result = TestCommon.RunAICLICommand("configure list", "--verbose");
            Assert.AreEqual(0, result.ExitCode);
            Assert.True(result.StdOut.Contains(ConfigureTestRepoFile));
        }

        /// <summary>
        /// Applies a configuration (to ensure at least one exists), gets the overall list, then the details about the first configuration.
        /// </summary>
        [Test]
        public void ListSpecificConfiguration()
        {
            var result = TestCommon.RunAICLICommand(ConfigureWithAgreementsAndVerbose, TestCommon.GetTestDataFile("Configuration\\Configure_TestRepo.yml"));
            Assert.AreEqual(0, result.ExitCode);

            string guid = TestCommon.GetConfigurationInstanceIdentifierFor(ConfigureTestRepoFile);
            result = TestCommon.RunAICLICommand("configure list", $"-h {guid}");
            Assert.AreEqual(0, result.ExitCode);
            Assert.True(result.StdOut.Contains(guid));
            Assert.True(result.StdOut.Contains(ConfigureTestRepoFile));
        }

        /// <summary>
        /// Applies a configuration (to ensure at least one exists), gets the overall list, then the removes the first configuration.
        /// </summary>
        [Test]
        public void RemoveConfiguration()
        {
            var result = TestCommon.RunAICLICommand(ConfigureWithAgreementsAndVerbose, TestCommon.GetTestDataFile("Configuration\\Configure_TestRepo.yml"));
            Assert.AreEqual(0, result.ExitCode);

            string guid = TestCommon.GetConfigurationInstanceIdentifierFor(ConfigureTestRepoFile);
            result = TestCommon.RunAICLICommand("configure list", $"-h {guid} --remove");
            Assert.AreEqual(0, result.ExitCode);

            result = TestCommon.RunAICLICommand("configure list", "--verbose");
            Assert.AreEqual(0, result.ExitCode);
            Assert.False(result.StdOut.Contains(guid));
        }

        /// <summary>
        /// Applies a configuration (to ensure at least one exists), gets the overall list, then the outputs the first configuration.
        /// </summary>
        [Test]
        public void OutputConfiguration()
        {
            var result = TestCommon.RunAICLICommand(ConfigureWithAgreementsAndVerbose, TestCommon.GetTestDataFile("Configuration\\Configure_TestRepo.yml"));
            Assert.AreEqual(0, result.ExitCode);

            string guid = TestCommon.GetConfigurationInstanceIdentifierFor(ConfigureTestRepoFile);
            string tempFile = TestCommon.GetRandomTestFile(".yml");
            result = TestCommon.RunAICLICommand("configure list", $"-h {guid} --output {tempFile}");
            Assert.AreEqual(0, result.ExitCode);

            result = TestCommon.RunAICLICommand("configure validate", $"--verbose {tempFile}");
            Assert.AreEqual(Constants.ErrorCode.S_FALSE, result.ExitCode);
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
