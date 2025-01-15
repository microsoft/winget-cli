// -----------------------------------------------------------------------------
// <copyright file="ValidateCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests
{
    using AppInstallerCLIE2ETests.Helpers;
    using NUnit.Framework;

    /// <summary>
    /// Test validate command.
    /// </summary>
    public class ValidateCommand : BaseCommand
    {
        /// <summary>
        /// Test validate manifest.
        /// </summary>
        [Test]
        public void ValidateManifest()
        {
            var result = TestCommon.RunAICLICommand("validate", TestCommon.GetTestDataFile("Manifests\\TestValidManifest.yaml"));
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Manifest validation succeeded."));
        }

        /// <summary>
        /// Test validate manifest with extended characters.
        /// </summary>
        [Test]
        public void ValidateManifestWithExtendedCharacter()
        {
            var result = TestCommon.RunAICLICommand("validate", TestCommon.GetTestDataFile("Manifests\\TëstExeInstaller.yaml"));
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Manifest validation succeeded."));
        }

        /// <summary>
        /// Test validate invalid manifest.
        /// </summary>
        [Test]
        public void ValidateInvalidManifest()
        {
            var result = TestCommon.RunAICLICommand("validate", TestCommon.GetTestDataFile("Manifests\\TestInvalidManifest.yaml"));
            Assert.AreEqual(Constants.ErrorCode.ERROR_MANIFEST_VALIDATION_FAILURE, result.ExitCode);
            Assert.True(result.StdOut.Contains("Manifest validation failed."));
        }

        /// <summary>
        /// Test validate manifest with warnings.
        /// </summary>
        [Test]
        public void ValidateManifestWithWarnings()
        {
            var result = TestCommon.RunAICLICommand("validate", TestCommon.GetTestDataFile("Manifests\\TestWarningManifest.yaml"));
            Assert.AreEqual(Constants.ErrorCode.ERROR_MANIFEST_VALIDATION_WARNING, result.ExitCode);
            Assert.True(result.StdOut.Contains("Manifest validation succeeded with warnings."));
        }

        /// <summary>
        /// Test validate manifest with warnings suppressed.
        /// </summary>
        [Test]
        public void ValidateManifestSuppressWarnings()
        {
            var result = TestCommon.RunAICLICommand("validate", TestCommon.GetTestDataFile("Manifests\\TestWarningManifest.yaml") + " --ignore-warnings");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.False(result.StdOut.Contains("Manifest validation succeeded with warnings."));
            Assert.True(result.StdOut.Contains("Manifest validation succeeded."));
        }

        /// <summary>
        /// Test validate manifest that doesn't exist.
        /// </summary>
        [Test]
        public void ValidateManifestDoesNotExist()
        {
            var result = TestCommon.RunAICLICommand("validate", TestCommon.GetTestDataFile("Manifests\\DoesNotExist"));
            Assert.AreEqual(Constants.ErrorCode.ERROR_PATH_NOT_FOUND, result.ExitCode);
            Assert.True(result.StdOut.Contains("Path does not exist"));
        }
    }
}