// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
{
    using NUnit.Framework;

    public class SearchCommand
    {
        [SetUp]
        public void Setup()
        {
            Assert.AreEqual(TestCommon.RunAICLICommand("source", $"add {Constants.TestSourceName} {Constants.TestSourceUrl}").ExitCode, 0);
        }

        [TearDown]
        public void TearDown()
        {
            TestCommon.RunAICLICommand("source", $"remove {Constants.TestSourceName}");
        }

        [Test]
        public void SearchCommands()
        {
            // Search without args list every app
            var result = TestCommon.RunAICLICommand("search", "");
            Assert.AreEqual(result.ExitCode, Constants.ErrorCode.S_OK);
            Assert.True(result.StdOut.Contains("Microsoft.PowerToys"));
            Assert.True(result.StdOut.Contains("Microsoft.VisualStudioCode"));

            // Search query
            result = TestCommon.RunAICLICommand("search", "VisualStudioCode");
            Assert.AreEqual(result.ExitCode, Constants.ErrorCode.S_OK);
            Assert.True(result.StdOut.Contains("Microsoft.VisualStudioCode"));

            // Search through id found the app
            result = TestCommon.RunAICLICommand("search", "--id VisualStudioCode");
            Assert.AreEqual(result.ExitCode, Constants.ErrorCode.S_OK);
            Assert.True(result.StdOut.Contains("Microsoft.VisualStudioCode"));

            // Search through name. No app found because name is "Visual Studio Code"
            result = TestCommon.RunAICLICommand("search", "--name VisualStudioCode");
            Assert.AreEqual(result.ExitCode, Constants.ErrorCode.ERROR_NO_APPLICATIONS_FOUND);
            Assert.True(result.StdOut.Contains("No app found matching input criteria."));

            // Search Microsoft should return multiple
            result = TestCommon.RunAICLICommand("search", "Microsoft");
            Assert.AreEqual(result.ExitCode, Constants.ErrorCode.S_OK);
            Assert.True(result.StdOut.Contains("Microsoft.PowerToys"));
            Assert.True(result.StdOut.Contains("Microsoft.VisualStudioCode"));

            // Search Microsoft with exact arg should return none
            result = TestCommon.RunAICLICommand("search", "Microsoft -e");
            Assert.AreEqual(result.ExitCode, Constants.ErrorCode.ERROR_NO_APPLICATIONS_FOUND);
            Assert.True(result.StdOut.Contains("No app found matching input criteria."));
        }
    }
}