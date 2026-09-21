// -----------------------------------------------------------------------------
// <copyright file="DSCv3ProcessorPathIntegrityUnitTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Tests
{
    using System;
    using System.IO;
    using Microsoft.Management.Configuration.Processor.DSCv3.Helpers;
    using Microsoft.Management.Configuration.UnitTests.Fixtures;
    using Microsoft.Management.Configuration.UnitTests.Helpers;
    using Xunit;
    using Xunit.Abstractions;

    /// <summary>
    /// In-process unit tests for DSCv3 processor path integrity helper methods.
    /// These exercise the helpers directly without going through the elevation split.
    /// </summary>
    [Collection("UnitTestCollection")]
    [InProc]
    public class DSCv3ProcessorPathIntegrityUnitTests : ConfigurationProcessorTestBase
    {
        private readonly UnitTestFixture fixture;
        private readonly ITestOutputHelper log;

        /// <summary>
        /// Initializes a new instance of the <see cref="DSCv3ProcessorPathIntegrityUnitTests"/> class.
        /// </summary>
        /// <param name="fixture">Unit test fixture.</param>
        /// <param name="log">Log helper.</param>
        public DSCv3ProcessorPathIntegrityUnitTests(UnitTestFixture fixture, ITestOutputHelper log)
            : base(fixture, log)
        {
            this.fixture = fixture;
            this.log = log;
        }

        /// <summary>
        /// Verifies that <see cref="ProcessorPathIntegrity.ComputeHash"/> returns a 64-char
        /// lowercase hex SHA256 hash for a regular file.
        /// </summary>
        [Fact]
        public void ComputeHash_RegularFile_ReturnsLowercaseHex64()
        {
            using var tempFile = new TempFile(content: "test content for hashing");

            string hash = ProcessorPathIntegrity.ComputeHash(tempFile.FullFileName, out bool isAlias);

            Assert.False(isAlias);
            Assert.Equal(64, hash.Length);
            Assert.Equal(hash, hash.ToLowerInvariant());
        }

        /// <summary>
        /// Verifies that two calls to <see cref="ProcessorPathIntegrity.ComputeHash"/> on the same
        /// file return the same hash.
        /// </summary>
        [Fact]
        public void ComputeHash_SameFile_ReturnsSameHash()
        {
            using var tempFile = new TempFile(content: "deterministic content");

            string hash1 = ProcessorPathIntegrity.ComputeHash(tempFile.FullFileName, out _);
            string hash2 = ProcessorPathIntegrity.ComputeHash(tempFile.FullFileName, out _);

            Assert.Equal(hash1, hash2);
        }

        /// <summary>
        /// Verifies that <see cref="ProcessorPathIntegrity.VerifyAndOpen"/> succeeds and returns a
        /// valid handle when the correct hash is supplied.
        /// </summary>
        [Fact]
        public void VerifyAndOpen_CorrectHash_ReturnsValidHandle()
        {
            using var tempFile = new TempFile(content: "test content");

            string hash = ProcessorPathIntegrity.ComputeHash(tempFile.FullFileName, out bool isAlias);
            using var handle = ProcessorPathIntegrity.VerifyAndOpen(tempFile.FullFileName, hash, isAlias);

            Assert.False(handle.IsInvalid);
        }

        /// <summary>
        /// Verifies that the handle returned by <see cref="ProcessorPathIntegrity.VerifyAndOpen"/>
        /// pins the file: it cannot be written, renamed or deleted, but it can still be opened for
        /// read, which is what is required to execute it.
        /// </summary>
        [Fact]
        public void VerifyAndOpen_PinsFileAgainstReplacement()
        {
            using var tempFile = new TempFile(content: "test content");

            string hash = ProcessorPathIntegrity.ComputeHash(tempFile.FullFileName, out bool isAlias);
            using var handle = ProcessorPathIntegrity.VerifyAndOpen(tempFile.FullFileName, hash, isAlias);

            using (var readStream = File.OpenRead(tempFile.FullFileName))
            {
                Assert.True(readStream.Length > 0);
            }

            Assert.ThrowsAny<IOException>(() => File.OpenWrite(tempFile.FullFileName));
            Assert.ThrowsAny<IOException>(() => File.Delete(tempFile.FullFileName));
            Assert.ThrowsAny<IOException>(() => File.Move(tempFile.FullFileName, tempFile.FullFileName + ".attacker"));
        }

        /// <summary>
        /// Verifies that <see cref="ProcessorPathIntegrity.VerifyAndOpen"/> throws with the
        /// hash mismatch HRESULT when the wrong hash is supplied.
        /// </summary>
        [Fact]
        public void VerifyAndOpen_WrongHash_ThrowsHashMismatch()
        {
            using var tempFile = new TempFile(content: "test content");

            Exception? ex = Record.Exception(() =>
            {
                using var handle = ProcessorPathIntegrity.VerifyAndOpen(tempFile.FullFileName, new string('0', 64), isAlias: false);
            });

            Assert.NotNull(ex);
            Assert.Equal(Errors.WINGET_CONFIG_ERROR_PROCESSOR_HASH_MISMATCH, ex.HResult);
        }

        /// <summary>
        /// Verifies that <see cref="ProcessorSettings.EffectiveDscExecutablePath"/> throws
        /// <see cref="InvalidOperationException"/> when a custom path is set without a hash.
        /// </summary>
        [Fact]
        public void ProcessorSettings_CustomPath_NoHash_Throws()
        {
            using var tempFile = new TempFile(content: "test content");

            var settings = new ProcessorSettings();
            settings.DscExecutablePath = tempFile.FullFileName;

            Exception? ex = Record.Exception(() => _ = settings.EffectiveDscExecutablePath);

            Assert.NotNull(ex);
            Assert.IsType<InvalidOperationException>(ex);
        }

        /// <summary>
        /// Verifies that <see cref="ProcessorSettings.EffectiveDscExecutablePath"/> returns the
        /// pinned path of the verified file when the correct hash is provided.
        /// </summary>
        [Fact]
        public void ProcessorSettings_CustomPath_CorrectHash_ReturnsPath()
        {
            using var tempFile = new TempFile(content: "test content");

            string hash = ProcessorPathIntegrity.ComputeHash(tempFile.FullFileName, out bool isAlias);

            using var settings = new ProcessorSettings();
            settings.DscExecutablePath = tempFile.FullFileName;
            settings.DscExecutablePathHash = hash;
            settings.DscExecutablePathIsAlias = isAlias;

            // The pinned path is the normalized path of the verified file, which is not necessarily
            // character for character the path that was provided (for example if it contained a
            // short name or a symbolic link).
            string launchPath = settings.EffectiveDscExecutablePath;

            Assert.Equal(Path.GetFileName(tempFile.FullFileName), Path.GetFileName(launchPath));
            Assert.True(File.Exists(launchPath));
            Assert.Equal(File.ReadAllText(tempFile.FullFileName), File.ReadAllText(launchPath));
        }

        /// <summary>
        /// Verifies that pinning the processor path also pins the directories that the path is
        /// composed of, so that the verified file cannot be swapped by renaming a directory.
        /// </summary>
        [Fact]
        public void ProcessorSettings_CustomPath_PinsAncestorDirectories()
        {
            using var tempDirectory = new TempDirectory();

            string processorDirectory = Path.Combine(tempDirectory.FullDirectoryPath, "processor");
            Directory.CreateDirectory(processorDirectory);

            string processorPath = Path.Combine(processorDirectory, "dsc.exe");
            File.WriteAllText(processorPath, "test content");

            string hash = ProcessorPathIntegrity.ComputeHash(processorPath, out bool isAlias);

            using var settings = new ProcessorSettings();
            settings.DscExecutablePath = processorPath;
            settings.DscExecutablePathHash = hash;
            settings.DscExecutablePathIsAlias = isAlias;

            Assert.True(File.Exists(settings.EffectiveDscExecutablePath));

            Assert.ThrowsAny<IOException>(() => Directory.Move(processorDirectory, processorDirectory + "-attacker"));
            Assert.ThrowsAny<IOException>(() => Directory.Move(tempDirectory.FullDirectoryPath, tempDirectory.FullDirectoryPath + "-attacker"));
        }

        /// <summary>
        /// Verifies that copies of the settings share a single pin, and that the pin is released
        /// only when the settings object that owns it is disposed.
        /// </summary>
        [Fact]
        public void ProcessorSettings_Clone_SharesSinglePin()
        {
            using var tempDirectory = new TempDirectory();

            string processorPath = Path.Combine(tempDirectory.FullDirectoryPath, "dsc.exe");
            File.WriteAllText(processorPath, "test content");

            string hash = ProcessorPathIntegrity.ComputeHash(processorPath, out bool isAlias);

            var settings = new ProcessorSettings();
            settings.DscExecutablePath = processorPath;
            settings.DscExecutablePathHash = hash;
            settings.DscExecutablePathIsAlias = isAlias;

            ProcessorSettings copy = settings.Clone();

            Assert.Equal(settings.EffectiveDscExecutablePath, copy.EffectiveDscExecutablePath);

            // The copy does not own the pin, so disposing it leaves the file pinned.
            copy.Dispose();
            Assert.ThrowsAny<IOException>(() => File.Delete(processorPath));

            // Disposing the owner releases both the file and the directory pins.
            settings.Dispose();
            File.Delete(processorPath);
            Directory.Move(tempDirectory.FullDirectoryPath, tempDirectory.FullDirectoryPath + "-moved");
            Directory.Move(tempDirectory.FullDirectoryPath + "-moved", tempDirectory.FullDirectoryPath);
        }

        /// <summary>
        /// Verifies that <see cref="ProcessorSettings.EffectiveDscExecutablePath"/> throws with the
        /// hash mismatch HRESULT when a wrong hash is set for a custom path.
        /// </summary>
        [Fact]
        public void ProcessorSettings_CustomPath_WrongHash_ThrowsHashMismatch()
        {
            using var tempFile = new TempFile(content: "test content");

            var settings = new ProcessorSettings();
            settings.DscExecutablePath = tempFile.FullFileName;
            settings.DscExecutablePathHash = new string('0', 64);
            settings.DscExecutablePathIsAlias = false;

            Exception? ex = Record.Exception(() => _ = settings.EffectiveDscExecutablePath);

            Assert.NotNull(ex);
            Assert.Equal(Errors.WINGET_CONFIG_ERROR_PROCESSOR_HASH_MISMATCH, ex.HResult);
        }
    }
}
