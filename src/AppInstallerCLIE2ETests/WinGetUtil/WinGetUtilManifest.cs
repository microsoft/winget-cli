// -----------------------------------------------------------------------------
// <copyright file="WinGetUtilManifest.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests.WinGetUtil
{
    using System;
    using System.IO;
    using AppInstallerCLIE2ETests.Helpers;
    using NUnit.Framework;

    /// <summary>
    /// Test winget util manifest.
    /// </summary>
    public class WinGetUtilManifest
    {
        private IntPtr indexHandle;

        /// <summary>
        /// Set up.
        /// </summary>
        [SetUp]
        public void SetUp()
        {
            this.indexHandle = IntPtr.Zero;
            var sqliteFile = TestCommon.GetRandomTestFile(".db");
            uint majorVersion = 1;
            uint minorVersion = 2;
            WinGetUtilWrapper.WinGetSQLiteIndexCreate(sqliteFile, majorVersion, minorVersion, out this.indexHandle);
        }

        /// <summary>
        /// Tear down.
        /// </summary>
        [TearDown]
        public void TearDown()
        {
            WinGetUtilWrapper.WinGetSQLiteIndexClose(this.indexHandle);
        }

        /// <summary>
        /// Test validate manifest.
        /// </summary>
        /// <param name="createManifestOption">Create manifest options.</param>
        [Test]
        [TestCase(WinGetUtilWrapper.CreateManifestOption.NoValidation)]
        [TestCase(WinGetUtilWrapper.CreateManifestOption.SchemaAndSemanticValidation)]
        public void WinGetUtil_ValidateManifest_Success(WinGetUtilWrapper.CreateManifestOption createManifestOption)
        {
            string manifestsDir = TestCommon.GetTestDataFile(@"WinGetUtil\Manifests\Unmerged\ValidateManifest");
            string mergedManifestPath = TestCommon.GetRandomTestFile(".yaml");

            // Create manifest
            WinGetUtilWrapper.WinGetCreateManifest(
                manifestsDir,
                out bool succeeded,
                out IntPtr manifestHandle,
                out string createFailureMessage,
                mergedManifestPath,
                createManifestOption);

            Assert.That(succeeded, Is.True);
            Assert.That(manifestHandle, Is.Not.EqualTo(IntPtr.Zero));
            Assert.That(createFailureMessage, Is.Null);
            Assert.That(mergedManifestPath, Does.Exist);

            // Validate manifest
            WinGetUtilWrapper.WinGetValidateManifestV3(
                manifestHandle,
                this.indexHandle,
                out WinGetUtilWrapper.ValidateManifestResultCode resultCode,
                out string validateFailureMessage,
                WinGetUtilWrapper.ValidateManifestOptionV2.ArpVersionValidation,
                WinGetUtilWrapper.ValidateManifestOperationType.Add);

            Assert.That(resultCode, Is.EqualTo(WinGetUtilWrapper.ValidateManifestResultCode.Success));
            Assert.That(validateFailureMessage, Is.Empty);

            // Close manifest
            WinGetUtilWrapper.WinGetCloseManifest(manifestHandle);
        }

        /// <summary>
        /// Test validate manifest with schema header.
        /// </summary>
        /// <param name="createManifestOption">Create manifest options.</param>
        [Test]
        [TestCase(WinGetUtilWrapper.CreateManifestOption.NoValidation)]
        [TestCase(WinGetUtilWrapper.CreateManifestOption.SchemaAndSemanticValidation)]
        public void WinGetUtil_ValidateManifest_V1_10_WithSchemaHeader_Success(WinGetUtilWrapper.CreateManifestOption createManifestOption)
        {
            string manifestsFilePath = TestCommon.GetTestDataFile(@"Manifests\TestGoodManifestV1_10-SchemaHeader.yaml");

            // Create manifest
            WinGetUtilWrapper.WinGetCreateManifest(
                manifestsFilePath,
                out bool succeeded,
                out IntPtr manifestHandle,
                out string createFailureMessage,
                string.Empty,
                createManifestOption);

            Assert.That(succeeded, Is.True);
            Assert.That(manifestHandle, Is.Not.EqualTo(IntPtr.Zero));
            Assert.That(createFailureMessage, Is.Null);

            // Close manifest
            WinGetUtilWrapper.WinGetCloseManifest(manifestHandle);
        }

        /// <summary>
        /// Test validate manifest with schema header for failure scenarios.
        /// </summary>
        /// <param name="createManifestOption">Create manifest options.</param>
        [Test]
        [TestCase(WinGetUtilWrapper.CreateManifestOption.SchemaAndSemanticValidation)]
        public void WinGetUtil_ValidateManifest_V1_10_WithSchemaHeader_Failure(WinGetUtilWrapper.CreateManifestOption createManifestOption)
        {
            // Schema header not found
            string manifestsFilePath = TestCommon.GetTestDataFile(@"Manifests\TestWarningManifestV1_10-SchemaHeaderNotFound.yaml");
            string expectedError = "Manifest Error: Schema header not found.";
            ValidateSchemaHeaderFailure(manifestsFilePath, createManifestOption, expectedError);

            // Schema header invalid
            manifestsFilePath = TestCommon.GetTestDataFile(@"Manifests\TestWarningManifestV1_10-SchemaHeaderInvalid.yaml");
            expectedError = "Manifest Error: The schema header is invalid. Please verify that the schema header is present and formatted correctly.";
            ValidateSchemaHeaderFailure(manifestsFilePath, createManifestOption, expectedError);

            // Schema header URL pattern mismatch
            manifestsFilePath = TestCommon.GetTestDataFile(@"Manifests\TestWarningManifestV1_10-SchemaHeaderURLPatternMismatch.yaml");
            expectedError = "Manifest Error: The schema header URL does not match the expected pattern.";
            ValidateSchemaHeaderFailure(manifestsFilePath, createManifestOption, expectedError);

            // Schema header manifest type mismatch
            manifestsFilePath = TestCommon.GetTestDataFile(@"Manifests\TestWarningManifestV1_10-SchemaHeaderManifestTypeMismatch.yaml");
            expectedError = "Manifest Error: The manifest type in the schema header does not match the ManifestType property value in the manifest.";
            ValidateSchemaHeaderFailure(manifestsFilePath, createManifestOption, expectedError);

            // Schema header version mismatch
            manifestsFilePath = TestCommon.GetTestDataFile(@"Manifests\TestWarningManifestV1_10-SchemaHeaderVersionMismatch.yaml");
            expectedError = "Manifest Error: The manifest version in the schema header does not match the ManifestVersion property value in the manifest.";
            ValidateSchemaHeaderFailure(manifestsFilePath, createManifestOption, expectedError);
        }

        private static void ValidateSchemaHeaderFailure(string manifestsFilePath, WinGetUtilWrapper.CreateManifestOption createManifestOption, string expectedError)
        {
            // Create manifest
            WinGetUtilWrapper.WinGetCreateManifest(
                manifestsFilePath,
                out bool succeeded,
                out IntPtr manifestHandle,
                out string createFailureMessage,
                string.Empty,
                createManifestOption);

            Assert.That(succeeded, Is.False);
            Assert.That(manifestHandle, Is.EqualTo(IntPtr.Zero));
            Assert.That(createFailureMessage, Is.Not.Null);
            Assert.That(createFailureMessage, Does.Contain(expectedError));
        }
    }
}
