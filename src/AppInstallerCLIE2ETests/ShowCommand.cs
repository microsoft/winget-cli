// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
{
    using NUnit.Framework;

    public class ShowCommand
    {
        private const string ShowTestSourceName = @"ShowTestSource";
        private const string ShowTestSourceUrl = @"https://localhost:5001/TestKit";


        [SetUp]
        public void Setup()
        {
            Assert.AreEqual(Constants.ErrorCode.S_OK, TestCommon.RunAICLICommand("source add", $"{ShowTestSourceName} {ShowTestSourceUrl}").ExitCode);
        }

        [TearDown]
        public void TearDown()
        {
            TestCommon.RunAICLICommand("source remove", ShowTestSourceName);

            TestCommon.WaitForDeploymentFinish();
        }

        [Test]
        public void ShowWithNoArgs()
        {
            // Show with no arg lists every app and a warning message
            var result = TestCommon.RunAICLICommand("show", $"-s {ShowTestSourceName}");
            Assert.AreEqual(Constants.ErrorCode.ERROR_MULTIPLE_APPLICATIONS_FOUND, result.ExitCode);
            Assert.True(result.StdOut.Contains("Multiple packages found matching input criteria. Please refine the input."));
            Assert.True(result.StdOut.Contains("AppInstallerTest.TestExeInstaller"));
            Assert.True(result.StdOut.Contains("AppInstallerTest.TestBurnInstaller"));
            Assert.True(result.StdOut.Contains("AppInstallerTest.TestExampleInstaller"));
        }

        [Test]
        public void ShowWithNoMatches()
        {
            // Show with 0 search match shows a "please refine input"
            var result = TestCommon.RunAICLICommand("show", $"DoesNotExist -s {ShowTestSourceName}");
            Assert.AreEqual(Constants.ErrorCode.ERROR_NO_APPLICATIONS_FOUND, result.ExitCode);
            Assert.True(result.StdOut.Contains("No package found matching input criteria."));
        }

        [Test]
        public void ShowWithSubstringMatch()
        {
            // Show with a substring match still returns 0 results
            var result = TestCommon.RunAICLICommand("show", $"AppInstallerTest -s {ShowTestSourceName}");
            Assert.AreEqual(Constants.ErrorCode.ERROR_NO_APPLICATIONS_FOUND, result.ExitCode);
            Assert.True(result.StdOut.Contains("No package found matching input criteria."));
        }

        [Test]
        public void ShowWithNameMatch()
        {
            var result = TestCommon.RunAICLICommand("show", $"--name TestExampleInstaller -s {ShowTestSourceName}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Found TestExampleInstaller [AppInstallerTest.TestExampleInstaller]"));
            Assert.True(result.StdOut.Contains("TestExampleInstaller"));
            Assert.True(result.StdOut.Contains("AppInstallerTest.TestExampleInstaller"));
        }

        [Test]
        public void ShowWithIDMatch()
        { 
            var result = TestCommon.RunAICLICommand("show", $"--id AppInstallerTest.TestExampleInstaller -s {ShowTestSourceName}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Found TestExampleInstaller [AppInstallerTest.TestExampleInstaller]"));
            Assert.True(result.StdOut.Contains("TestExampleInstaller"));
            Assert.True(result.StdOut.Contains("AppInstallerTest.TestExampleInstaller"));
        }

        [Test]
        public void ShowWithVersions()
        {
            // Show with --versions list the versions
            var result = TestCommon.RunAICLICommand("show", $"TestExampleInstaller --versions -s {ShowTestSourceName}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("TestExampleInstaller"));
            Assert.True(result.StdOut.Contains("AppInstallerTest.TestExampleInstaller"));
            Assert.True(result.StdOut.Contains("1.2.3.4"));
        }

        [Test]
        public void ShowWithExactArg()
        {
            // Show 'testexampleinstaller' with exact arg should return none due to case sensitivity
            var result = TestCommon.RunAICLICommand("show", $"--exact testexampleinstaller -s {ShowTestSourceName}");
            Assert.AreEqual(Constants.ErrorCode.ERROR_NO_APPLICATIONS_FOUND, result.ExitCode);
            Assert.True(result.StdOut.Contains("No package found matching input criteria."));
        }
    }
}