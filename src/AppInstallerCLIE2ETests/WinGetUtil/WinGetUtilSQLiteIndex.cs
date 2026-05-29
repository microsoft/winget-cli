﻿// -----------------------------------------------------------------------------
// <copyright file="WinGetUtilSQLiteIndex.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests.WinGetUtil
{
    using System;
    using System.IO;
    using System.Runtime.InteropServices;
    using AppInstallerCLIE2ETests.Helpers;
    using NUnit.Framework;

    /// <summary>
    /// WinGetUtil sql index tests.
    /// </summary>
    public class WinGetUtilSQLiteIndex
    {
        private readonly uint majorVersion = 1;
        private readonly uint minorVersion = 2;

        // Manifest example 1
        private readonly string addManifestsFile = TestCommon.GetTestDataFile(@"WinGetUtil\Manifests\Merged\WinGetUtilTest.Add.yaml");
        private readonly string updateManifestsFile = TestCommon.GetTestDataFile(@"WinGetUtil\Manifests\Merged\WinGetUtilTest.Update.yaml");
        private readonly string relativePath = @"manifests\a\AppInstallerTest\WinGetUtilTest\1.0.0.0\WinGetTest.yaml";

        private string sqlitePath;

        /// <summary>
        /// Set up.
        /// </summary>
        [SetUp]
        public void SetUp()
        {
             this.sqlitePath = TestCommon.GetRandomTestFile(".sql");
        }

        /// <summary>
        /// Test add manifest.
        /// </summary>
        [Test]
        public void WinGetUtil_SQLiteIndex_AddManifest()
        {
            this.SQLiteIndex((indexHandle) =>
            {
                // Add manifest
                WinGetUtilWrapper.WinGetSQLiteIndexAddManifest(indexHandle, this.addManifestsFile, this.relativePath);
            });
        }

        /// <summary>
        /// Test update manifest.
        /// </summary>
        [Test]
        public void WinGetUtil_SQLiteIndex_UpdateManifest_Success()
        {
            this.SQLiteIndex((indexHandle) =>
            {
                // Add manifest
                WinGetUtilWrapper.WinGetSQLiteIndexAddManifest(indexHandle, this.addManifestsFile, this.relativePath);

                // Update manifest
                WinGetUtilWrapper.WinGetSQLiteIndexUpdateManifest(indexHandle, this.updateManifestsFile, this.relativePath, out bool indexModified);
                Assert.That(indexModified, Is.True);
            });
        }

        /// <summary>
        /// Test update manifest file not found.
        /// </summary>
        [Test]
        public void WinGetUtil_SQLiteIndex_UpdateManifest_Fail_NotFound()
        {
            this.SQLiteIndex((indexHandle) =>
            {
                // Update non-existing manifest
                Assert.That((Action)(() =>
                {
                    WinGetUtilWrapper.WinGetSQLiteIndexUpdateManifest(indexHandle, this.updateManifestsFile, this.relativePath, out bool indexModified);
                }), Throws.TypeOf<COMException>());
            });
        }

        /// <summary>
        /// Test remove manifest.
        /// </summary>
        [Test]
        public void WinGetUtil_SQLiteIndex_RemoveManifest_Success()
        {
            this.SQLiteIndex((indexHandle) =>
            {
                // Add manifest
                WinGetUtilWrapper.WinGetSQLiteIndexAddManifest(indexHandle, this.addManifestsFile, this.relativePath);

                // Remove manifest
                WinGetUtilWrapper.WinGetSQLiteIndexRemoveManifest(indexHandle, this.addManifestsFile, this.relativePath);
            });
        }

        /// <summary>
        /// Test remove manifest file not found.
        /// </summary>
        [Test]
        public void WinGetUtil_SQLiteIndex_RemoveManifest_Fail_NotFound()
        {
            this.SQLiteIndex((indexHandle) =>
            {
                // Remove non-existing manifest
                Assert.That((Action)(() =>
                {
                    WinGetUtilWrapper.WinGetSQLiteIndexRemoveManifest(indexHandle, this.addManifestsFile, this.relativePath);
                }), Throws.TypeOf<COMException>());
            });
        }

        /// <summary>
        /// Test open and closing index.
        /// </summary>
        [Test]
        public void WinGetUtil_SQLiteIndex_OpenClose()
        {
            this.SQLiteIndex((_) =>
            {
                // Open
                WinGetUtilWrapper.WinGetSQLiteIndexOpen(this.sqlitePath, out IntPtr indexHandle);

                // Add manifest
                WinGetUtilWrapper.WinGetSQLiteIndexAddManifest(indexHandle, this.addManifestsFile, this.relativePath);

                // Close
                WinGetUtilWrapper.WinGetSQLiteIndexClose(indexHandle);
            });
        }

        /// <summary>
        /// Test check consistency.
        /// </summary>
        [Test]
        public void WinGetUtil_SQLiteIndex_CheckConsistency()
        {
            this.SQLiteIndex((indexHandle) =>
            {
                // Add manifest
                WinGetUtilWrapper.WinGetSQLiteIndexAddManifest(indexHandle, this.addManifestsFile, this.relativePath);

                // Prepare for packaging
                WinGetUtilWrapper.WinGetSQLiteIndexPrepareForPackaging(indexHandle);

                // Check consistency
                WinGetUtilWrapper.WinGetSQLiteIndexCheckConsistency(indexHandle, out bool succeeded);
                Assert.That(succeeded, Is.True);
            });
        }

        /// <summary>
        /// Create and close an sqlite index file.
        /// </summary>
        /// <param name="execute">Function to execute.</param>
        private void SQLiteIndex(Action<IntPtr> execute)
        {
            // Create
            WinGetUtilWrapper.WinGetSQLiteIndexCreate(this.sqlitePath, this.majorVersion, this.minorVersion, out IntPtr indexHandle);
            Assert.That(File.Exists(this.sqlitePath), Is.True);
            Assert.That(indexHandle, Is.Not.EqualTo(IntPtr.Zero));

            // Execute provided function
            execute(indexHandle);

            // Close
            WinGetUtilWrapper.WinGetSQLiteIndexClose(indexHandle);
        }
    }
}
