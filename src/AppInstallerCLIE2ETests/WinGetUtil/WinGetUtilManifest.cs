﻿// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests.WinGetUtil
{
    using System;
    using System.IO;
    using NUnit.Framework;

    public class WinGetUtilManifest
    {
        IntPtr indexHandle;

        [SetUp]
        public void SetUp()
        {
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
        public void ValidateManifest()
        {
            string manifestsDir = TestCommon.GetTestDataFile(@"WinGetUtil\Manifests\Unmerged\ValidateManifest");
            string mergedManifestPath = TestCommon.GetRandomTestFile(".yaml");
            IntPtr hresult;

            // Create manifest
            hresult = WinGetUtilWrapper.WinGetCreateManifest(
                manifestsDir,
                out bool succeeded,
                out IntPtr manifestHandle,
                out string createFailureMessage,
                mergedManifestPath,
                WinGetUtilWrapper.CreateManifestOption.NoValidation);

            Assert.AreEqual(IntPtr.Zero, hresult);
            Assert.True(succeeded);
            Assert.AreNotEqual(IntPtr.Zero, manifestHandle);
            Assert.IsNull(createFailureMessage);
            Assert.True(File.Exists(mergedManifestPath));

            // Validate manifest
            hresult = WinGetUtilWrapper.WinGetValidateManifestV3(
                manifestHandle,
                indexHandle,
                out WinGetUtilWrapper.ValidateManifestResultCode resultCode,
                out string validateFailureMessage,
                WinGetUtilWrapper.ValidateManifestOptionV2.ArpVersionValidation,
                WinGetUtilWrapper.ValidateManifestOperationType.Add);

            Assert.AreEqual(IntPtr.Zero, hresult);
            Assert.AreEqual(WinGetUtilWrapper.ValidateManifestResultCode.Success, resultCode);
            Assert.IsEmpty(validateFailureMessage);

            // Close manifest
            hresult = WinGetUtilWrapper.WinGetCloseManifest(manifestHandle);

            Assert.AreEqual(IntPtr.Zero, hresult);
        }
    }
}
