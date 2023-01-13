// -----------------------------------------------------------------------------
// <copyright file="SourceCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests
{
    using NUnit.Framework;

    /// <summary>
    /// Test source command.
    /// </summary>
    public class SourceCommand : BaseCommand
    {
        /// <summary>
        /// Test set up.
        /// </summary>
        [SetUp]
        public void Setup()
        {
            this.ResetTestSource(false);
        }

        /// <summary>
        /// Test source add.
        /// </summary>
        [Test]
        public void SourceAdd()
        {
            var result = TestCommon.RunAICLICommand("source add", $"SourceTest {Constants.TestSourceUrl}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Done"));
            TestCommon.RunAICLICommand("source remove", $"-n SourceTest");
        }

        /// <summary>
        /// Test source add with duplicate name.
        /// </summary>
        [Test]
        public void SourceAddWithDuplicateName()
        {
            // Add source with duplicate name should fail
            var result = TestCommon.RunAICLICommand("source add", $"{Constants.TestSourceName} https://microsoft.com");
            Assert.AreEqual(Constants.ErrorCode.ERROR_SOURCE_NAME_ALREADY_EXISTS, result.ExitCode);
            Assert.True(result.StdOut.Contains("A source with the given name already exists and refers to a different location"));
        }

        /// <summary>
        /// Test source add with invalid url.
        /// </summary>
        [Test]
        public void SourceAddWithInvalidURL()
        {
            // Add source with invalid url should fail
            var result = TestCommon.RunAICLICommand("source add", $"AnotherSource {Constants.TestSourceUrl}/Invalid/Directory/Dont/Add/Me");
            Assert.AreEqual(Constants.ErrorCode.HTTP_E_STATUS_NOT_FOUND, result.ExitCode);
            Assert.True(result.StdOut.Contains("An unexpected error occurred while executing the command"));
        }

        /// <summary>
        /// Test source add with http url.
        /// </summary>
        [Test]
        public void SourceAddWithHttpURL()
        {
            // Add source with an HTTP url should fail
            var result = TestCommon.RunAICLICommand("source add", "Insecure http://microsoft.com");
            Assert.AreEqual(Constants.ErrorCode.ERROR_SOURCE_NOT_SECURE, result.ExitCode);
            Assert.True(result.StdOut.Contains("error occurred while executing the command"));
        }

        /// <summary>
        /// Test source list with no args.
        /// </summary>
        [Test]
        public void SourceListWithNoArgs()
        {
            // List with no args should list all available sources
            var result = TestCommon.RunAICLICommand("source list", string.Empty);
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains(Constants.TestSourceUrl));
        }

        /// <summary>
        /// Test source list with name.
        /// </summary>
        [Test]
        public void SourceListWithName()
        {
            var result = TestCommon.RunAICLICommand("source list", $"-n {Constants.TestSourceName}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains(Constants.TestSourceName));
            Assert.True(result.StdOut.Contains(Constants.TestSourceUrl));
            Assert.True(result.StdOut.Contains("Microsoft.PreIndexed.Package"));
            Assert.True(result.StdOut.Contains("Updated"));
        }

        /// <summary>
        /// Test source list name mismatch.
        /// </summary>
        [Test]
        public void SourceListNameMismatch()
        {
            var result = TestCommon.RunAICLICommand("source list", "-n UnknownName");
            Assert.AreEqual(Constants.ErrorCode.ERROR_SOURCE_NAME_DOES_NOT_EXIST, result.ExitCode);
            Assert.True(result.StdOut.Contains("Did not find a source named"));
        }

        /// <summary>
        /// Test source update.
        /// </summary>
        [Test]
        public void SourceUpdate()
        {
            var result = TestCommon.RunAICLICommand("source update", $"-n {Constants.TestSourceName}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Done"));
        }

        /// <summary>
        /// Test source update with invalid name.
        /// </summary>
        [Test]
        public void SourceUpdateWithInvalidName()
        {
            var result = TestCommon.RunAICLICommand("source update", "-n UnknownName");
            Assert.AreEqual(Constants.ErrorCode.ERROR_SOURCE_NAME_DOES_NOT_EXIST, result.ExitCode);
            Assert.True(result.StdOut.Contains("Did not find a source named: UnknownName"));
        }

        /// <summary>
        /// Test source remove by name.
        /// </summary>
        [Test]
        public void SourceRemoveValidName()
        {
            var result = TestCommon.RunAICLICommand("source remove", $"-n {Constants.TestSourceName}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Done"));
            this.ResetTestSource(false);
        }

        /// <summary>
        /// Test source remove with invalid name.
        /// </summary>
        [Test]
        public void SourceRemoveInvalidName()
        {
            var result = TestCommon.RunAICLICommand("source remove", "-n UnknownName");
            Assert.AreEqual(Constants.ErrorCode.ERROR_SOURCE_NAME_DOES_NOT_EXIST, result.ExitCode);
            Assert.True(result.StdOut.Contains("Did not find a source named: UnknownName"));
        }

        /// <summary>
        /// Test source reset.
        /// </summary>
        [Test]
        public void SourceReset()
        {
            var result = TestCommon.RunAICLICommand("source reset", string.Empty);
            Assert.True(result.StdOut.Contains("The following sources will be reset if the --force option is given:"));
            Assert.True(result.StdOut.Contains(Constants.TestSourceName));
            Assert.True(result.StdOut.Contains(Constants.TestSourceUrl));
        }

        /// <summary>
        /// Test source reset force.
        /// </summary>
        [Test]
        public void SourceForceReset()
        {
            // Force Reset Sources
            var result = TestCommon.RunAICLICommand("source reset", "--force");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Resetting all sources...Done"));

            // Verify sources have been reset
            result = TestCommon.RunAICLICommand("source list", string.Empty);
            Assert.True(result.StdOut.Contains("winget"));
            Assert.True(result.StdOut.Contains("https://cdn.winget.microsoft.com/cache"));
            Assert.False(result.StdOut.Contains(Constants.TestSourceName));
            Assert.False(result.StdOut.Contains(Constants.TestSourceUrl));
        }
    }
}