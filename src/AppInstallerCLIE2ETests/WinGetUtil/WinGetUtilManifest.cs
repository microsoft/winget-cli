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

            Assert.True(succeeded);
            Assert.AreNotEqual(IntPtr.Zero, manifestHandle);
            Assert.IsNull(createFailureMessage);
            Assert.True(File.Exists(mergedManifestPath));

            // Validate manifest
            WinGetUtilWrapper.WinGetValidateManifestV3(
                manifestHandle,
                this.indexHandle,
                out WinGetUtilWrapper.ValidateManifestResultCode resultCode,
                out string validateFailureMessage,
                WinGetUtilWrapper.ValidateManifestOptionV2.ArpVersionValidation,
                WinGetUtilWrapper.ValidateManifestOperationType.Add);

            Assert.AreEqual(WinGetUtilWrapper.ValidateManifestResultCode.Success, resultCode);
            Assert.IsEmpty(validateFailureMessage);

            // Close manifest
            WinGetUtilWrapper.WinGetCloseManifest(manifestHandle);
        }
    }
}
