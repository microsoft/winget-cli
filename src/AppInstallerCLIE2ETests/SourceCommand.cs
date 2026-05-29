// -----------------------------------------------------------------------------
// <copyright file="SourceCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests
{
    using AppInstallerCLIE2ETests.Helpers;
    using NUnit.Framework;

    /// <summary>
    /// Test source command.
    /// </summary>
    public class SourceCommand : BaseCommand
    {
        /// <summary>
        /// One time set up.
        /// </summary>
        [OneTimeSetUp]
        public void OneTimeSetup()
        {
            WinGetSettingsHelper.ConfigureFeature("sourcePriority", true);
        }

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
            // TODO: Our test source package is being rejected by SmartScreen on the build server.
            //       Reenable when SmartScreen issue is solved or removed.
            Assert.Ignore();

            var result = TestCommon.RunAICLICommand("source add", $"SourceTest {Constants.TestSourceUrl}");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut.Contains("Done"), Is.True);
            TestCommon.RunAICLICommand("source remove", $"-n SourceTest");
        }

        /// <summary>
        /// Test source add with trust level.
        /// </summary>
        [Test]
        public void SourceAddWithTrustLevel()
        {
            // Remove the test source.
            TestCommon.RunAICLICommand("source remove", Constants.TestSourceName);

            var result = TestCommon.RunAICLICommand("source add", $"SourceTest {Constants.TestSourceUrl} --trust-level trusted");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut.Contains("Done"), Is.True);

            var listResult = TestCommon.RunAICLICommand("source list", $"-n SourceTest");
            Assert.That(listResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(listResult.StdOut.Contains("Trust Level"), Is.True);
            Assert.That(listResult.StdOut.Contains("Trusted"), Is.True);
            TestCommon.RunAICLICommand("source remove", $"-n SourceTest");
        }

        /// <summary>
        /// Test source add with store origin trust level.
        /// </summary>
        [Test]
        public void SourceAddWithStoreOriginTrustLevel()
        {
            // Remove the test source.
            TestCommon.RunAICLICommand("source remove", Constants.TestSourceName);

            var result = TestCommon.RunAICLICommand("source add", $"SourceTest {Constants.TestSourceUrl} --trust-level storeOrigin");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.ERROR_SOURCE_DATA_INTEGRITY_FAILURE));
            Assert.That(result.StdOut.Contains("The source data is corrupted or tampered"), Is.True);
        }

        /// <summary>
        /// Test source add with explicit flag. Packages should only appear if the source is explicitly declared.
        /// </summary>
        [Test]
        public void SourceAddWithExplicit()
        {
            // Remove the test source.
            TestCommon.RunAICLICommand("source remove", Constants.TestSourceName);

            var result = TestCommon.RunAICLICommand("source add", $"SourceTest {Constants.TestSourceUrl} --trust-level trusted --explicit");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut.Contains("Done"), Is.True);

            var searchResult = TestCommon.RunAICLICommand("search", "TestExampleInstaller");
            Assert.That(searchResult.ExitCode, Is.EqualTo(Constants.ErrorCode.ERROR_NO_SOURCES_DEFINED));
            Assert.That(searchResult.StdOut.Contains("No sources defined; add one with 'source add' or reset to defaults with 'source reset'"), Is.True);

            var searchResult2 = TestCommon.RunAICLICommand("search", "TestExampleInstaller --source SourceTest");
            Assert.That(searchResult2.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(searchResult2.StdOut.Contains("TestExampleInstaller"), Is.True);
            Assert.That(searchResult2.StdOut.Contains("AppInstallerTest.TestExampleInstaller"), Is.True);
            TestCommon.RunAICLICommand("source remove", $"-n SourceTest");
        }

        /// <summary>
        /// Test source add with a priority value.
        /// </summary>
        [Test]
        public void SourceAddWithPriority()
        {
            // Remove the test source.
            TestCommon.RunAICLICommand("source remove", Constants.TestSourceName);

            var result = TestCommon.RunAICLICommand("source add", $"SourceTest {Constants.TestSourceUrl} --priority 42");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut.Contains("Done"), Is.True);

            var listResult = TestCommon.RunAICLICommand("source list", "SourceTest");
            Assert.That(listResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(listResult.StdOut.Contains("42"), Is.True);

            var exportResult = TestCommon.RunAICLICommand("source export", string.Empty);
            Assert.That(listResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(exportResult.StdOut.Contains("42"), Is.True);
        }

        /// <summary>
        /// Test source add with duplicate name.
        /// </summary>
        [Test]
        public void SourceAddWithDuplicateName()
        {
            // Add source with duplicate name should fail
            var result = TestCommon.RunAICLICommand("source add", $"{Constants.TestSourceName} https://microsoft.com");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.ERROR_SOURCE_NAME_ALREADY_EXISTS));
            Assert.That(result.StdOut.Contains("A source with the given name already exists and refers to a different location"), Is.True);
        }

        /// <summary>
        /// Test source add with duplicate source url.
        /// </summary>
        [Test]
        public void SourceAddWithDuplicateSourceUrl()
        {
            // Add source with duplicate url should fail
            var result = TestCommon.RunAICLICommand("source add", $"TestSource2 {Constants.TestSourceUrl}");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.ERROR_SOURCE_ARG_ALREADY_EXISTS));
            Assert.That(result.StdOut.Contains("A source with a different name already refers to this location"), Is.True);
        }

        /// <summary>
        /// Test source add with invalid url.
        /// </summary>
        [Test]
        public void SourceAddWithInvalidURL()
        {
            // Add source with invalid url should fail
            var result = TestCommon.RunAICLICommand("source add", $"AnotherSource {Constants.TestSourceUrl}/Invalid/Directory/Dont/Add/Me");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.HTTP_E_STATUS_NOT_FOUND));
            Assert.That(result.StdOut.Contains("An unexpected error occurred while executing the command"), Is.True);
        }

        /// <summary>
        /// Test source add with http url.
        /// </summary>
        [Test]
        public void SourceAddWithHttpURL()
        {
            // Add source with an HTTP url should fail
            var result = TestCommon.RunAICLICommand("source add", "Insecure http://microsoft.com");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.ERROR_SOURCE_NOT_SECURE));
            Assert.That(result.StdOut.Contains("error occurred while executing the command"), Is.True);
        }

        /// <summary>
        /// Test source list with no args.
        /// </summary>
        [Test]
        public void SourceListWithNoArgs()
        {
            // List with no args should list all available sources
            var result = TestCommon.RunAICLICommand("source list", string.Empty);
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut.Contains(Constants.TestSourceUrl), Is.True);
        }

        /// <summary>
        /// Test source list with name.
        /// </summary>
        [Test]
        public void SourceListWithName()
        {
            var result = TestCommon.RunAICLICommand("source list", $"-n {Constants.TestSourceName}");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut.Contains(Constants.TestSourceName), Is.True);
            Assert.That(result.StdOut.Contains(Constants.TestSourceUrl), Is.True);
            Assert.That(result.StdOut.Contains("Microsoft.PreIndexed.Package"), Is.True);
            Assert.That(result.StdOut.Contains("Trust Level"), Is.True);
            Assert.That(result.StdOut.Contains("Updated"), Is.True);
        }

        /// <summary>
        /// Test source list name mismatch.
        /// </summary>
        [Test]
        public void SourceListNameMismatch()
        {
            var result = TestCommon.RunAICLICommand("source list", "-n UnknownName");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.ERROR_SOURCE_NAME_DOES_NOT_EXIST));
            Assert.That(result.StdOut.Contains("Did not find a source named"), Is.True);
        }

        /// <summary>
        /// Test source update.
        /// </summary>
        [Test]
        public void SourceUpdate()
        {
            var result = TestCommon.RunAICLICommand("source update", $"-n {Constants.TestSourceName}");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut.Contains("Done"), Is.True);
        }

        /// <summary>
        /// Test source update with invalid name.
        /// </summary>
        [Test]
        public void SourceUpdateWithInvalidName()
        {
            var result = TestCommon.RunAICLICommand("source update", "-n UnknownName");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.ERROR_SOURCE_NAME_DOES_NOT_EXIST));
            Assert.That(result.StdOut.Contains("Did not find a source named: UnknownName"), Is.True);
        }

        /// <summary>
        /// Test source remove by name.
        /// </summary>
        [Test]
        public void SourceRemoveValidName()
        {
            var result = TestCommon.RunAICLICommand("source remove", $"-n {Constants.TestSourceName}");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut.Contains("Done"), Is.True);
            this.ResetTestSource(false);
        }

        /// <summary>
        /// Test source remove with invalid name.
        /// </summary>
        [Test]
        public void SourceRemoveInvalidName()
        {
            var result = TestCommon.RunAICLICommand("source remove", "-n UnknownName");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.ERROR_SOURCE_NAME_DOES_NOT_EXIST));
            Assert.That(result.StdOut.Contains("Did not find a source named: UnknownName"), Is.True);
        }

        /// <summary>
        /// Test source reset.
        /// </summary>
        [Test]
        public void SourceReset()
        {
            var result = TestCommon.RunAICLICommand("source reset", string.Empty);
            Assert.That(result.StdOut.Contains("The following sources will be reset if the --force option is given:"), Is.True);
            Assert.That(result.StdOut.Contains(Constants.TestSourceName), Is.True);
            Assert.That(result.StdOut.Contains(Constants.TestSourceUrl), Is.True);
        }

        /// <summary>
        /// Test source reset force.
        /// </summary>
        [Test]
        public void SourceForceReset()
        {
            // Force Reset Sources
            var result = TestCommon.RunAICLICommand("source reset", "--force");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut.Contains("Resetting all sources...Done"), Is.True);

            // Verify sources have been reset
            result = TestCommon.RunAICLICommand("source list", string.Empty);
            Assert.That(result.StdOut.Contains("winget"), Is.True);
            Assert.That(result.StdOut.Contains("https://cdn.winget.microsoft.com/cache"), Is.True);
            Assert.That(result.StdOut.Contains(Constants.TestSourceName), Is.False);
            Assert.That(result.StdOut.Contains(Constants.TestSourceUrl), Is.False);
        }

        /// <summary>
        /// Test source edit with explicit flag, edit the source to not be explicit.
        /// </summary>
        [Test]
        public void SourceEdit()
        {
            // Remove the test source.
            TestCommon.RunAICLICommand("source remove", Constants.TestSourceName);

            // Add source as explicit and verify it is explicit.
            var addResult = TestCommon.RunAICLICommand("source add", $"SourceTest {Constants.TestSourceUrl} --trust-level trusted --explicit");
            Assert.That(addResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(addResult.StdOut.Contains("Done"), Is.True);

            var searchResult = TestCommon.RunAICLICommand("search", "TestExampleInstaller");
            Assert.That(searchResult.ExitCode, Is.EqualTo(Constants.ErrorCode.ERROR_NO_SOURCES_DEFINED));
            Assert.That(searchResult.StdOut.Contains("No sources defined; add one with 'source add' or reset to defaults with 'source reset'"), Is.True);

            // Run the edit, this should be S_OK with "Done" as it changed the state to not-explicit.
            var editResult = TestCommon.RunAICLICommand("source edit", $"SourceTest --explicit false");
            Assert.That(editResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(editResult.StdOut.Contains("Explicit"), Is.True);

            // Run it again, this should result in S_OK with no changes and a message that the source is already in that state.
            var editResult2 = TestCommon.RunAICLICommand("source edit", $"SourceTest --explicit false");
            Assert.That(editResult2.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(editResult2.StdOut.Contains("The source named 'SourceTest' is already in the desired state."), Is.True);

            // Now verify it is no longer explicit by running the search again without adding the source parameter.
            var searchResult2 = TestCommon.RunAICLICommand("search", "TestExampleInstaller");
            Assert.That(searchResult2.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(searchResult2.StdOut.Contains("TestExampleInstaller"), Is.True);
            Assert.That(searchResult2.StdOut.Contains("AppInstallerTest.TestExampleInstaller"), Is.True);
            TestCommon.RunAICLICommand("source remove", $"-n SourceTest");
        }

        /// <summary>
        /// Test source edit with priority.
        /// </summary>
        [Test]
        public void SourceEdit_Priority()
        {
            // Remove the test source.
            TestCommon.RunAICLICommand("source remove", Constants.TestSourceName);

            var addResult = TestCommon.RunAICLICommand("source add", $"SourceTest {Constants.TestSourceUrl}");
            Assert.That(addResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(addResult.StdOut.Contains("Done"), Is.True);

            // Run the edit, this should be S_OK with "Done" as it changed the state
            var editResult = TestCommon.RunAICLICommand("source edit", $"SourceTest --priority 14");
            Assert.That(editResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(editResult.StdOut.Contains("14"), Is.True);

            // Run it again, this should result in S_OK with no changes and a message that the source is already in that state.
            var editResult2 = TestCommon.RunAICLICommand("source edit", $"SourceTest --priority 14");
            Assert.That(editResult2.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(editResult2.StdOut.Contains("The source named 'SourceTest' is already in the desired state."), Is.True);
        }

        /// <summary>
        /// Test override of a default source via edit command.
        /// </summary>
        [Test]
        public void SourceEditOverrideDefault()
        {
            // Force Reset Sources
            var resetResult = TestCommon.RunAICLICommand("source reset", "--force");
            Assert.That(resetResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));

            // Verify it is explicit true. Explicit is the only boolean value in the output.
            var listResult = TestCommon.RunAICLICommand("source list", "winget-font");
            Assert.That(listResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(listResult.StdOut.Contains("true"), Is.True);

            var editResult = TestCommon.RunAICLICommand("source edit", "winget-font -e false");
            Assert.That(editResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(editResult.StdOut.Contains("Explicit"), Is.True);

            // Verify that after edit it is now explicit false.
            var listResult2 = TestCommon.RunAICLICommand("source list", "winget-font");
            Assert.That(listResult2.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(listResult2.StdOut.Contains("false"), Is.True);

            // Remove the source. This should correctly tombstone it, even though it is overridden.
            var removeResult = TestCommon.RunAICLICommand("source remove", "winget-font");
            Assert.That(removeResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(removeResult.StdOut.Contains("Done"), Is.True);

            var listResult3 = TestCommon.RunAICLICommand("source list", "winget-font");
            Assert.That(listResult3.ExitCode, Is.EqualTo(Constants.ErrorCode.ERROR_SOURCE_NAME_DOES_NOT_EXIST));

            // Force Reset Sources
            var resetResult2 = TestCommon.RunAICLICommand("source reset", "--force");
            Assert.That(resetResult2.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));

            // Verify it is back to being explicit true.
            var listResult4 = TestCommon.RunAICLICommand("source list", "winget-font");
            Assert.That(listResult4.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(listResult4.StdOut.Contains("true"), Is.True);
        }
    }
}
