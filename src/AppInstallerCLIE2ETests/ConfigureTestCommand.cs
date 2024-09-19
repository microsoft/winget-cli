// -----------------------------------------------------------------------------
// <copyright file="ConfigureTestCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests
{
    using System.IO;
    using AppInstallerCLIE2ETests.Helpers;
    using NUnit.Framework;

    /// <summary>
    /// `Configure test` command tests.
    /// </summary>
    public class ConfigureTestCommand
    {
        private const string CommandAndAgreements = "configure test --accept-configuration-agreements";

        /// <summary>
        /// Setup done once before all the tests here.
        /// </summary>
        [OneTimeSetUp]
        public void OneTimeSetup()
        {
            this.DeleteTxtFiles();
        }

        /// <summary>
        /// Teardown done once after all the tests here.
        /// </summary>
        [OneTimeTearDown]
        public void OneTimeTeardown()
        {
            this.DeleteTxtFiles();
        }

        /// <summary>
        /// Checks for a resource not in the desired state.
        /// </summary>
        [Test]
        public void ConfigureTest_NotInDesiredState()
        {
            TestCommon.EnsureModuleState(Constants.SimpleTestModuleName, present: false);
            this.DeleteTxtFiles();

            var result = TestCommon.RunAICLICommand(CommandAndAgreements, TestCommon.GetTestDataFile("Configuration\\Configure_TestRepo.yml"));
            Assert.AreEqual(Constants.ErrorCode.S_FALSE, result.ExitCode);
            Assert.True(result.StdOut.Contains("System is not in the described configuration state."));
        }

        /// <summary>
        /// Checks for a resource in a desired state.
        /// </summary>
        [Test]
        public void ConfigureTest_InDesiredState()
        {
            TestCommon.EnsureModuleState(Constants.SimpleTestModuleName, present: false);
            this.DeleteTxtFiles();

            // Set up the expected state
            File.WriteAllText(TestCommon.GetTestDataFile("Configuration\\Configure_TestRepo.txt"), "Contents!");

            var result = TestCommon.RunAICLICommand(CommandAndAgreements, TestCommon.GetTestDataFile("Configuration\\Configure_TestRepo.yml"));
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("System is in the described configuration state."));
        }

        /// <summary>
        /// One resource fails.
        /// </summary>
        [Test]
        public void ConfigureTest_TestFailure()
        {
            var result = TestCommon.RunAICLICommand(CommandAndAgreements, TestCommon.GetTestDataFile("Configuration\\IndependentResources_OneFailure.yml"));
            Assert.AreEqual(Constants.ErrorCode.CONFIG_ERROR_TEST_FAILED, result.ExitCode);
            Assert.True(result.StdOut.Contains("Some of the configuration units failed while testing their state."));
            Assert.True(result.StdOut.Contains("System is not in the described configuration state."));
        }

        /// <summary>
        /// Test from https configuration file.
        /// </summary>
        [Test]
        public void ConfigureTest_HttpsConfigurationFile()
        {
            var result = TestCommon.RunAICLICommand(CommandAndAgreements, $"{Constants.TestSourceUrl}/TestData/Configuration/Configure_TestRepo_Location.yml");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("System is in the described configuration state."));
        }

        /// <summary>
        /// Runs a configuration, then tests it from history.
        /// </summary>
        [Test]
        public void TestFromHistory()
        {
            var result = TestCommon.RunAICLICommand("configure --accept-configuration-agreements --verbose", TestCommon.GetTestDataFile("Configuration\\Configure_TestRepo.yml"));
            Assert.AreEqual(0, result.ExitCode);

            // The configuration creates a file next to itself with the given contents
            string targetFilePath = TestCommon.GetTestDataFile("Configuration\\Configure_TestRepo.txt");
            FileAssert.Exists(targetFilePath);
            Assert.AreEqual("Contents!", File.ReadAllText(targetFilePath));

            string guid = TestCommon.GetConfigurationInstanceIdentifierFor("Configure_TestRepo.yml");
            result = TestCommon.RunAICLICommand(CommandAndAgreements, $"-h {guid}");
            Assert.AreEqual(0, result.ExitCode);

            File.WriteAllText(targetFilePath, "Changed contents!");

            result = TestCommon.RunAICLICommand(CommandAndAgreements, $"-h {guid}");
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
