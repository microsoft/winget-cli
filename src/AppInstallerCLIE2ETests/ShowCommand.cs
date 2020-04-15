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
            // There's a deployment bug that if the last optional package is removed, the main package will also be removed.
            Assert.True(TestCommon.InstallMsix(TestCommon.GetTestFile(Constants.AICLIPackageFile)));

            Assert.AreEqual(Constants.ErrorCode.S_OK, TestCommon.RunAICLICommand("source", $"add {Constants.TestSourceName} {Constants.TestSourceUrl}").ExitCode);
        }

        [TearDown]
        public void TearDown()
        {
            TestCommon.RunAICLICommand("source", $"remove {Constants.TestSourceName}");

            // There's a deployment bug that if the last optional package is removed, the main package will also be removed.
            Assert.True(TestCommon.InstallMsix(TestCommon.GetTestFile(Constants.AICLIPackageFile)));
        }

        [Test]
        public void ShowCommands()
        {
            // Show with no arg lists every app and a warning message
            var result = TestCommon.RunAICLICommand("show", "");
            Assert.AreEqual(Constants.ErrorCode.ERROR_MULTIPLE_APPLICATIONS_FOUND, result.ExitCode);
            Assert.True(result.StdOut.Contains("Multiple apps found matching input criteria. Please refine the input."));
            Assert.True(result.StdOut.Contains("Microsoft.PowerToys"));
            Assert.True(result.StdOut.Contains("Microsoft.VisualStudioCode"));

            // Show with multiple search matches shows a "please refine input"
            result = TestCommon.RunAICLICommand("show", "Microsoft");
            Assert.AreEqual(Constants.ErrorCode.ERROR_MULTIPLE_APPLICATIONS_FOUND, result.ExitCode);
            Assert.True(result.StdOut.Contains("Multiple apps found matching input criteria. Please refine the input."));
            Assert.True(result.StdOut.Contains("Microsoft.PowerToys"));
            Assert.True(result.StdOut.Contains("Microsoft.VisualStudioCode"));

            // Show with 0 search match shows a "please refine input"
            result = TestCommon.RunAICLICommand("show", "DoesNotExist");
            Assert.AreEqual(Constants.ErrorCode.ERROR_NO_APPLICATIONS_FOUND, result.ExitCode);
            Assert.True(result.StdOut.Contains("No app found matching input criteria."));

            // Show with 1 search match shows detailed manifest info
            result = TestCommon.RunAICLICommand("show", "VisualStudioCode");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Id: Microsoft.VisualStudioCode"));
            Assert.True(result.StdOut.Contains("Name: Visual Studio Code"));

            // Show with --versions list the versions
            result = TestCommon.RunAICLICommand("show", "VisualStudioCode --versions");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Microsoft.VisualStudioCode, Visual Studio Code"));
            Assert.True(result.StdOut.Contains("1.41.1"));
        }
    }
}