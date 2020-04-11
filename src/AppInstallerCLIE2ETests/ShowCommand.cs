// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
{
    using NUnit.Framework;

    public class ShowCommand
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
        public void ShowCommands()
        {
            // Show with no arg lists every app and a warning message
            var result = TestCommon.RunAICLICommand("show", "");
            Assert.AreEqual(result.ExitCode, Constants.ErrorCode.ERROR_MULTIPLE_APPLICATIONS_FOUND);
            Assert.True(result.StdOut.Contains("Multiple apps found matching input criteria. Please refine the input."));
            Assert.True(result.StdOut.Contains("Microsoft.PowerToys"));
            Assert.True(result.StdOut.Contains("Microsoft.VisualStudioCode"));

            // Show with multiple search matches shows a "please refine input"
            result = TestCommon.RunAICLICommand("show", "Microsoft");
            Assert.AreEqual(result.ExitCode, Constants.ErrorCode.ERROR_MULTIPLE_APPLICATIONS_FOUND);
            Assert.True(result.StdOut.Contains("Multiple apps found matching input criteria. Please refine the input."));
            Assert.True(result.StdOut.Contains("Microsoft.PowerToys"));
            Assert.True(result.StdOut.Contains("Microsoft.VisualStudioCode"));

            // Show with 0 search match shows a "please refine input"
            result = TestCommon.RunAICLICommand("show", "DoesNotExist");
            Assert.AreEqual(result.ExitCode, Constants.ErrorCode.ERROR_NO_APPLICATIONS_FOUND);
            Assert.True(result.StdOut.Contains("No app found matching input criteria."));

            // Show with 1 search match shows detailed manifest info
            result = TestCommon.RunAICLICommand("show", "VisualStudioCode");
            Assert.AreEqual(result.ExitCode, Constants.ErrorCode.S_OK);
            Assert.True(result.StdOut.Contains("Id: Microsoft.VisualStudioCode"));
            Assert.True(result.StdOut.Contains("Name: Visual Studio Code"));

            // Show with --versions list the versions
            result = TestCommon.RunAICLICommand("show", "VisualStudioCode --versions");
            Assert.AreEqual(result.ExitCode, Constants.ErrorCode.S_OK);
            Assert.True(result.StdOut.Contains("Microsoft.VisualStudioCode, Visual Studio Code"));
            Assert.True(result.StdOut.Contains("1.41.1"));
        }
    }
}