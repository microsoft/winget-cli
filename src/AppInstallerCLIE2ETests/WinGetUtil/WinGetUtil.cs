using System;
using System.Linq;

namespace AppInstallerCLIE2ETests.WinGetUtil
{
    using AppInstallerCLIE2ETests;
    using NUnit.Framework;
    using System.IO;

    public class WinGetUtil
    {
        [Test]
        // V1 = V2
        [TestCase("1.0.0.0", "1.0.0.0", 0)]
        [TestCase("1.0.0", "1.0.0.0", 0)]
        [TestCase("1.0", "1.0.0.0", 0)]
        [TestCase("1", "1.0.0.0", 0)]
        // V1 > V2
        [TestCase("1.0.0.1", "1.0.0.0", 1)]
        [TestCase("1.0.1.0", "1.0.0.0", 1)]
        [TestCase("1.1.0.0", "1.0.0.0", 1)]
        [TestCase("2.0.0.0", "1.0.0.0", 1)]
        // V1 < V2
        [TestCase("1.0.0.0", "1.0.0.1", -1)]
        [TestCase("1.0.0.0", "1.0.1.0", -1)]
        [TestCase("1.0.0.0", "1.1.0.0", -1)]
        [TestCase("1.0.0.0", "2.0.0.0", -1)]
        public void WinGetCompareVersions(string version1, string version2, int expectedResult)
        {
            // Compare versions
            IntPtr hresult = WinGetUtilWrapper.WinGetCompareVersions(version1, version2, out int result);

            Assert.AreEqual(IntPtr.Zero, hresult);
            Assert.AreEqual(expectedResult, result);
        }

        [Test]
        public void WinGetDownload()
        {
            uint hashSize = 32;
            byte[] sha256Hash = new byte[hashSize];
            string installerUrl = @"https://localhost:5001/TestKit/AppInstallerTestExeInstaller/AppInstallerTestExeInstaller.exe";
            string filePath = TestCommon.GetRandomTestFile(".exe");

            // Download
            IntPtr hresult = WinGetUtilWrapper.WinGetDownload(installerUrl, filePath, sha256Hash, hashSize);

            Assert.AreEqual(IntPtr.Zero, hresult);
            Assert.True(File.Exists(filePath));
            Assert.False(sha256Hash.All(byteVal => byteVal == 0));
        }

        [Test]
        public void Logging()
        {
            IntPtr hresult;
            string filePath = TestCommon.GetRandomTestFile(".log");

            // Init logging
            hresult = WinGetUtilWrapper.WinGetLoggingInit(filePath);

            Assert.AreEqual(IntPtr.Zero, hresult);
            Assert.True(File.Exists(filePath));

            // Terminate logging
            hresult = WinGetUtilWrapper.WinGetLoggingTerm(filePath);

            Assert.AreEqual(IntPtr.Zero, hresult);
        }

        [Test]
        public void ValidateManifest()
        {
            string manifestsDir = TestCommon.GetTestDataFile(@"WinGetUtil\Manifests\Unmerged\ValidateManifest");
            string mergedManifestPath = TestCommon.GetRandomTestFile(".yaml");
            string sqlitePath = TestCommon.GetRandomTestFile(".db");
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

            // SQLite index create
            hresult = WinGetUtilWrapper.WinGetSQLiteIndexCreate(
                sqlitePath,
                majorVersion: 1,
                minorVersion: 2,
                out IntPtr indexHandle);

            Assert.AreEqual(IntPtr.Zero, hresult);
            Assert.True(File.Exists(sqlitePath));
            Assert.AreNotEqual(IntPtr.Zero, indexHandle);

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

            // SQLite index close
            hresult = WinGetUtilWrapper.WinGetSQLiteIndexClose(indexHandle);

            Assert.AreEqual(IntPtr.Zero, hresult);

            // Close manifest
            hresult = WinGetUtilWrapper.WinGetCloseManifest(manifestHandle);

            Assert.AreEqual(IntPtr.Zero, hresult);
        }

        [Test]
        public void SQLiteIndex()
        {
            string addManifestsFile = TestCommon.GetTestDataFile(@"WinGetUtil\Manifests\Merged\WinGetUtilTest.Add.yaml");
            string updateManifestsFile = TestCommon.GetTestDataFile(@"WinGetUtil\Manifests\Merged\WinGetUtilTest.Update.yaml");
            string relativePath = @"manifests\a\AppInstallerTest\WinGetUtilTest\1.0.0.0\WinGetTest.yaml";
            var sqlitePath = TestCommon.GetRandomTestFile(".sql");

            IntPtr hresult;
            uint majorVersion = 1;
            uint minorVersion = 0;

            // Create and open
            hresult = WinGetUtilWrapper.WinGetSQLiteIndexCreate(sqlitePath, majorVersion, minorVersion, out IntPtr indexHandle);

            Assert.AreEqual(IntPtr.Zero, hresult);
            Assert.True(File.Exists(sqlitePath));
            Assert.AreNotEqual(IntPtr.Zero, indexHandle);

            // Add manifest
            hresult = WinGetUtilWrapper.WinGetSQLiteIndexAddManifest(indexHandle, addManifestsFile, relativePath);

            Assert.AreEqual(IntPtr.Zero, hresult);

            // Update manifest
            hresult = WinGetUtilWrapper.WinGetSQLiteIndexUpdateManifest(indexHandle, updateManifestsFile, relativePath, out bool indexModified);

            Assert.AreEqual(IntPtr.Zero, hresult);
            Assert.True(indexModified);

            // Remove manifest
            hresult = WinGetUtilWrapper.WinGetSQLiteIndexRemoveManifest(indexHandle, addManifestsFile, relativePath);

            Assert.AreEqual(IntPtr.Zero, hresult);

            // Packging and check consistency
            hresult = WinGetUtilWrapper.WinGetSQLiteIndexPrepareForPackaging(indexHandle);

            Assert.AreEqual(IntPtr.Zero, hresult);

            // Check consistency
            hresult = WinGetUtilWrapper.WinGetSQLiteIndexCheckConsistency(indexHandle, out bool succeeded);

            Assert.AreEqual(IntPtr.Zero, hresult);
            Assert.True(succeeded);

            // Open
            hresult = WinGetUtilWrapper.WinGetSQLiteIndexOpen(sqlitePath, out IntPtr indexHandle2);

            Assert.AreEqual(IntPtr.Zero, hresult);

            // Close
            hresult = WinGetUtilWrapper.WinGetSQLiteIndexClose(indexHandle2);

            Assert.AreEqual(IntPtr.Zero, hresult);

            // Close
            hresult = WinGetUtilWrapper.WinGetSQLiteIndexClose(indexHandle);

            Assert.AreEqual(IntPtr.Zero, hresult);
        }
    }
}
