// -----------------------------------------------------------------------------
// <copyright file="ConfigureShowCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests
{
    using System.IO;
    using AppInstallerCLIE2ETests.Helpers;
    using Microsoft.Win32;
    using NUnit.Framework;

    /// <summary>
    /// `Configure show` command tests.
    /// </summary>
    public class ConfigureShowCommand
    {
        /// <summary>
        /// Setup done once before all the tests here.
        /// </summary>
        [OneTimeSetUp]
        public void OneTimeSetup()
        {
            this.DeleteResourceArtifacts();
            ConfigureCommand.EnsureTestResourcePresence();
        }

        /// <summary>
        /// One time teardown.
        /// </summary>
        [OneTimeTearDown]
        public void OneTimeTearDown()
        {
            this.DeleteResourceArtifacts();
        }

        /// <summary>
        /// Simple test to confirm that a resource without a module specified can be discovered in the PSGallery.
        /// </summary>
        [Test]
        [Ignore("PS Gallery tests are unreliable.")]
        public void ShowDetailsFromGallery()
        {
            TestCommon.EnsureModuleState(Constants.GalleryTestModuleName, present: false);

            var result = TestCommon.RunAICLICommand("configure show", $"{TestCommon.GetTestDataFile("Configuration\\PSGallery_NoModule_NoSettings.yml")} --verbose", timeOut: 120000);
            Assert.AreEqual(0, result.ExitCode);
            Assert.True(result.StdOut.Contains(Constants.PSGalleryName));
        }

        /// <summary>
        /// Simple test to confirm that a resource with a module specified can be discovered in a local repository that doesn't support resource discovery.
        /// </summary>
        [Test]
        public void ShowDetailsFromTestRepo()
        {
            TestCommon.EnsureModuleState(Constants.SimpleTestModuleName, present: false);

            var result = TestCommon.RunAICLICommand("configure show", $"{TestCommon.GetTestDataFile("Configuration\\ShowDetails_TestRepo.yml")} --verbose");
            Assert.AreEqual(0, result.ExitCode);
            Assert.True(result.StdOut.Contains(Constants.TestRepoName));
        }

        /// <summary>
        /// Simple test to confirm that a resource that is already locally available shows that way.
        /// </summary>
        /// <param name="location">The location the module should be before running.</param>
        [TestCase(TestCommon.TestModuleLocation.CurrentUser)]
        [TestCase(TestCommon.TestModuleLocation.AllUsers)]
        [TestCase(TestCommon.TestModuleLocation.WinGetModulePath)]
        [TestCase(TestCommon.TestModuleLocation.Custom)]
        public void ShowDetailsFromLocal(TestCommon.TestModuleLocation location)
        {
            TestCommon.EnsureModuleState(Constants.SimpleTestModuleName, present: true, repository: Constants.TestRepoName, location: location);

            string args = $"{TestCommon.GetTestDataFile("Configuration\\ShowDetails_TestRepo.yml")} --verbose";
            if (location == TestCommon.TestModuleLocation.Custom)
            {
                args += " --module-path " + TestCommon.GetExpectedModulePath(location);
            }

            var result = TestCommon.RunAICLICommand("configure show", args);
            Assert.AreEqual(0, result.ExitCode);
            Assert.True(result.StdOut.Contains(Constants.LocalModuleDescriptor));
        }

        /// <summary>
        /// A schema 0.3 config file is allowed with the experimental feature.
        /// </summary>
        [Test]
        public void ShowDetails_Schema0_3_Succeeds()
        {
            TestCommon.EnsureModuleState(Constants.SimpleTestModuleName, present: false);

            var result = TestCommon.RunAICLICommand("configure show", $"{TestCommon.GetTestDataFile("Configuration\\ShowDetails_TestRepo_0_3.yml")} --verbose");
            Assert.AreEqual(0, result.ExitCode);
            Assert.True(result.StdOut.Contains(Constants.TestRepoName));
        }

        /// <summary>
        /// A schema 0.3 config file with parameters is blocked.
        /// </summary>
        [Test]
        public void ShowDetails_Schema0_3_Parameters()
        {
            var result = TestCommon.RunAICLICommand("configure show", TestCommon.GetTestDataFile("Configuration\\WithParameters_0_3.yml"));
            Assert.AreEqual(0, result.ExitCode);
            Assert.True(result.StdOut.Contains("Failed to get detailed information about the configuration."));
        }

        /// <summary>
        /// Simple test to show details from a https configuration file.
        /// </summary>
        [Test]
        public void ShowDetailsFromHttpsConfigurationFile()
        {
            var result = TestCommon.RunAICLICommand("configure show", $"{Constants.TestSourceUrl}/TestData/Configuration/ShowDetails_TestRepo.yml --verbose");
            Assert.AreEqual(0, result.ExitCode);
            Assert.True(result.StdOut.Contains(Constants.TestRepoName));
        }

        /// <summary>
        /// This test ensures that there is not significant overflow from large strings in the configuration file.
        /// </summary>
        [Test]
        public void ShowTruncatedDetailsAndFileContent()
        {
            var result = TestCommon.RunAICLICommand("configure show", $"{TestCommon.GetTestDataFile("Configuration\\LargeContentStrings.yml")} --verbose");
            Assert.AreEqual(0, result.ExitCode);
            Assert.True(result.StdOut.Contains("<this value has been truncated; inspect the file contents for the complete text>"));
            Assert.True(result.StdOut.Contains("Some of the data present in the configuration file was truncated for this output; inspect the file contents for the complete content."));
            Assert.False(result.StdOut.Contains("Line5"));
        }

        /// <summary>
        /// Runs a configuration, then shows it from history.
        /// </summary>
        [Test]
        public void ShowFromHistory()
        {
            var result = TestCommon.RunAICLICommand("configure --accept-configuration-agreements --verbose", TestCommon.GetTestDataFile("Configuration\\Configure_TestRepo.yml"));
            Assert.AreEqual(0, result.ExitCode);

            string guid = TestCommon.GetConfigurationInstanceIdentifierFor("Configure_TestRepo.yml");
            result = TestCommon.RunAICLICommand("configure show", $"-h {guid}");
            Assert.AreEqual(0, result.ExitCode);
        }

        /// <summary>
        /// Runs a configuration, then shows it from history.
        /// </summary>
        [Test]
        public void ShowWithBadProcessorIdentifier()
        {
            var result = TestCommon.RunAICLICommand("configure show", $"{TestCommon.GetTestDataFile("Configuration\\Unknown_Processor.yml")} --verbose");
            Assert.AreEqual(Constants.ErrorCode.CONFIG_ERROR_INVALID_FIELD_VALUE, result.ExitCode);
        }

        /// <summary>
        /// Simple test to confirm that a resource is discoverable with DSC v3.
        /// </summary>
        [Test]
        public void ShowDetails_DSCv3()
        {
            var result = TestCommon.RunAICLICommand("configure show", $"{TestCommon.GetTestDataFile("Configuration\\ShowDetails_DSCv3.yml")} --verbose");
            Assert.AreEqual(0, result.ExitCode);

            var outputLines = result.StdOut.Split('\n');
            int startLine = -1;
            for (int i = 0; i < outputLines.Length; ++i)
            {
                if (outputLines[i].Trim() == "Microsoft.WinGet.Dev/TestFile [Test File]")
                {
                    startLine = i;
                }
            }

            Assert.AreNotEqual(-1, startLine);
            Assert.LessOrEqual(3, outputLines.Length - startLine);

            // outputLines[1] should contain the discovered resource string if working properly.
            Assert.AreEqual("Description 1.", outputLines[startLine + 2].Trim());
        }

        /// <summary>
        /// Runs a DSCv3 configuration, then shows it from history.
        /// </summary>
        [Test]
        public void ShowFromHistory_DSCv3()
        {
            var result = TestCommon.RunAICLICommand("configure --accept-configuration-agreements --verbose", TestCommon.GetTestDataFile("Configuration\\ShowDetails_DSCv3.yml"));
            Assert.AreEqual(0, result.ExitCode);

            string guid = TestCommon.GetConfigurationInstanceIdentifierFor("ShowDetails_DSCv3.yml");
            result = TestCommon.RunAICLICommand("configure show", $"-h {guid} --");
            Assert.AreEqual(0, result.ExitCode);

            var outputLines = result.StdOut.Split('\n');
            int startLine = -1;
            for (int i = 0; i < outputLines.Length; ++i)
            {
                if (outputLines[i].Trim() == "Microsoft.WinGet.Dev/TestFile [Test File]")
                {
                    startLine = i;
                }
            }

            Assert.AreNotEqual(-1, startLine);
            Assert.LessOrEqual(3, outputLines.Length - startLine);

            // outputLines[1] should contain the discovered resource string if working properly.
            Assert.AreEqual("Description 1.", outputLines[startLine + 2].Trim());
        }

        private void DeleteResourceArtifacts()
        {
            // Delete all .txt files in the test directory; they are placed there by the tests
            foreach (string file in Directory.GetFiles(TestCommon.GetTestDataFile("Configuration"), "*.txt"))
            {
                File.Delete(file);
            }
        }
    }
}
