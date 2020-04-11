// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
{
    using NUnit.Framework;

    public class SourceCommand
    {
        [SetUp]
        [TearDown]
        public void Setup()
        {
            TestCommon.RunAICLICommand("source", $"remove {Constants.TestSourceName}");
        }
        
        [Test]
        public void SourceCommands()
        {
            // Add test source should succeed
            var result = TestCommon.RunAICLICommand("source", $"add {Constants.TestSourceName} {Constants.TestSourceUrl}");
            Assert.AreEqual(result.ExitCode, Constants.ErrorCode.S_OK);
            Assert.True(result.StdOut.Contains("Done"));

            // Add source with duplicate name should fail
            result = TestCommon.RunAICLICommand("source", $"add {Constants.TestSourceName} https://microsoft.com");
            Assert.AreEqual(result.ExitCode, Constants.ErrorCode.ERROR_SOURCE_NAME_ALREADY_EXISTS);
            Assert.True(result.StdOut.Contains("A source with the given name already exists and refers to a different location"));

            // Add source with invalid url should fail
            result = TestCommon.RunAICLICommand("source", "add AnotherSource https://microsoft.com");
            Assert.AreEqual(result.ExitCode, Constants.ErrorCode.ERROR_NO_RANGES_PROCESSED);
            Assert.True(result.StdOut.Contains("error occurred while executing the command"));

            // List when source exists
            result = TestCommon.RunAICLICommand("source", "list");
            Assert.AreEqual(result.ExitCode, Constants.ErrorCode.S_OK);
            Assert.True(result.StdOut.Contains("TestSource -> https://pkgmgr-int.azureedge.net/cache"));

            // List when source name matches, it shows detailed info
            result = TestCommon.RunAICLICommand("source", $"list -n {Constants.TestSourceName}");
            Assert.AreEqual(result.ExitCode, Constants.ErrorCode.S_OK);
            Assert.True(result.StdOut.Contains("Name   : TestSource"));
            Assert.True(result.StdOut.Contains("Arg    : https://pkgmgr-int.azureedge.net/cache"));
            Assert.True(result.StdOut.Contains("Data   : AppInstallerSQLiteIndex-int_2020.413.2430.751_neutral__g4ype1skzj3jy"));
            Assert.True(result.StdOut.Contains("Updated: "));

            // List when source name does not match
            result = TestCommon.RunAICLICommand("source", "list -n UnknownName");
            Assert.AreEqual(result.ExitCode, Constants.ErrorCode.ERROR_SOURCE_NAME_DOES_NOT_EXIST);
            Assert.True(result.StdOut.Contains("Did not find a source named: UnknownName"));

            // Update should succeed
            result = TestCommon.RunAICLICommand("source", $"update -n {Constants.TestSourceName}");
            Assert.AreEqual(result.ExitCode, Constants.ErrorCode.S_OK);
            Assert.True(result.StdOut.Contains("Done"));

            // Update with bad name should fail
            result = TestCommon.RunAICLICommand("source", "update -n UnknownName");
            Assert.AreEqual(result.ExitCode, Constants.ErrorCode.ERROR_SOURCE_NAME_DOES_NOT_EXIST);
            Assert.True(result.StdOut.Contains("Did not find a source named: UnknownName"));

            // Remove with a bad name should fail
            result = TestCommon.RunAICLICommand("source", "remove -n UnknownName");
            Assert.AreEqual(result.ExitCode, Constants.ErrorCode.ERROR_SOURCE_NAME_DOES_NOT_EXIST);
            Assert.True(result.StdOut.Contains("Did not find a source named: UnknownName"));

            // Remove with a good name should succeed
            result = TestCommon.RunAICLICommand("source", $"remove -n {Constants.TestSourceName}");
            Assert.AreEqual(result.ExitCode, Constants.ErrorCode.S_OK);
            Assert.True(result.StdOut.Contains("Done"));

            // List should show no source
            result = TestCommon.RunAICLICommand("source", "list");
            Assert.AreEqual(result.ExitCode, Constants.ErrorCode.S_OK);
            Assert.True(result.StdOut.Contains("<none>"));
        }
    }
}