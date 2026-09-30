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

        // The first schema version that can produce a delta index.
        private const uint DeltaMajorVersion = 2;
        private const uint DeltaMinorVersion = 1;

        // Opaque to the index; only the publishing service knows how its baselines are laid out.
        private const string BaselineRelativeSourcePath = "baselines/1.2.3.4/baseline.msix";
        private const string BaselinePackageVersion = "1.2.3.4";

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
        ///  Verify that add or update works both times.
        /// </summary>
        [Fact]
        [DisplayTestMethodName]
        public void AddOrUpdateManifest()
        {
            this.CreateIndexHelperForIndexTest((wrapper) =>
            {
                // Add manifest; should return true.
                string addManifest = Path.Combine(this.indexTestDataPath, PackageTest);
                Assert.True(wrapper.AddOrUpdateManifest(addManifest, PackageTestRelativePath));

                // Update manifest. name is different, should return false.
                string updateManifest = Path.Combine(this.indexTestDataPath, PackageTestNewName);
                Assert.False(wrapper.AddOrUpdateManifest(updateManifest, PackageTestRelativePath));

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

        /// <summary>
        /// Verifies that a delta index can be generated through the public API surface.
        /// This covers every delta property, including designating a baseline in the same prepare
        /// that produces its empty delta.
        /// </summary>
        [Fact]
        [DisplayTestMethodName]
        public void GenerateDeltaIndex()
        {
            string workingPath = Path.Combine(this.indexTestOutputPath, "working.db");
            string baselinePath = Path.Combine(this.indexTestOutputPath, "baseline.db");
            string baselineDeltaPath = Path.Combine(this.indexTestOutputPath, "baseline_delta.db");
            string deltaPath = Path.Combine(this.indexTestOutputPath, "delta.db");

            var factory = new WinGetFactory();
            using var log = factory.LoggingInit(this.indexTestLogFile);

            // The working index accumulates changes; a copy of it becomes the baseline, so that the
            // two share the database lineage that generation requires.
            using (var working = factory.SQLiteIndexCreate(workingPath, DeltaMajorVersion, DeltaMinorVersion))
            {
                working.SetProperty(SQLiteIndexProperty.PackageUpdateTrackingBaseTime, "0");
                working.AddManifest(Path.Combine(this.indexTestDataPath, PackageTest), PackageTestRelativePath);
            }

            File.Copy(workingPath, baselinePath);

            // Designating the baseline and producing the delta that describes it is a single
            // prepare; the delta is empty because the index is its own baseline.
            using (var baseline = factory.SQLiteIndexOpen(baselinePath))
            {
                baseline.SetProperty(SQLiteIndexProperty.DeltaMarkAsBaseline, "true");
                baseline.SetProperty(SQLiteIndexProperty.DeltaOutputPath, baselineDeltaPath);
                baseline.SetProperty(SQLiteIndexProperty.DeltaBaselineRelativeSourcePath, BaselineRelativeSourcePath);
                baseline.SetProperty(SQLiteIndexProperty.DeltaBaselinePackageVersion, BaselinePackageVersion);
                baseline.PrepareForPackaging();
            }

            Assert.True(File.Exists(baselineDeltaPath), "The baseline's own delta index was written.");

            using (var working = factory.SQLiteIndexOpen(workingPath))
            {
                working.SetProperty(SQLiteIndexProperty.PackageUpdateTrackingBaseTime, string.Empty);
                Assert.True(working.UpdateManifest(Path.Combine(this.indexTestDataPath, PackageTestNewName), PackageTestRelativePath));
            }

            using (var working = factory.SQLiteIndexOpen(workingPath))
            {
                working.SetProperty(SQLiteIndexProperty.DeltaBaselineIndexPath, baselinePath);
                working.SetProperty(SQLiteIndexProperty.DeltaOutputPath, deltaPath);
                working.SetProperty(SQLiteIndexProperty.DeltaBaselineRelativeSourcePath, BaselineRelativeSourcePath);
                working.SetProperty(SQLiteIndexProperty.DeltaBaselinePackageVersion, BaselinePackageVersion);
                working.PrepareForPackaging();
            }

            Assert.True(File.Exists(deltaPath), "The delta index was written.");
            Assert.True(new FileInfo(deltaPath).Length > 0, "The delta index is not empty.");
        }

        /// <summary>
        /// Verifies that naming a baseline and designating one are mutually exclusive, and that the
        /// contradiction is reported rather than resolved.
        /// </summary>
        [Fact]
        [DisplayTestMethodName]
        public void DeltaBaselineSourcesAreExclusive()
        {
            string workingPath = Path.Combine(this.indexTestOutputPath, "exclusive.db");
            string baselinePath = Path.Combine(this.indexTestOutputPath, "exclusive_baseline.db");
            string deltaPath = Path.Combine(this.indexTestOutputPath, "exclusive_delta.db");

            var factory = new WinGetFactory();
            using var log = factory.LoggingInit(this.indexTestLogFile);
            using var working = factory.SQLiteIndexCreate(workingPath, DeltaMajorVersion, DeltaMinorVersion);

            working.SetProperty(SQLiteIndexProperty.PackageUpdateTrackingBaseTime, "0");
            working.AddManifest(Path.Combine(this.indexTestDataPath, PackageTest), PackageTestRelativePath);

            working.SetProperty(SQLiteIndexProperty.DeltaBaselineIndexPath, baselinePath);
            working.SetProperty(SQLiteIndexProperty.DeltaMarkAsBaseline, "true");
            working.SetProperty(SQLiteIndexProperty.DeltaOutputPath, deltaPath);
            working.SetProperty(SQLiteIndexProperty.DeltaBaselineRelativeSourcePath, BaselineRelativeSourcePath);
            working.SetProperty(SQLiteIndexProperty.DeltaBaselinePackageVersion, BaselinePackageVersion);

            var exception = Assert.Throws<WinGetSQLiteIndexException>(() => working.PrepareForPackaging());
            Assert.NotNull(exception.InnerException);

            Assert.False(File.Exists(deltaPath), "Nothing was written for a contradictory request.");
        }

        /// <summary>
        /// Verifies that the designation property only accepts true, since an index is either being
        /// designated as a baseline or the property is simply not set.
        /// </summary>
        [Fact]
        [DisplayTestMethodName]
        public void DeltaMarkAsBaselineRejectsOtherValues()
        {
            string workingPath = Path.Combine(this.indexTestOutputPath, "mark_value.db");

            var factory = new WinGetFactory();
            using var log = factory.LoggingInit(this.indexTestLogFile);
            using var working = factory.SQLiteIndexCreate(workingPath, DeltaMajorVersion, DeltaMinorVersion);

            Assert.Throws<WinGetSQLiteIndexException>(() => working.SetProperty(SQLiteIndexProperty.DeltaMarkAsBaseline, "false"));
            Assert.Throws<WinGetSQLiteIndexException>(() => working.SetProperty(SQLiteIndexProperty.DeltaMarkAsBaseline, string.Empty));

            working.SetProperty(SQLiteIndexProperty.DeltaMarkAsBaseline, "TRUE");
        }

        /// <summary>
        /// Verifies that a delta can be checked for consistency through the public API surface, both
        /// on its own and as the merged result of the delta, its baseline, and the standard index
        /// built from the same data. This is the only place the new property value is exercised as a
        /// caller would reach it.
        /// </summary>
        [Fact]
        [DisplayTestMethodName]
        public void CheckDeltaIndexConsistency()
        {
            string workingPath = Path.Combine(this.indexTestOutputPath, "consistency_working.db");
            string baselinePath = Path.Combine(this.indexTestOutputPath, "consistency_baseline.db");
            string baselineDeltaPath = Path.Combine(this.indexTestOutputPath, "consistency_baseline_delta.db");
            string deltaPath = Path.Combine(this.indexTestOutputPath, "consistency_delta.db");

            var factory = new WinGetFactory();
            using var log = factory.LoggingInit(this.indexTestLogFile);

            using (var working = factory.SQLiteIndexCreate(workingPath, DeltaMajorVersion, DeltaMinorVersion))
            {
                working.SetProperty(SQLiteIndexProperty.PackageUpdateTrackingBaseTime, "0");
                working.AddManifest(Path.Combine(this.indexTestDataPath, PackageTest), PackageTestRelativePath);
            }

            File.Copy(workingPath, baselinePath);

            using (var baseline = factory.SQLiteIndexOpen(baselinePath))
            {
                baseline.SetProperty(SQLiteIndexProperty.DeltaMarkAsBaseline, "true");
                baseline.SetProperty(SQLiteIndexProperty.DeltaOutputPath, baselineDeltaPath);
                baseline.SetProperty(SQLiteIndexProperty.DeltaBaselineRelativeSourcePath, BaselineRelativeSourcePath);
                baseline.SetProperty(SQLiteIndexProperty.DeltaBaselinePackageVersion, BaselinePackageVersion);
                baseline.PrepareForPackaging();
            }

            using (var working = factory.SQLiteIndexOpen(workingPath))
            {
                working.SetProperty(SQLiteIndexProperty.PackageUpdateTrackingBaseTime, string.Empty);
                Assert.True(working.UpdateManifest(Path.Combine(this.indexTestDataPath, PackageTestNewName), PackageTestRelativePath));
            }

            using (var working = factory.SQLiteIndexOpen(workingPath))
            {
                working.SetProperty(SQLiteIndexProperty.DeltaBaselineIndexPath, baselinePath);
                working.SetProperty(SQLiteIndexProperty.DeltaOutputPath, deltaPath);
                working.SetProperty(SQLiteIndexProperty.DeltaBaselineRelativeSourcePath, BaselineRelativeSourcePath);
                working.SetProperty(SQLiteIndexProperty.DeltaBaselinePackageVersion, BaselinePackageVersion);
                working.PrepareForPackaging();

                // The working index is an ordinary one, even though it produced a delta.
                Assert.True(working.IsIndexConsistent(), "The index that generated the delta is consistent.");
            }

            using (var delta = factory.SQLiteIndexOpen(deltaPath))
            {
                Assert.True(delta.IsIndexConsistent(), "The delta alone is consistent.");
            }

            using (var delta = factory.SQLiteIndexOpen(deltaPath))
            {
                delta.SetProperty(SQLiteIndexProperty.DeltaBaselineIndexPath, baselinePath);

                Assert.True(delta.IsIndexConsistent(), "The merged view is consistent.");
            }

            using (var delta = factory.SQLiteIndexOpen(deltaPath))
            {
                delta.SetProperty(SQLiteIndexProperty.DeltaBaselineIndexPath, baselinePath);
                delta.SetProperty(SQLiteIndexProperty.DeltaComparisonIndexPath, workingPath);

                Assert.True(delta.IsIndexConsistent(), "The merged delta matches the standard index.");
            }
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
