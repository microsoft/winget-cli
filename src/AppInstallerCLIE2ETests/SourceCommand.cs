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
        public void AddSource()
        {
            // Add test source should succeed
            var result = TestCommon.RunAICLICommand("source add", $"{SourceTestSourceName} {SourceTestSourceUrl}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Done"));
        }

        [Test]
        public void AddSourceWithDuplicateName()
        {
            // Add source with duplicate name should fail
            var result = TestCommon.RunAICLICommand("source add", $"{SourceTestSourceName} https://microsoft.com");
            Assert.AreEqual(Constants.ErrorCode.ERROR_SOURCE_NAME_ALREADY_EXISTS, result.ExitCode);
            Assert.True(result.StdOut.Contains("A source with the given name already exists and refers to a different location"));
        }

        [Test]
        public void AddSourceWithInvalidURL()
        {
            // Add source with invalid url should fail
            var result = TestCommon.RunAICLICommand("source add", "AnotherSource https://microsoft.com");
            Assert.AreEqual(Constants.ErrorCode.ERROR_NO_RANGES_PROCESSED, result.ExitCode);
            Assert.True(result.StdOut.Contains("error occurred while executing the command"));
        }


        [Test]
        public void AddSourceWithHttpURL()
        {
            // Add source with an HTTP url should fail
            var result = TestCommon.RunAICLICommand("source add", "Insecure http://microsoft.com");
            Assert.AreEqual(Constants.ErrorCode.ERROR_SOURCE_NOT_SECURE, result.ExitCode);
            Assert.True(result.StdOut.Contains("error occurred while executing the command"));
        }

        [Test]
        public void ListWithNoArgs()
        {
            // List with no args should list all available sources
            var result = TestCommon.RunAICLICommand("source list", "");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("https://winget-int.azureedge.net/cache"));
        }

        [Test]
        public void ListWithSourceName()
        {
            // List when source name matches, it shows detailed info
            var result = TestCommon.RunAICLICommand("source list", $"-n {SourceTestSourceName}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("SourceTestSource"));
            Assert.True(result.StdOut.Contains("https://winget-int.azureedge.net/cache"));
            Assert.True(result.StdOut.Contains("Microsoft.Winget.Source.int"));
            Assert.True(result.StdOut.Contains("Updated"));
        }

        [Test]
        public void ListSourceNameMismatch()
        {
            // List when source name does not match
            var result = TestCommon.RunAICLICommand("source list", "-n UnknownName");
            Assert.AreEqual(Constants.ErrorCode.ERROR_SOURCE_NAME_DOES_NOT_EXIST, result.ExitCode);
            Assert.True(result.StdOut.Contains("Did not find a source named"));
        }

        [Test]
        public void SourceUpdate()
        {
            // Update should succeed
            var result = TestCommon.RunAICLICommand("source update", $"-n {SourceTestSourceName}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Done"));
        }

        [Test]
        public void SourceUpdateWithInvalidName()
        {
            // Update with bad name should fail
            var result = TestCommon.RunAICLICommand("source update", "-n UnknownName");
            Assert.AreEqual(Constants.ErrorCode.ERROR_SOURCE_NAME_DOES_NOT_EXIST, result.ExitCode);
            Assert.True(result.StdOut.Contains("Did not find a source named: UnknownName"));
        }

        [Test]
        public void SourceRemoveInvalidName()
        {
            // Remove with a bad name should fail
            var result = TestCommon.RunAICLICommand("source remove", "-n UnknownName");
            Assert.AreEqual(Constants.ErrorCode.ERROR_SOURCE_NAME_DOES_NOT_EXIST, result.ExitCode);
            Assert.True(result.StdOut.Contains("Did not find a source named: UnknownName"));
        }

        [Test]
        public void SourceRemoveValidName()
        {
            // Remove with a good name should succeed
            var result = TestCommon.RunAICLICommand("source remove", $"-n {SourceTestSourceName}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Done"));
        }
    }
}