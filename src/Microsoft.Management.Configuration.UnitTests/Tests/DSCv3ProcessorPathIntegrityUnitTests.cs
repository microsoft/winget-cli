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
        /// valid pin when the correct hash is supplied.
        /// </summary>
        [Fact]
        public void VerifyAndOpen_CorrectHash_ReturnsValidHandle()
        {
            using var tempFile = new TempFile(content: "test content");

            string hash = ProcessorPathIntegrity.ComputeHash(tempFile.FullFileName, out bool isAlias);
            using var pinnedFile = ProcessorPathIntegrity.VerifyAndOpen(tempFile.FullFileName, hash, isAlias);

            Assert.False(pinnedFile.NameHandle.IsInvalid);
            Assert.False(pinnedFile.ContentHandle.IsInvalid);
        }

        /// <summary>
        /// Verifies that the pin returned by <see cref="ProcessorPathIntegrity.VerifyAndOpen"/>
        /// pins the file: it cannot be written, renamed or deleted, but it can still be opened for
        /// read, which is what is required to execute it.
        /// </summary>
        [Fact]
        public void VerifyAndOpen_PinsFileAgainstReplacement()
        {
            using var tempFile = new TempFile(content: "test content");

            string hash = ProcessorPathIntegrity.ComputeHash(tempFile.FullFileName, out bool isAlias);
            using var pinnedFile = ProcessorPathIntegrity.VerifyAndOpen(tempFile.FullFileName, hash, isAlias);

            using (var readStream = File.OpenRead(tempFile.FullFileName))
            {
                Assert.True(readStream.Length > 0);
            }

            Assert.ThrowsAny<IOException>(() => File.OpenWrite(tempFile.FullFileName));
            Assert.ThrowsAny<IOException>(() => File.Delete(tempFile.FullFileName));
            Assert.ThrowsAny<IOException>(() => File.Move(tempFile.FullFileName, tempFile.FullFileName + ".attacker"));
        }

        /// <summary>
        /// Verifies that a regular file is rejected when it is claimed to be an app execution alias.
        /// </summary>
        [Fact]
        public void VerifyAndOpen_RegularFileAsAlias_Throws()
        {
            using var tempFile = new TempFile(content: "test content");

            string hash = ProcessorPathIntegrity.ComputeHash(tempFile.FullFileName, out _);

            Exception? ex = Record.Exception(() =>
            {
                using var pinnedFile = ProcessorPathIntegrity.VerifyAndOpen(tempFile.FullFileName, hash, isAlias: true);
            });

            Assert.NotNull(ex);
            Assert.Equal(Errors.WINGET_CONFIG_ERROR_PROCESSOR_PATH_CHANGED, ex.HResult);
        }

        /// <summary>
        /// Verifies that a symbolic link is hashed through to its target and that pinning it holds
        /// both ends of the link, so that neither the link nor the file it resolves to can be
        /// changed while the processor path is in use.
        /// </summary>
        [Fact]
        public void VerifyAndOpen_SymbolicLink_PinsLinkAndTarget()
        {
            using var tempDirectory = new TempDirectory();

            string targetPath = Path.Combine(tempDirectory.FullDirectoryPath, "real-dsc.exe");
            File.WriteAllText(targetPath, "test content");

            string linkPath = Path.Combine(tempDirectory.FullDirectoryPath, "dsc.exe");
            if (!TryCreateSymbolicLink(linkPath, targetPath))
            {
                // Creating symbolic links requires developer mode or administrator rights.
                return;
            }

            string hash = ProcessorPathIntegrity.ComputeHash(linkPath, out bool isAlias);

            Assert.False(isAlias);
            Assert.Equal(ProcessorPathIntegrity.ComputeHash(targetPath, out _), hash);

            using (var pinnedFile = ProcessorPathIntegrity.VerifyAndOpen(linkPath, hash, isAlias))
            {
                // The link itself cannot be removed, replaced or retargeted; retargeting requires
                // write access to the link, which the pin denies.
                Assert.ThrowsAny<IOException>(() => File.Delete(linkPath));
                Assert.ThrowsAny<IOException>(() => File.Move(linkPath, linkPath + ".attacker"));
                Assert.ThrowsAny<IOException>(() => File.OpenWrite(linkPath));

                // The target cannot be modified, removed or replaced either.
                Assert.ThrowsAny<IOException>(() => File.OpenWrite(targetPath));
                Assert.ThrowsAny<IOException>(() => File.Delete(targetPath));
                Assert.ThrowsAny<IOException>(() => File.Move(targetPath, targetPath + ".attacker"));

                // The link still resolves, which is what execution requires.
                Assert.Equal("test content", File.ReadAllText(linkPath));
            }

            Exception? ex = Record.Exception(() =>
            {
                using var pinnedFile = ProcessorPathIntegrity.VerifyAndOpen(linkPath, hash, isAlias: true);
            });

            Assert.NotNull(ex);
            Assert.Equal(Errors.WINGET_CONFIG_ERROR_PROCESSOR_PATH_CHANGED, ex.HResult);
        }

        /// <summary>
        /// Verifies that the path used to launch the processor is the path of the file that was
        /// verified, rather than the symbolic link that pointed at it.
        /// </summary>
        [Fact]
        public void ProcessorSettings_SymbolicLink_LaunchesResolvedTarget()
        {
            using var tempDirectory = new TempDirectory();

            string targetPath = Path.Combine(tempDirectory.FullDirectoryPath, "real-dsc.exe");
            File.WriteAllText(targetPath, "test content");

            string linkPath = Path.Combine(tempDirectory.FullDirectoryPath, "dsc.exe");
            if (!TryCreateSymbolicLink(linkPath, targetPath))
            {
                return;
            }

            string hash = ProcessorPathIntegrity.ComputeHash(linkPath, out bool isAlias);

            using var settings = new ProcessorSettings();
            settings.DscExecutablePath = linkPath;
            settings.DscExecutablePathHash = hash;
            settings.DscExecutablePathIsAlias = isAlias;

            Assert.Equal(Path.GetFileName(targetPath), Path.GetFileName(settings.EffectiveDscExecutablePath));
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
        /// Verifies that a symbolic link in a directory component of the path is resolved away, so
        /// that the pinned directories are the real ones rather than the link.
        /// </summary>
        [Fact]
        public void ProcessorSettings_DirectoryLinkInPath_PinsResolvedDirectories()
        {
            using var tempDirectory = new TempDirectory();

            string realDirectory = Path.Combine(tempDirectory.FullDirectoryPath, "real");
            Directory.CreateDirectory(realDirectory);

            string processorPath = Path.Combine(realDirectory, "dsc.exe");
            File.WriteAllText(processorPath, "test content");

            string linkDirectory = Path.Combine(tempDirectory.FullDirectoryPath, "link");
            if (!TryCreateDirectorySymbolicLink(linkDirectory, realDirectory))
            {
                return;
            }

            string pathThroughLink = Path.Combine(linkDirectory, "dsc.exe");
            string hash = ProcessorPathIntegrity.ComputeHash(pathThroughLink, out bool isAlias);

            using var settings = new ProcessorSettings();
            settings.DscExecutablePath = pathThroughLink;
            settings.DscExecutablePathHash = hash;
            settings.DscExecutablePathIsAlias = isAlias;

            Assert.Equal(processorPath, settings.EffectiveDscExecutablePath, ignoreCase: true);

            // The real directory is the one that is pinned, not the link that was used to reach it.
            Assert.ThrowsAny<IOException>(() => Directory.Move(realDirectory, realDirectory + "-attacker"));
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

        /// <summary>
        /// Creating symbolic links requires developer mode or administrator rights, so the tests
        /// that use them are skipped when they cannot be created.
        /// </summary>
        /// <param name="path">The link to create.</param>
        /// <param name="target">The target of the link.</param>
        /// <returns>True if the link was created; false if links cannot be created.</returns>
        private static bool TryCreateSymbolicLink(string path, string target)
        {
            try
            {
                File.CreateSymbolicLink(path, target);
                return true;
            }
            catch (IOException)
            {
                return false;
            }
            catch (UnauthorizedAccessException)
            {
                return false;
            }
        }

        /// <summary>
        /// Creating symbolic links requires developer mode or administrator rights, so the tests
        /// that use them are skipped when they cannot be created.
        /// </summary>
        /// <param name="path">The link to create.</param>
        /// <param name="target">The target of the link.</param>
        /// <returns>True if the link was created; false if links cannot be created.</returns>
        private static bool TryCreateDirectorySymbolicLink(string path, string target)
        {
            try
            {
                Directory.CreateSymbolicLink(path, target);
                return true;
            }
            catch (IOException)
            {
                return false;
            }
            catch (UnauthorizedAccessException)
            {
                return false;
            }
        }
    }
}
