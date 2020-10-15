// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
{
    using NUnit.Framework;

    public class SourceCommand : BaseCommand
    {
        [Test]
        public void SourceAdd()
        {
            var result = TestCommon.RunAICLICommand("source add", $"SourceTest {Constants.TestSourceUrl}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Done"));
            TestCommon.RunAICLICommand("source remove", $"-n SourceTest");
        }

        [Test]
        public void SourceAddWithDuplicateName()
        {
            // Add source with duplicate name should fail
            var result = TestCommon.RunAICLICommand("source add", $"{Constants.TestSourceName} https://microsoft.com");
            Assert.AreEqual(Constants.ErrorCode.ERROR_SOURCE_NAME_ALREADY_EXISTS, result.ExitCode);
            Assert.True(result.StdOut.Contains("A source with the given name already exists and refers to a different location"));
        }

        [Test]
        public void SourceAddWithInvalidURL()
        {
            // Add source with invalid url should fail
            var result = TestCommon.RunAICLICommand("source add", "AnotherSource https://microsoft.com");
            Assert.AreEqual(Constants.ErrorCode.ERROR_NO_RANGES_PROCESSED, result.ExitCode);
            Assert.True(result.StdOut.Contains("An unexpected error occurred while executing the command"));
        }


        [Test]
        public void SourceAddWithHttpURL()
        {
            // Add source with an HTTP url should fail
            var result = TestCommon.RunAICLICommand("source add", "Insecure http://microsoft.com");
            Assert.AreEqual(Constants.ErrorCode.ERROR_SOURCE_NOT_SECURE, result.ExitCode);
            Assert.True(result.StdOut.Contains("error occurred while executing the command"));
        }

        [Test]
        public void SourceListWithNoArgs()
        {
            // List with no args should list all available sources
            var result = TestCommon.RunAICLICommand("source list", "");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("https://localhost:5001/TestKit"));
        }

        [Test]
        public void SourceListWithName()
        {
            var result = TestCommon.RunAICLICommand("source list", $"-n {Constants.TestSourceName}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("TestSource"));
            Assert.True(result.StdOut.Contains("https://localhost:5001/TestKit"));
            Assert.True(result.StdOut.Contains("Microsoft.PreIndexed.Package"));
            Assert.True(result.StdOut.Contains("Updated"));
        }

        [Test]
        public void SourceListNameMismatch()
        {
            var result = TestCommon.RunAICLICommand("source list", "-n UnknownName");
            Assert.AreEqual(Constants.ErrorCode.ERROR_SOURCE_NAME_DOES_NOT_EXIST, result.ExitCode);
            Assert.True(result.StdOut.Contains("Did not find a source named"));
        }

        [Test]
        public void SourceUpdate()
        {
            var result = TestCommon.RunAICLICommand("source update", $"-n {Constants.TestSourceName}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Done"));
        }

        [Test]
        public void SourceUpdateWithInvalidName()
        {
            var result = TestCommon.RunAICLICommand("source update", "-n UnknownName");
            Assert.AreEqual(Constants.ErrorCode.ERROR_SOURCE_NAME_DOES_NOT_EXIST, result.ExitCode);
            Assert.True(result.StdOut.Contains("Did not find a source named: UnknownName"));
        }

        [Test]
        public void SourceRemoveValidName()
        {
            var result = TestCommon.RunAICLICommand("source remove", $"-n {Constants.TestSourceName}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Done"));
            ResetTestSource();
        }

        [Test]
        public void SourceRemoveInvalidName()
        {
            var result = TestCommon.RunAICLICommand("source remove", "-n UnknownName");
            Assert.AreEqual(Constants.ErrorCode.ERROR_SOURCE_NAME_DOES_NOT_EXIST, result.ExitCode);
            Assert.True(result.StdOut.Contains("Did not find a source named: UnknownName"));
        }

        [Test]
        public void SourceReset()
        {
            var result = TestCommon.RunAICLICommand("source reset", "");
            Assert.True(result.StdOut.Contains("The following sources will be reset if the --force option is given:"));
            Assert.True(result.StdOut.Contains("TestSource"));
            Assert.True(result.StdOut.Contains("https://localhost:5001/TestKit"));
        }

        [Test]
        public void SourceForceReset()
        {
            // Force Reset Sources
            var result = TestCommon.RunAICLICommand("source reset", "--force");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Resetting all sources...Done"));

            //Verify sources have been reset
            result = TestCommon.RunAICLICommand("source list", "");
            Assert.True(result.StdOut.Contains("winget"));
            Assert.True(result.StdOut.Contains("https://winget.azureedge.net/cache"));
            Assert.False(result.StdOut.Contains($"{Constants.TestSourceName}"));
            Assert.False(result.StdOut.Contains($"{Constants.TestSourceUrl}"));
            ResetTestSource();
        }
    }
}