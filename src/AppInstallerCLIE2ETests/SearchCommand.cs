// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
{
    using NUnit.Framework;

    public class SearchCommand : BaseCommand
    {
        [Test]
        public void SearchWithoutArgs()
        {
            var result = TestCommon.RunAICLICommand("search", "");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("AppInstallerTest.TestExeInstaller"));
            Assert.True(result.StdOut.Contains("AppInstallerTest.TestBurnInstaller"));
            Assert.True(result.StdOut.Contains("AppInstallerTest.TestExampleInstaller"));
        }

        [Test]
        public void SearchQuery()
        {
            var result = TestCommon.RunAICLICommand("search", "TestExampleInstaller");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("TestExampleInstaller"));
            Assert.True(result.StdOut.Contains("AppInstallerTest.TestExampleInstaller"));
        }

        [Test]
        public void SearchWithName()
        {
            var result = TestCommon.RunAICLICommand("search", "--name testexampleinstaller");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("TestExampleInstaller"));
            Assert.True(result.StdOut.Contains("AppInstallerTest.TestExampleInstaller"));
        }

        [Test]
        public void SearchWithID()
        {
            var result = TestCommon.RunAICLICommand("search", "--id appinstallertest.testexampleinstaller");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("TestExampleInstaller"));
            Assert.True(result.StdOut.Contains("AppInstallerTest.TestExampleInstaller"));
        }

        [Test]
        public void SearchWithInvalidName()
        {
            var result = TestCommon.RunAICLICommand("search", "--name InvalidName");
            Assert.AreEqual(Constants.ErrorCode.ERROR_NO_APPLICATIONS_FOUND, result.ExitCode);
            Assert.True(result.StdOut.Contains("No package found matching input criteria."));
        }

        [Test]
        public void SearchReturnsMultiple()
        {
            // Search Microsoft should return multiple
            var result = TestCommon.RunAICLICommand("search", "AppInstallerTest");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("AppInstallerTest.TestExeInstaller"));
            Assert.True(result.StdOut.Contains("AppInstallerTest.TestBurnInstaller"));
            Assert.True(result.StdOut.Contains("AppInstallerTest.TestExampleInstaller"));
        }

        [Test]
        public void SearchWithExactName()
        {
            var result = TestCommon.RunAICLICommand("search", "--exact TestExampleInstaller");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("TestExampleInstaller"));
            Assert.True(result.StdOut.Contains("AppInstallerTest.TestExampleInstaller"));
        }

        [Test]
        public void SearchWithExactID()
        {
            var result = TestCommon.RunAICLICommand("search", "--exact AppInstallerTest.TestExampleInstaller");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("TestExampleInstaller"));
            Assert.True(result.StdOut.Contains("AppInstallerTest.TestExampleInstaller"));
        }

        [Test]
        public void SearchWithExactArgCaseSensitivity()
        {
            var result = TestCommon.RunAICLICommand("search", "--exact testexampleinstaller");
            Assert.AreEqual(Constants.ErrorCode.ERROR_NO_APPLICATIONS_FOUND, result.ExitCode);
            Assert.True(result.StdOut.Contains("No package found matching input criteria."));
        }
    }
}