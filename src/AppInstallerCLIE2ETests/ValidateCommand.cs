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
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut.Contains("Manifest validation succeeded."), Is.True);
        }

        /// <summary>
        /// Test validate manifest with extended characters.
        /// </summary>
        [Test]
        public void ValidateManifestWithExtendedCharacter()
        {
            var result = TestCommon.RunAICLICommand("validate", TestCommon.GetTestDataFile("Manifests\\TëstExeInstaller.yaml"));
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut.Contains("Manifest validation succeeded."), Is.True);
        }

        /// <summary>
        /// Test validate invalid manifest.
        /// </summary>
        [Test]
        public void ValidateInvalidManifest()
        {
            var result = TestCommon.RunAICLICommand("validate", TestCommon.GetTestDataFile("Manifests\\TestInvalidManifest.yaml"));
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.ERROR_MANIFEST_VALIDATION_FAILURE));
            Assert.That(result.StdOut.Contains("Manifest validation failed."), Is.True);
        }

        /// <summary>
        /// Test validate manifest with warnings.
        /// </summary>
        [Test]
        public void ValidateManifestWithWarnings()
        {
            var result = TestCommon.RunAICLICommand("validate", TestCommon.GetTestDataFile("Manifests\\TestWarningManifest.yaml"));
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.ERROR_MANIFEST_VALIDATION_WARNING));
            Assert.That(result.StdOut.Contains("Manifest validation succeeded with warnings."), Is.True);
        }

        /// <summary>
        /// Test validate manifest with warnings suppressed.
        /// </summary>
        [Test]
        public void ValidateManifestSuppressWarnings()
        {
            var result = TestCommon.RunAICLICommand("validate", TestCommon.GetTestDataFile("Manifests\\TestWarningManifest.yaml") + " --ignore-warnings");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut.Contains("Manifest validation succeeded with warnings."), Is.False);
            Assert.That(result.StdOut.Contains("Manifest validation succeeded."), Is.True);
        }

        /// <summary>
        /// Test validate manifest that doesn't exist.
        /// </summary>
        [Test]
        public void ValidateManifestDoesNotExist()
        {
            var result = TestCommon.RunAICLICommand("validate", TestCommon.GetTestDataFile("Manifests\\DoesNotExist"));
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.ERROR_PATH_NOT_FOUND));
            Assert.That(result.StdOut.Contains("Path does not exist"), Is.True);
        }

        /// <summary>
        /// Test validate manifest with invalid schema and expect warnings.
        /// </summary>
        [Test]
        public void ValidateManifestV1_10_SchemaHeaderExpectWarnings()
        {
            var result = TestCommon.RunAICLICommand("validate", TestCommon.GetTestDataFile("Manifests\\TestWarningManifestV1_10-SchemaHeaderNotFound.yaml"));
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.ERROR_MANIFEST_VALIDATION_WARNING));
            Assert.That(result.StdOut.Contains("Manifest validation succeeded with warnings."), Is.True);
            Assert.That(result.StdOut.Contains("Manifest Warning: Schema header not found."), Is.True);

            result = TestCommon.RunAICLICommand("validate", TestCommon.GetTestDataFile("Manifests\\TestWarningManifestV1_10-SchemaHeaderInvalid.yaml"));
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.ERROR_MANIFEST_VALIDATION_WARNING));
            Assert.That(result.StdOut.Contains("Manifest validation succeeded with warnings."), Is.True);
            Assert.That(result.StdOut.Contains("Manifest Warning: The schema header is invalid. Please verify that the schema header is present and formatted correctly."), Is.True);

            result = TestCommon.RunAICLICommand("validate", TestCommon.GetTestDataFile("Manifests\\TestWarningManifestV1_10-SchemaHeaderURLPatternMismatch.yaml"));
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.ERROR_MANIFEST_VALIDATION_WARNING));
            Assert.That(result.StdOut.Contains("Manifest validation succeeded with warnings."), Is.True);
            Assert.That(result.StdOut.Contains("Manifest Warning: The schema header URL does not match the expected pattern"), Is.True);

            result = TestCommon.RunAICLICommand("validate", TestCommon.GetTestDataFile("Manifests\\TestWarningManifestV1_10-SchemaHeaderManifestTypeMismatch.yaml"));
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.ERROR_MANIFEST_VALIDATION_WARNING));
            Assert.That(result.StdOut.Contains("Manifest validation succeeded with warnings."), Is.True);
            Assert.That(result.StdOut.Contains("Manifest Warning: The manifest type in the schema header does not match the ManifestType property value in the manifest."), Is.True);

            result = TestCommon.RunAICLICommand("validate", TestCommon.GetTestDataFile("Manifests\\TestWarningManifestV1_10-SchemaHeaderVersionMismatch.yaml"));
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.ERROR_MANIFEST_VALIDATION_WARNING));
            Assert.That(result.StdOut.Contains("Manifest validation succeeded with warnings."), Is.True);
            Assert.That(result.StdOut.Contains("Manifest Warning: The manifest version in the schema header does not match the ManifestVersion property value in the manifest."), Is.True);
        }

        /// <summary>
        /// Test validate manifest with valid schema header.
        /// </summary>
        [Test]
        public void ValidateManifestV1_10_SchemaHeaderExpectNoWarning()
        {
            var result = TestCommon.RunAICLICommand("validate", TestCommon.GetTestDataFile("Manifests\\TestGoodManifestV1_10-SchemaHeader.yaml"));
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
        }
    }
}