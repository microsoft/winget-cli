// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests.WinGetUtil
{
    using System;
    using System.IO;
    using NUnit.Framework;

    public class WinGetUtilManifest
    {
        private IntPtr indexHandle;

        [SetUp]
        public void SetUp()
        {
            this.indexHandle = IntPtr.Zero;
            var sqliteFile = TestCommon.GetRandomTestFile(".db");
            uint majorVersion = 1;
            uint minorVersion = 2;
            WinGetUtilWrapper.WinGetSQLiteIndexCreate(sqliteFile, majorVersion, minorVersion, out this.indexHandle); ;
        }

        [TearDown]
        public void TearDown()
        {
            WinGetUtilWrapper.WinGetSQLiteIndexClose(this.indexHandle);
        }

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
                indexHandle,
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
