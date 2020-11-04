// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
{
    using System;
    using System.IO;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Linq;
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
            Assert.True(result.StdOut.Contains("Command Sample"));
            Assert.True(result.StdOut.Contains("Argument Sample"));
            Assert.True(result.StdOut.Contains("Microsoft Store Support"));
            Assert.False(result.StdOut.Contains("Enabled"));
        }

        [Test]
        public void EnableExperimentalFeatures()
        {
            ConfigureFeature("experimentalArg", true);
            ConfigureFeature("experimentalCmd", true);
            ConfigureFeature("experimentalMSStore", true);
            var result = TestCommon.RunAICLICommand("features", "");
            Assert.True(result.StdOut.Contains("Enabled"));
        }
    }
}
