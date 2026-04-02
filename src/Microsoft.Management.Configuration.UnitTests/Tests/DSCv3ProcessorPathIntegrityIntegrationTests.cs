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
    /// Out-of-process integration tests for DSCv3 processor path integrity in the elevation split.
    /// These use EnableDynamicFactoryTestMode to avoid actual elevation, making them safe for ADO
    /// pipelines where the user is always admin.
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
        /// Verifies that providing a wrong hash for a custom processor path causes the elevated
        /// apply to fail with a hash mismatch error.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Fact]
        public async Task ElevatedApply_ProcessorPath_WrongHash_FailsWithHashMismatch()
        {
            using var tempFile = new TempFile(content: "test content for hash test");

            IConfigurationSetProcessorFactory dynamicFactory =
                await this.fixture.ConfigurationStatics.CreateConfigurationSetProcessorFactoryAsync(
                    Helpers.Constants.DynamicRuntimeHandlerIdentifier);

            var factoryMap = (IDictionary<string, string>)dynamicFactory;
            factoryMap["DscExecutablePath"] = tempFile.FullFileName;
            factoryMap["DscExecutablePathHash"] = new string('0', 64); // Wrong hash.
            factoryMap["DscExecutablePathIsAlias"] = "false";

            ConfigurationSet configurationSet = this.ConfigurationSet();
            configurationSet.SchemaVersion = "0.3";
            configurationSet.Environment.ProcessorIdentifier = "dscv3";
            configurationSet.Metadata.Add(Helpers.Constants.EnableDynamicFactoryTestMode, true);
            configurationSet.Metadata.Add(Helpers.Constants.ForceHighIntegrityLevelUnitsTestGuid, true);

            ConfigurationUnit elevatedUnit = this.ConfigurationUnit();
            elevatedUnit.Environment.Context = SecurityContext.Elevated;
            elevatedUnit.Type = "TestResource/TestUnit";
            elevatedUnit.Intent = ConfigurationUnitIntent.Apply;
            configurationSet.Units = new[] { elevatedUnit };

            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(dynamicFactory);
            ApplyConfigurationSetResult result = processor.ApplySet(configurationSet, ApplyConfigurationSetFlags.None);

            Assert.NotNull(result);
            Assert.Equal(1, result.UnitResults.Count);
            Assert.NotNull(result.UnitResults[0].ResultInformation.ResultCode);
            Assert.Equal(
                Errors.WINGET_CONFIG_ERROR_PROCESSOR_HASH_MISMATCH,
                result.UnitResults[0].ResultInformation.ResultCode!.HResult);
        }

        /// <summary>
        /// Verifies that omitting the hash for a custom processor path causes the elevated apply
        /// to fail. The server requires a hash whenever a custom path is provided.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Fact]
        public async Task ElevatedApply_ProcessorPath_MissingHash_ApplyFails()
        {
            using var tempFile = new TempFile(content: "test content");

            IConfigurationSetProcessorFactory dynamicFactory =
                await this.fixture.ConfigurationStatics.CreateConfigurationSetProcessorFactoryAsync(
                    Helpers.Constants.DynamicRuntimeHandlerIdentifier);

            var factoryMap = (IDictionary<string, string>)dynamicFactory;

            // DscExecutablePathHash intentionally omitted.
            factoryMap["DscExecutablePath"] = tempFile.FullFileName;

            ConfigurationSet configurationSet = this.ConfigurationSet();
            configurationSet.SchemaVersion = "0.3";
            configurationSet.Environment.ProcessorIdentifier = "dscv3";
            configurationSet.Metadata.Add(Helpers.Constants.EnableDynamicFactoryTestMode, true);
            configurationSet.Metadata.Add(Helpers.Constants.ForceHighIntegrityLevelUnitsTestGuid, true);

            ConfigurationUnit elevatedUnit = this.ConfigurationUnit();
            elevatedUnit.Environment.Context = SecurityContext.Elevated;
            elevatedUnit.Type = "TestResource/TestUnit";
            elevatedUnit.Intent = ConfigurationUnitIntent.Apply;
            configurationSet.Units = new[] { elevatedUnit };

            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(dynamicFactory);
            ApplyConfigurationSetResult result = processor.ApplySet(configurationSet, ApplyConfigurationSetFlags.None);

            Assert.NotNull(result);
            bool anyFailure = result.ResultCode != null ||
                (result.UnitResults.Count > 0 && result.UnitResults[0].ResultInformation.ResultCode != null);
            Assert.True(anyFailure, "Expected apply to fail when processor path hash is missing.");
        }
    }
}
