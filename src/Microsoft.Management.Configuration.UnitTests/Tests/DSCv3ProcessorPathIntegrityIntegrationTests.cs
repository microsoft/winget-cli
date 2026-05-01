// -----------------------------------------------------------------------------
// <copyright file="DSCv3ProcessorPathIntegrityIntegrationTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Tests
{
    using System;
    using System.Collections.Generic;
    using System.Threading.Tasks;
    using Microsoft.Management.Configuration.UnitTests.Fixtures;
    using Microsoft.Management.Configuration.UnitTests.Helpers;
    using Xunit;
    using Xunit.Abstractions;

    /// <summary>
    /// Integration tests for DSCv3 processor path integrity checks.
    /// These tests verify that hash verification catches mismatches when a custom DSC executable
    /// path is provided. Units run in-process (no elevation split) so limit mode is not involved.
    /// </summary>
    [Collection("UnitTestCollection")]
    [OutOfProc]
    public class DSCv3ProcessorPathIntegrityIntegrationTests : ConfigurationProcessorTestBase
    {
        private readonly UnitTestFixture fixture;
        private readonly ITestOutputHelper log;

        /// <summary>
        /// Initializes a new instance of the <see cref="DSCv3ProcessorPathIntegrityIntegrationTests"/> class.
        /// </summary>
        /// <param name="fixture">Unit test fixture.</param>
        /// <param name="log">Log helper.</param>
        public DSCv3ProcessorPathIntegrityIntegrationTests(UnitTestFixture fixture, ITestOutputHelper log)
            : base(fixture, log)
        {
            this.fixture = fixture;
            this.log = log;
        }

        /// <summary>
        /// Verifies that providing a wrong hash for a custom processor path causes the
        /// apply to fail with a hash mismatch error.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Fact]
        public async Task Apply_ProcessorPath_WrongHash_FailsWithHashMismatch()
        {
            using var tempFile = new TempFile(content: "test content for hash test");

            IConfigurationSetProcessorFactory dynamicFactory =
                await this.fixture.ConfigurationStatics.CreateConfigurationSetProcessorFactoryAsync(
                    Helpers.Constants.DSCv3DynamicRuntimeHandlerIdentifier);

            var factoryMap = (IDictionary<string, string>)dynamicFactory;
            factoryMap["DscExecutablePath"] = tempFile.FullFileName;
            factoryMap["DscExecutablePathHash"] = new string('0', 64); // Wrong hash.
            factoryMap["DscExecutablePathIsAlias"] = "false";

            ConfigurationSet configurationSet = this.ConfigurationSet();
            configurationSet.SchemaVersion = "0.3";
            configurationSet.Metadata.Add(Helpers.Constants.EnableDynamicFactoryTestMode, true);

            ConfigurationUnit unit = this.ConfigurationUnit();
            unit.Identifier = "testUnit";
            unit.Type = "TestResource/TestUnit";
            unit.Intent = ConfigurationUnitIntent.Apply;
            configurationSet.Units = new[] { unit };

            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(dynamicFactory);
            var ex = Assert.ThrowsAny<Exception>(() => processor.ApplySet(configurationSet, ApplyConfigurationSetFlags.None));
            Assert.Equal(Errors.WINGET_CONFIG_ERROR_PROCESSOR_HASH_MISMATCH, ex.HResult);
        }

        /// <summary>
        /// Verifies that omitting the hash for a custom processor path causes the apply
        /// to fail. The server requires a hash whenever a custom path is provided.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Fact]
        public async Task Apply_ProcessorPath_MissingHash_ApplyFails()
        {
            using var tempFile = new TempFile(content: "test content");

            IConfigurationSetProcessorFactory dynamicFactory =
                await this.fixture.ConfigurationStatics.CreateConfigurationSetProcessorFactoryAsync(
                    Helpers.Constants.DSCv3DynamicRuntimeHandlerIdentifier);

            var factoryMap = (IDictionary<string, string>)dynamicFactory;

            // DscExecutablePathHash intentionally omitted.
            factoryMap["DscExecutablePath"] = tempFile.FullFileName;

            ConfigurationSet configurationSet = this.ConfigurationSet();
            configurationSet.SchemaVersion = "0.3";
            configurationSet.Metadata.Add(Helpers.Constants.EnableDynamicFactoryTestMode, true);

            ConfigurationUnit unit = this.ConfigurationUnit();
            unit.Identifier = "testUnit";
            unit.Type = "TestResource/TestUnit";
            unit.Intent = ConfigurationUnitIntent.Apply;
            configurationSet.Units = new[] { unit };

            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(dynamicFactory);
            Assert.ThrowsAny<Exception>(() => processor.ApplySet(configurationSet, ApplyConfigurationSetFlags.None));
        }
    }
}
