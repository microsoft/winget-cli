// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
{
    using NUnit.Framework;

    public class SourceCommand
    {
        // Todo: switch to use prod index when available
        private const string SourceTestSourceUrl = @"https://winget-int.azureedge.net/cache";
        private const string SourceTestSourceName = @"SourceTestSource";

        [TearDown]
        public void TearDown()
        {
            TestCommon.RunAICLICommand("source remove", SourceTestSourceName);

            TestCommon.WaitForDeploymentFinish();
        }

        [Test]
        public void SourceCommands()
        {
            // Add test source should succeed
            var result = TestCommon.RunAICLICommand("source add", $"{SourceTestSourceName} {SourceTestSourceUrl}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Done"));

            // Add source with duplicate name should fail
            result = TestCommon.RunAICLICommand("source add", $"{SourceTestSourceName} https://microsoft.com");
            Assert.AreEqual(Constants.ErrorCode.ERROR_SOURCE_NAME_ALREADY_EXISTS, result.ExitCode);
            Assert.True(result.StdOut.Contains("A source with the given name already exists and refers to a different location"));

            // Add source with invalid url should fail
            result = TestCommon.RunAICLICommand("source add", "AnotherSource https://microsoft.com");
            Assert.AreEqual(Constants.ErrorCode.ERROR_NO_RANGES_PROCESSED, result.ExitCode);
            Assert.True(result.StdOut.Contains("error occurred while executing the command"));

            // Add source with an HTTP url should fail
            result = TestCommon.RunAICLICommand("source add", "Insecure http://microsoft.com");
            Assert.AreEqual(Constants.ErrorCode.ERROR_SOURCE_NOT_SECURE, result.ExitCode);
            Assert.True(result.StdOut.Contains("error occurred while executing the command"));

            // List with no args should list all available sources
            result = TestCommon.RunAICLICommand("source list", "");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("https://winget-int.azureedge.net/cache"));

            // List when source name matches, it shows detailed info
            result = TestCommon.RunAICLICommand("source list", $"-n {SourceTestSourceName}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("SourceTestSource"));
            Assert.True(result.StdOut.Contains("https://winget-int.azureedge.net/cache"));
            Assert.True(result.StdOut.Contains("Microsoft.Winget.Source.int"));
            Assert.True(result.StdOut.Contains("Updated"));

            // List when source name does not match
            result = TestCommon.RunAICLICommand("source list", "-n UnknownName");
            Assert.AreEqual(Constants.ErrorCode.ERROR_SOURCE_NAME_DOES_NOT_EXIST, result.ExitCode);
            Assert.True(result.StdOut.Contains("Did not find a source named"));

            // Update should succeed
            result = TestCommon.RunAICLICommand("source update", $"-n {SourceTestSourceName}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Done"));

            // Update with bad name should fail
            result = TestCommon.RunAICLICommand("source update", "-n UnknownName");
            Assert.AreEqual(Constants.ErrorCode.ERROR_SOURCE_NAME_DOES_NOT_EXIST, result.ExitCode);
            Assert.True(result.StdOut.Contains("Did not find a source named: UnknownName"));

            // Remove with a bad name should fail
            result = TestCommon.RunAICLICommand("source remove", "-n UnknownName");
            Assert.AreEqual(Constants.ErrorCode.ERROR_SOURCE_NAME_DOES_NOT_EXIST, result.ExitCode);
            Assert.True(result.StdOut.Contains("Did not find a source named: UnknownName"));

            // Remove with a good name should succeed
            result = TestCommon.RunAICLICommand("source remove", $"-n {SourceTestSourceName}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Done"));
        }
    }
}