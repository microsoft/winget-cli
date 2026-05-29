// -----------------------------------------------------------------------------
// <copyright file="DSCv3ProcessorPathIntegrityUnitTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Tests
{
    using System;
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
        /// path when the correct hash is provided.
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

            Assert.Equal(tempFile.FullFileName, settings.EffectiveDscExecutablePath);
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
