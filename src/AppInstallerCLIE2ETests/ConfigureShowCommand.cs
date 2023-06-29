﻿// -----------------------------------------------------------------------------
// <copyright file="ConfigureShowCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests
{
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
            WinGetSettingsHelper.ConfigureFeature("configuration", true);
        }

        /// <summary>
        /// Simple test to confirm that a resource without a module specified can be discovered in the PSGallery.
        /// </summary>
        [Test]
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
        [Test]
        public void ShowDetailsFromLocal()
        {
            TestCommon.EnsureModuleState(Constants.SimpleTestModuleName, present: true, repository: Constants.TestRepoName);

            var result = TestCommon.RunAICLICommand("configure show", $"{TestCommon.GetTestDataFile("Configuration\\ShowDetails_TestRepo.yml")} --verbose");
            Assert.AreEqual(0, result.ExitCode);
            Assert.True(result.StdOut.Contains(Constants.LocalModuleDescriptor));
        }
    }
}
