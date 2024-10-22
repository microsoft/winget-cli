// -----------------------------------------------------------------------------
// <copyright file="SQLiteIndexUnitTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace WinGetUtilInterop.UnitTests.APIUnitTests
{
    using System;
    using System.IO;
    using System.Reflection;
    using Microsoft.WinGetUtil.Api;
    using Microsoft.WinGetUtil.Exceptions;
    using Microsoft.WinGetUtil.Interfaces;
    using Microsoft.WinGetUtil.UnitTests.Common.Logging;
    using Xunit;
    using Xunit.Abstractions;

    /// <summary>
    /// SQLite Index tests.
    /// </summary>
    public class SQLiteIndexUnitTests
    {
        private const string Index = "Index";
        private const string IndexLogFile = "index_log.txt";
        private const string IndexFileName = "index.db";

        private const string PackageTest = "PackageTest.yaml";
        private const string PackageTestNewName = "PackageTestNewName.yaml";
        private const string PackageTestNewVersion = "PackageTestNewVersion.yaml";
        private const string PackageTestRelativePath = @"manifests\t\Test\Test\1.0\Test.Test.yaml";

        private readonly string indexTestFilePath;
        private readonly string indexTestLogFile;
        private readonly string indexTestOutputPath;
        private readonly string indexTestDataPath;

        private ITestOutputHelper log;

        /// <summary>
        /// Initializes a new instance of the <see cref="SQLiteIndexUnitTests"/> class.
        /// </summary>
        /// <param name="log">Output Helper.</param>
        public SQLiteIndexUnitTests(ITestOutputHelper log)
        {
            this.log = log;

            var assemblyLocation = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);
            this.indexTestDataPath = Path.Combine(
                Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location),
                "TestCollateral");

            this.indexTestOutputPath = Path.Combine(Path.GetTempPath(), Guid.NewGuid().ToString(), "WinGetUtilIndexTests");
            Directory.CreateDirectory(this.indexTestOutputPath);

            this.indexTestFilePath = Path.Combine(this.indexTestOutputPath, IndexFileName);
            this.indexTestLogFile = Path.Combine(this.indexTestOutputPath, IndexLogFile);

            if (File.Exists(this.indexTestFilePath))
            {
                File.Delete(this.indexTestFilePath);
            }

            if (File.Exists(this.indexTestLogFile))
            {
                File.Delete(this.indexTestLogFile);
            }
        }

        /// <summary>
        /// Verify opening a existing index file succeeds.
        /// </summary>
        [Fact]
        [DisplayTestMethodName]
        public void OpenIndex()
        {
            this.CreateIndexHelperForIndexTest((wrapper) => true);
            this.OpenIndexHelper((wrapper) => true);
        }

        /// <summary>
        /// Verify opening a fake file fails.
        /// </summary>
        [Fact]
        [DisplayTestMethodName]
        public void OpenIndexFileDontExist()
        {
            // Verify fails with nonexistent file.
            var exception = Assert.Throws<WinGetSQLiteIndexException>(
                () =>
                {
                    var factory = new WinGetFactory();
                    using var log = factory.LoggingInit(this.indexTestLogFile);
                    using var wrapper = factory.SQLiteIndexOpen("fakeFile.db");
                });
            Assert.NotNull(exception.InnerException);
            Assert.True(exception.InnerException is System.Runtime.InteropServices.COMException);
            Assert.Equal(-2018574322, exception.InnerException.HResult);
            File.Delete(this.indexTestLogFile);

            // Verify fails with not an index file.
            string textFile = Path.Combine(this.indexTestOutputPath, "text.txt");
            File.WriteAllText(textFile, "This is a text file.");
            Assert.True(File.Exists(textFile), "File created correctly.");
            var exception2 = Assert.Throws<WinGetSQLiteIndexException>(
                () =>
                {
                    var factory = new WinGetFactory();
                    using var log = factory.LoggingInit(this.indexTestLogFile);
                    using var wrapper = factory.SQLiteIndexOpen(textFile);
                });
            Assert.NotNull(exception2.InnerException);
            Assert.True(exception2.InnerException is System.Runtime.InteropServices.COMException);
            Assert.Equal("File opened that is not a database file (0x87AF001A)", exception2.InnerException.Message);
            File.Delete(textFile);
        }

        /// <summary>
        /// Verifies adding a manifest to the index succeeds.
        /// </summary>
        [Fact]
        [DisplayTestMethodName]
        public void AddManifest()
        {
            this.CreateIndexHelperForIndexTest((wrapper) =>
            {
                // Add manifest.
                string testManifest = Path.Combine(this.indexTestDataPath, PackageTest);
                wrapper.AddManifest(testManifest, PackageTestRelativePath);
                return true;
            });
        }

        /// <summary>
        /// Verify that adding an already existing manifest fails.
        /// </summary>
        [Fact]
        [DisplayTestMethodName]
        public void AddManifestAlreadyExists()
        {
            this.CreateIndexHelperForIndexTest((wrapper) =>
            {
                // Add manifest.
                string testManifest = Path.Combine(this.indexTestDataPath, PackageTest);
                wrapper.AddManifest(testManifest, PackageTestRelativePath);

                // Add manifest again.
                var exception = Assert.Throws<WinGetSQLiteIndexException>(
                    () =>
                    {
                        wrapper.AddManifest(testManifest, PackageTestRelativePath);
                    });
                Assert.NotNull(exception.InnerException);
                Assert.True(exception.InnerException is System.Runtime.InteropServices.COMException);
                Assert.Equal(-2147024713, exception.InnerException.HResult);

                return true;
            });
        }

        /// <summary>
        ///  Verify that updating a manifest with different name.
        /// </summary>
        [Fact]
        [DisplayTestMethodName]
        public void UpdateManifest()
        {
            this.CreateIndexHelperForIndexTest((wrapper) =>
            {
                // Add manifest.
                string addManifest = Path.Combine(this.indexTestDataPath, PackageTest);
                wrapper.AddManifest(addManifest, PackageTestRelativePath);

                // Update manifest. name is different, should return true.
                string updateManifest = Path.Combine(this.indexTestDataPath, PackageTestNewName);
                Assert.True(wrapper.UpdateManifest(updateManifest, PackageTestRelativePath));

                return true;
            });
        }

        /// <summary>
        ///  Verify that updating a manifest in the index with the exact same information succeeds.
        /// </summary>
        [Fact]
        [DisplayTestMethodName]
        public void UpdateManifestNoChanges()
        {
            this.CreateIndexHelperForIndexTest((wrapper) =>
            {
                // Add manifest.
                string addManifest = Path.Combine(this.indexTestDataPath, PackageTest);
                wrapper.AddManifest(addManifest, PackageTestRelativePath);

                // Update manifest. Same file, should succeed but return false.
                Assert.False(wrapper.UpdateManifest(addManifest, PackageTestRelativePath));

                return true;
            });
        }

        /// <summary>
        /// Verify that updating a manifest that doesn't exists fails.
        /// </summary>
        [Fact]
        [DisplayTestMethodName]
        public void UpdateManifestNonExistant()
        {
            this.CreateIndexHelperForIndexTest((wrapper) =>
            {
                // Update manifest that doesn't exists
                string updateManifest = Path.Combine(this.indexTestDataPath, PackageTest);
                var exception = Assert.Throws<WinGetSQLiteIndexException>(
                    () =>
                    {
                        wrapper.UpdateManifest(updateManifest, PackageTestRelativePath);
                    });
                Assert.NotNull(exception.InnerException);
                Assert.True(exception.InnerException is System.Runtime.InteropServices.COMException);
                Assert.Equal(-2147023728, exception.InnerException.HResult);

                return true;
            });
        }

        /// <summary>
        /// Verify that updating an existing manifest in the index, but another version.
        /// </summary>
        [Fact]
        [DisplayTestMethodName]
        public void UpdateManifestDifferentVersion()
        {
            this.CreateIndexHelperForIndexTest((wrapper) =>
            {
                // Add manifest.
                string addManifest = Path.Combine(this.indexTestDataPath, PackageTest);
                wrapper.AddManifest(addManifest, PackageTestRelativePath);

                // Update manifest. Version is different.
                string updateManifest = Path.Combine(this.indexTestDataPath, PackageTestNewVersion);
                var exception = Assert.Throws<WinGetSQLiteIndexException>(
                    () =>
                    {
                        wrapper.UpdateManifest(updateManifest, PackageTestRelativePath);
                    });
                Assert.NotNull(exception.InnerException);
                Assert.True(exception.InnerException is System.Runtime.InteropServices.COMException);
                Assert.Equal(-2147023728, exception.InnerException.HResult);

                return true;
            });
        }

        /// <summary>
        /// Verify that removing a manifest in the index succeeds.
        /// </summary>
        [Fact]
        [DisplayTestMethodName]
        public void RemoveManifest()
        {
            this.CreateIndexHelperForIndexTest((wrapper) =>
            {
                // Add manifest.
                string addManifest = Path.Combine(this.indexTestDataPath, PackageTest);
                wrapper.AddManifest(addManifest, PackageTestRelativePath);

                // Remove manifest.
                wrapper.RemoveManifest(addManifest, PackageTestRelativePath);

                return true;
            });
        }

        /// <summary>
        /// Verify that trying to delete a manifest that doesn't exists fails.
        /// </summary>
        [Fact]
        [DisplayTestMethodName]
        public void RemoveManifestNonExistant()
        {
            // create index, add manifest, delete.
            this.CreateIndexHelperForIndexTest((wrapper) =>
            {
                // Add manifest.
                string addManifest = Path.Combine(this.indexTestDataPath, PackageTest);

                // Remove manifest.
                Assert.Throws<WinGetSQLiteIndexException>(
                    () =>
                    {
                        wrapper.RemoveManifest(addManifest, PackageTestRelativePath);
                    });

                return true;
            });
        }

        private void CreateIndexHelperForIndexTest(Func<IWinGetSQLiteIndex, bool> lambda)
        {
            if (File.Exists(this.indexTestLogFile))
            {
                File.Delete(this.indexTestLogFile);
            }

            if (File.Exists(this.indexTestFilePath))
            {
                File.Delete(this.indexTestFilePath);
            }

            // Create index.
            var factory = new WinGetFactory();
            using var log = factory.LoggingInit(this.indexTestLogFile);
            using var wrapper = factory.SQLiteIndexCreateLatestVersion(this.indexTestFilePath);
            Assert.True(lambda(wrapper), "Expression passed");

            Assert.True(File.Exists(this.indexTestLogFile));
            Assert.True(File.Exists(this.indexTestFilePath));
        }

        private void OpenIndexHelper(Func<IWinGetSQLiteIndex, bool> lambda)
        {
            Assert.True(File.Exists(this.indexTestFilePath));
            if (File.Exists(this.indexTestLogFile))
            {
                File.Delete(this.indexTestLogFile);
            }

            // Open index.
            var factory = new WinGetFactory();
            using var log = factory.LoggingInit(this.indexTestLogFile);
            using var wrapper = factory.SQLiteIndexOpen(this.indexTestFilePath);
            Assert.True(lambda(wrapper), "Expression passed");

            Assert.True(File.Exists(this.indexTestLogFile));
        }
    }
}
