// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
{
    using NUnit.Framework;
    using System.Threading;

    public class SearchCommand
    {
        // Todo: use created test source when available
        private const string SearchTestSourceUrl = @"https://winget-int.azureedge.net/cache";
        private const string SearchTestSourceName = @"SearchTestSource";

        [SetUp]
        public void Setup()
        {
            Assert.AreEqual(Constants.ErrorCode.S_OK, TestCommon.RunAICLICommand("source add", $"{SearchTestSourceName} {SearchTestSourceUrl}").ExitCode);
        }

        [TearDown]
        public void TearDown()
        {
            TestCommon.RunAICLICommand("source remove", SearchTestSourceName);

            TestCommon.WaitForDeploymentFinish();
        }

        [Test]
        public void SearchWithoutArgs()
        {
            // Search without args list every app
            var result = TestCommon.RunAICLICommand("search", "");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Microsoft.PowerToys"));
            Assert.True(result.StdOut.Contains("Microsoft.VisualStudioCode"));
        }

        [Test]
        public void SearchQuery()
        {
            // Search query
            var result = TestCommon.RunAICLICommand("search", "VisualStudioCode");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Microsoft.VisualStudioCode"));
        }

        [Test]
        public void SearchWithID()
        {
            // Search through id found the app
            var result = TestCommon.RunAICLICommand("search", "--id VisualStudioCode");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Microsoft.VisualStudioCode"));
        }

        [Test]
        public void SearchWithInvalidName()
        {
            // Search through name. No app found because name is "Visual Studio Code"
            var result = TestCommon.RunAICLICommand("search", "--name VisualStudioCode");
            Assert.AreEqual(Constants.ErrorCode.ERROR_NO_APPLICATIONS_FOUND, result.ExitCode);
            Assert.True(result.StdOut.Contains("No package found matching input criteria."));
        }

        [Test]
        public void SearchReturnsMultiple()
        {
            // Search Microsoft should return multiple
            var result = TestCommon.RunAICLICommand("search", "Microsoft");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Microsoft.PowerToys"));
            Assert.True(result.StdOut.Contains("Microsoft.VisualStudioCode"));
        }

        [Test]
        public void SearchWithExactArg()
        {
            // Search 'powertoys' with exact arg should return none due to case sensitivity
            var result = TestCommon.RunAICLICommand("search", "powertoys -e");
            Assert.AreEqual(Constants.ErrorCode.ERROR_NO_APPLICATIONS_FOUND, result.ExitCode);
            Assert.True(result.StdOut.Contains("No package found matching input criteria."));
        }
    }
}