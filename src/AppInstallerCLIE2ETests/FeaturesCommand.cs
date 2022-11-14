// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
{
    using NUnit.Framework;

    public class FeaturesCommand : BaseCommand
    {
        [SetUp]
        public void Setup()
        {
            InitializeAllFeatures(false);
        }

        [TearDown]
        public void TearDown()
        {
            InitializeAllFeatures(false);
        }

        [Test]
        public void DisplayFeatures()
        {
            var result = TestCommon.RunAICLICommand("features", "");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Direct MSI Installation"));
            Assert.False(result.StdOut.Contains("Enabled"));
        }

        [Test]
        public void EnableExperimentalFeatures()
        {
            ConfigureFeature("experimentalArg", true);
            ConfigureFeature("experimentalCmd", true);
            ConfigureFeature("directMSI", true);
            ConfigureFeature("openLogsArgument", true);
            var result = TestCommon.RunAICLICommand("features", "");
            Assert.True(result.StdOut.Contains("Enabled"));
        }
    }
}
