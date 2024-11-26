// -----------------------------------------------------------------------------
// <copyright file="ConfigureCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests
{
    using System.IO;
    using AppInstallerCLIE2ETests.Helpers;
    using NUnit.Framework;

    /// <summary>
    /// `Configure` command tests.
    /// </summary>
    public class ConfigureCommand
    {
        private const string CommandAndAgreementsAndVerbose = "configure --accept-configuration-agreements --verbose";

        /// <summary>
        /// Setup done once before all the tests here.
        /// </summary>
        [OneTimeSetUp]
        public void OneTimeSetup()
        {
            WinGetSettingsHelper.ConfigureFeature("configuration03", true);
            WinGetSettingsHelper.ConfigureFeature("configureSelfElevate", true);
            this.DeleteTxtFiles();
        }

        /// <summary>
        /// Teardown done once after all the tests here.
        /// </summary>
        [OneTimeTearDown]
        public void OneTimeTeardown()
        {
            WinGetSettingsHelper.ConfigureFeature("configuration03", false);
            WinGetSettingsHelper.ConfigureFeature("configureSelfElevate", false);
            this.DeleteTxtFiles();
        }

        /// <summary>
        /// Simple test to confirm that a resource without a module specified can be discovered in the PSGallery.
        /// Intentionally has no settings to force a failure, but only after acquiring the module.
        /// </summary>
        [Test]
        [Ignore("PS Gallery tests are unreliable.")]
        public void ConfigureFromGallery()
        {
            TestCommon.EnsureModuleState(Constants.GalleryTestModuleName, present: false);

            var result = TestCommon.RunAICLICommand(CommandAndAgreementsAndVerbose, TestCommon.GetTestDataFile("Configuration\\PSGallery_NoModule_NoSettings.yml"), timeOut: 120000);
            Assert.AreEqual(Constants.ErrorCode.CONFIG_ERROR_SET_APPLY_FAILED, result.ExitCode);
            Assert.True(result.StdOut.Contains("The configuration unit failed while attempting to test the current system state."));
        }

        /// <summary>
        /// Simple test to confirm that a resource with a module specified can be discovered in a local repository that doesn't support resource discovery.
        /// </summary>
        [Test]
        public void ConfigureFromTestRepo()
        {
            TestCommon.EnsureModuleState(Constants.SimpleTestModuleName, present: false);

            var result = TestCommon.RunAICLICommand(CommandAndAgreementsAndVerbose, TestCommon.GetTestDataFile("Configuration\\Configure_TestRepo.yml"));
            Assert.AreEqual(0, result.ExitCode);

            // The configuration creates a file next to itself with the given contents
            string targetFilePath = TestCommon.GetTestDataFile("Configuration\\Configure_TestRepo.txt");
            FileAssert.Exists(targetFilePath);
            Assert.AreEqual("Contents!", File.ReadAllText(targetFilePath));

            Assert.True(Directory.Exists(
                Path.Combine(
                    TestCommon.GetExpectedModulePath(TestCommon.TestModuleLocation.Default),
                    Constants.SimpleTestModuleName)));
        }

        /// <summary>
        /// Simple test to confirm that the module was installed to the location specified in the DefaultModuleRoot settings.
        /// </summary>
        [Test]
        public void ConfigureFromTestRepo_DefaultModuleRootSetting()
        {
            TestCommon.EnsureModuleState(Constants.SimpleTestModuleName, present: false);
            string moduleTestDir = TestCommon.GetRandomTestDir();
            WinGetSettingsHelper.ConfigureConfigureBehavior(Constants.DefaultModuleRoot, moduleTestDir);

            string args = TestCommon.GetTestDataFile("Configuration\\Configure_TestRepo_Location.yml");
            var result = TestCommon.RunAICLICommand(CommandAndAgreementsAndVerbose, args);

            WinGetSettingsHelper.ConfigureConfigureBehavior(Constants.DefaultModuleRoot, string.Empty);
            bool moduleExists = Directory.Exists(Path.Combine(moduleTestDir, Constants.SimpleTestModuleName));
            if (moduleExists)
            {
                // Clean test directory to avoid impacting other tests.
                Directory.Delete(moduleTestDir, true);
            }

            Assert.AreEqual(0, result.ExitCode);
            Assert.True(moduleExists);
        }

        /// <summary>
        /// Simple test to confirm that the module was installed in the right location.
        /// </summary>
        /// <param name="location">Location to pass.</param>
        [TestCase(TestCommon.TestModuleLocation.CurrentUser)]
        [TestCase(TestCommon.TestModuleLocation.AllUsers)]
        [TestCase(TestCommon.TestModuleLocation.WinGetModulePath)]
        [TestCase(TestCommon.TestModuleLocation.Custom)]
        [TestCase(TestCommon.TestModuleLocation.Default)]
        public void ConfigureFromTestRepo_Location(TestCommon.TestModuleLocation location)
        {
            TestCommon.EnsureModuleState(Constants.SimpleTestModuleName, present: false);

            string args = TestCommon.GetTestDataFile("Configuration\\Configure_TestRepo_Location.yml");
            if (location == TestCommon.TestModuleLocation.CurrentUser)
            {
                args += " --module-path currentuser";
            }
            else if (location == TestCommon.TestModuleLocation.AllUsers)
            {
                args += " --module-path allusers";
            }
            else if (location == TestCommon.TestModuleLocation.Default)
            {
                args += " --module-path default";
            }
            else if (location == TestCommon.TestModuleLocation.Custom)
            {
                args += " --module-path " + TestCommon.GetExpectedModulePath(location);
            }

            var result = TestCommon.RunAICLICommand(CommandAndAgreementsAndVerbose, args);
            Assert.AreEqual(0, result.ExitCode);

            Assert.True(Directory.Exists(Path.Combine(
                TestCommon.GetExpectedModulePath(location),
                Constants.SimpleTestModuleName)));
        }

        /// <summary>
        /// One resource fails, but the other is not dependent and should be executed.
        /// </summary>
        [Test]
        public void IndependentResourceWithSingleFailure()
        {
            var result = TestCommon.RunAICLICommand(CommandAndAgreementsAndVerbose, TestCommon.GetTestDataFile("Configuration\\IndependentResources_OneFailure.yml"));
            Assert.AreEqual(Constants.ErrorCode.CONFIG_ERROR_SET_APPLY_FAILED, result.ExitCode);

            // The configuration creates a file next to itself with the given contents
            string targetFilePath = TestCommon.GetTestDataFile("Configuration\\IndependentResources_OneFailure.txt");
            FileAssert.Exists(targetFilePath);
            Assert.AreEqual("Contents!", File.ReadAllText(targetFilePath));
        }

        /// <summary>
        /// One resource fails, and the dependent resource should not be executed.
        /// </summary>
        [Test]
        public void DependentResourceWithFailure()
        {
            var result = TestCommon.RunAICLICommand(CommandAndAgreementsAndVerbose, TestCommon.GetTestDataFile("Configuration\\DependentResources_Failure.yml"));
            Assert.AreEqual(Constants.ErrorCode.CONFIG_ERROR_SET_APPLY_FAILED, result.ExitCode);

            // The configuration creates a file next to itself with the given contents
            string targetFilePath = TestCommon.GetTestDataFile("Configuration\\DependentResources_Failure.txt");
            FileAssert.DoesNotExist(targetFilePath);
        }

        /// <summary>
        /// The configuration server unexpectedly exits. Winget should continue to operate properly.
        /// </summary>
        [Test]
        public void ConfigServerUnexpectedExit()
        {
            var result = TestCommon.RunAICLICommand(CommandAndAgreementsAndVerbose, TestCommon.GetTestDataFile("Configuration\\ConfigServerUnexpectedExit.yml"));
            Assert.AreEqual(Constants.ErrorCode.CONFIG_ERROR_SET_APPLY_FAILED, result.ExitCode);

            // The configuration creates a file next to itself with the given contents
            string targetFilePath = TestCommon.GetTestDataFile("Configuration\\ConfigServerUnexpectedExit.txt");
            FileAssert.DoesNotExist(targetFilePath);
        }

        /// <summary>
        /// Resource name case-insensitive test.
        /// </summary>
        [Test]
        public void ResourceCaseInsensitive()
        {
            TestCommon.EnsureModuleState(Constants.SimpleTestModuleName, present: false);

            var result = TestCommon.RunAICLICommand(CommandAndAgreementsAndVerbose, TestCommon.GetTestDataFile("Configuration\\ResourceCaseInsensitive.yml"));
            Assert.AreEqual(0, result.ExitCode);

            // The configuration creates a file next to itself with the given contents
            string targetFilePath = TestCommon.GetTestDataFile("Configuration\\ResourceCaseInsensitive.txt");
            FileAssert.Exists(targetFilePath);
            Assert.AreEqual("Contents!", File.ReadAllText(targetFilePath));
        }

        /// <summary>
        /// Simple test to configure from an https configuration file.
        /// </summary>
        [Test]
        public void ConfigureFromHttpsConfigurationFile()
        {
            string args = $"{Constants.TestSourceUrl}/TestData/Configuration/Configure_TestRepo_Location.yml";

            var result = TestCommon.RunAICLICommand(CommandAndAgreementsAndVerbose, args);
            Assert.AreEqual(0, result.ExitCode);
        }

        /// <summary>
        /// Runs a configuration, then changes the state and runs it again from history.
        /// </summary>
        [Test]
        public void ConfigureFromHistory()
        {
            var result = TestCommon.RunAICLICommand(CommandAndAgreementsAndVerbose, TestCommon.GetTestDataFile("Configuration\\Configure_TestRepo.yml"));
            Assert.AreEqual(0, result.ExitCode);

            // The configuration creates a file next to itself with the given contents
            string targetFilePath = TestCommon.GetTestDataFile("Configuration\\Configure_TestRepo.txt");
            FileAssert.Exists(targetFilePath);
            Assert.AreEqual("Contents!", File.ReadAllText(targetFilePath));

            File.WriteAllText(targetFilePath, "Changed contents!");

            string guid = TestCommon.GetConfigurationInstanceIdentifierFor("Configure_TestRepo.yml");
            result = TestCommon.RunAICLICommand(CommandAndAgreementsAndVerbose, $"-h {guid}");
            Assert.AreEqual(0, result.ExitCode);

            FileAssert.Exists(targetFilePath);
            Assert.AreEqual("Contents!", File.ReadAllText(targetFilePath));
        }

        /// <summary>
        /// Specifies the module path to an "elevated" server.
        /// </summary>
        [Test]
        public void SpecifyModulePathToHighIntegrityServer()
        {
            string configFile = TestCommon.GetTestDataFile("Configuration\\GetPSModulePath.yml");
            string testDirectory = TestCommon.GetRandomTestDir();

            var result = TestCommon.RunAICLICommand(CommandAndAgreementsAndVerbose, $"{configFile} --module-path \"{testDirectory}\"");
            Assert.AreEqual(0, result.ExitCode);

            string testFile = Path.Join(TestCommon.GetTestDataFile("Configuration"), "PSModulePath.txt");
            Assert.True(File.Exists(testFile));
            string testFileContents = File.ReadAllText(testFile);
            Assert.True(testFileContents.StartsWith(testDirectory));
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
