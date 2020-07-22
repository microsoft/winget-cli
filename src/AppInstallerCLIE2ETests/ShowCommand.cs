// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
{
    using NUnit.Framework;
    using System.Threading;

    public class ShowCommand
    {
        // Todo: use created test source when available
        private const string ShowTestSourceUrl = @"https://winget-int.azureedge.net/cache";
        private const string ShowTestSourceName = @"ShowTestSource";

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
        public void ShowCommands()
        {
            // Show with no arg lists every app and a warning message
            var result = TestCommon.RunAICLICommand("show", $"-s {ShowTestSourceName}");
            Assert.AreEqual(Constants.ErrorCode.ERROR_MULTIPLE_APPLICATIONS_FOUND, result.ExitCode);
            Assert.True(result.StdOut.Contains("Multiple packages found matching input criteria. Please refine the input."));
            Assert.True(result.StdOut.Contains("Microsoft.PowerToys"));
            Assert.True(result.StdOut.Contains("Microsoft.VisualStudioCode"));

            // Show with 0 search match shows a "please refine input"
            result = TestCommon.RunAICLICommand("show", $"DoesNotExist -s {ShowTestSourceName}");
            Assert.AreEqual(Constants.ErrorCode.ERROR_NO_APPLICATIONS_FOUND, result.ExitCode);
            Assert.True(result.StdOut.Contains("No package found matching input criteria."));

            // Show with a substring match still returns 0 results
            result = TestCommon.RunAICLICommand("show", $"Microsoft -s {ShowTestSourceName}");
            Assert.AreEqual(Constants.ErrorCode.ERROR_NO_APPLICATIONS_FOUND, result.ExitCode);
            Assert.True(result.StdOut.Contains("No package found matching input criteria."));

            // Show with 1 search match shows detailed manifest info
            result = TestCommon.RunAICLICommand("show", $"Microsoft.VisualStudioCode -s {ShowTestSourceName}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Microsoft.VisualStudioCode"));
            Assert.True(result.StdOut.Contains("Visual Studio Code"));

            // Show with --versions list the versions
            result = TestCommon.RunAICLICommand("show", $"Microsoft.VisualStudioCode --versions -s {ShowTestSourceName}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Microsoft.VisualStudioCode"));
            Assert.True(result.StdOut.Contains("1.41.1"));
        }
    }
}