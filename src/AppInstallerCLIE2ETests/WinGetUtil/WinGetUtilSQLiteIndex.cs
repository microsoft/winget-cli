// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests.WinGetUtil
{
    using System;
    using System.IO;
    using System.Runtime.InteropServices;
    using NUnit.Framework;

    public class WinGetUtilSQLiteIndex
    {
        private string sqlitePath;
        private readonly uint majorVersion = 1;
        private readonly uint minorVersion = 2;

        // Manifest example 1
        private readonly string addManifestsFile_1 = TestCommon.GetTestDataFile(@"WinGetUtil\Manifests\Merged\WinGetUtilTest.Add.yaml");
        private readonly string updateManifestsFile_1 = TestCommon.GetTestDataFile(@"WinGetUtil\Manifests\Merged\WinGetUtilTest.Update.yaml");
        private readonly string relativePath_1 = @"manifests\a\AppInstallerTest\WinGetUtilTest\1.0.0.0\WinGetTest.yaml";

        [SetUp]
        public void SetUp()
        {
             this.sqlitePath = TestCommon.GetRandomTestFile(".sql");
        }

        [Test]
        public void WinGetUtil_SQLiteIndex_AddManifest()
        {
            SQLiteIndex((indexHandle) =>
            {
                // Add manifest
                WinGetUtilWrapper.WinGetSQLiteIndexAddManifest(indexHandle, addManifestsFile_1, relativePath_1);
            });
        }

        [Test]
        public void WinGetUtil_SQLiteIndex_UpdateManifest_Success()
        {
            SQLiteIndex((indexHandle) =>
            {
                // Add manifest
                WinGetUtilWrapper.WinGetSQLiteIndexAddManifest(indexHandle, addManifestsFile_1, relativePath_1);
                
                // Update manifest
                WinGetUtilWrapper.WinGetSQLiteIndexUpdateManifest(indexHandle, updateManifestsFile_1, relativePath_1, out bool indexModified);
                Assert.True(indexModified);
            });
        }

        [Test]
        public void WinGetUtil_SQLiteIndex_UpdateManifest_Fail_NotFound()
        {
            SQLiteIndex((indexHandle) =>
            {
                // Update non-existing manifest
                Assert.Throws<COMException>(() =>
                {
                    WinGetUtilWrapper.WinGetSQLiteIndexUpdateManifest(indexHandle, updateManifestsFile_1, relativePath_1, out bool indexModified);
                });
            });
        }

        [Test]
        public void WinGetUtil_SQLiteIndex_RemoveManifest_Success()
        {
            SQLiteIndex((indexHandle) =>
            {
                // Add manifest
                WinGetUtilWrapper.WinGetSQLiteIndexAddManifest(indexHandle, addManifestsFile_1, relativePath_1);
                
                // Remove manifest
                WinGetUtilWrapper.WinGetSQLiteIndexRemoveManifest(indexHandle, addManifestsFile_1, relativePath_1);
            });
        }

        [Test]
        public void WinGetUtil_SQLiteIndex_RemoveManifest_Fail_NotFound()
        {
            SQLiteIndex((indexHandle) =>
            {
                // Remove non-existing manifest
                Assert.Throws<COMException>(() =>
                {
                    WinGetUtilWrapper.WinGetSQLiteIndexRemoveManifest(indexHandle, addManifestsFile_1, relativePath_1);
                });
            });
        }

        [Test]
        public void WinGetUtil_SQLiteIndex_OpenClose()
        {
            SQLiteIndex((_) =>
            {
                // Open
                WinGetUtilWrapper.WinGetSQLiteIndexOpen(sqlitePath, out IntPtr indexHandle);

                // Add manifest
                WinGetUtilWrapper.WinGetSQLiteIndexAddManifest(indexHandle, addManifestsFile_1, relativePath_1);

                // Close
                WinGetUtilWrapper.WinGetSQLiteIndexClose(indexHandle);
            });
        }

        [Test]
        public void WinGetUtil_SQLiteIndex_CheckConsistency()
        {
            SQLiteIndex((indexHandle) =>
            {
                // Add manifest
                WinGetUtilWrapper.WinGetSQLiteIndexAddManifest(indexHandle, addManifestsFile_1, relativePath_1);

                // Prepare for packaging
                WinGetUtilWrapper.WinGetSQLiteIndexPrepareForPackaging(indexHandle);

                // Check consistency
                WinGetUtilWrapper.WinGetSQLiteIndexCheckConsistency(indexHandle, out bool succeeded);
                Assert.True(succeeded);
            });
        }

        /// <summary>
        /// Create and close an sqlite index file.
        /// </summary>
        /// <param name="Execute">Function to execute.</param>
        private void SQLiteIndex(Action<IntPtr> Execute)
        {
            // Create
            WinGetUtilWrapper.WinGetSQLiteIndexCreate(sqlitePath, majorVersion, minorVersion, out IntPtr indexHandle);
            Assert.True(File.Exists(sqlitePath));
            Assert.AreNotEqual(IntPtr.Zero, indexHandle);
            
            // Execute provided function
            Execute(indexHandle);

            // Close
            WinGetUtilWrapper.WinGetSQLiteIndexClose(indexHandle);
        }
    }
}
