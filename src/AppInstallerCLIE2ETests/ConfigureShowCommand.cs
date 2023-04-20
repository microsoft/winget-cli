// -----------------------------------------------------------------------------
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
    public class ConfigureShowCommand : BaseCommand
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
        /// Simple smoke test to ensure that showing details is working.
        /// </summary>
        [Test]
        public void ShowDetailsFromGallery()
        {
            var result = TestCommon.RunAICLICommand("configure show", TestCommon.GetTestDataFile("Configuration\\ShowDetails.yml"));
            TestContext.Out.Write(result.StdOut);
            Assert.AreEqual(0, result.ExitCode);
        }
    }
}
