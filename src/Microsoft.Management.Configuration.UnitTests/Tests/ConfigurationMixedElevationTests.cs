// -----------------------------------------------------------------------------
// <copyright file="ConfigurationMixedElevationTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Tests
{
    using System;
    using System.IO;
    using System.Threading.Tasks;
    using Microsoft.Management.Configuration.UnitTests.Fixtures;
    using Microsoft.Management.Configuration.UnitTests.Helpers;
    using Xunit;
    using Xunit.Abstractions;

    /// <summary>
    /// Unit tests for verifying the processor behavior for handling mixed elevation scenarios.
    /// </summary>
    [Collection("UnitTestCollection")]
    [OutOfProc]
    public class ConfigurationMixedElevationTests : ConfigurationProcessorTestBase
    {
        private readonly UnitTestFixture fixture;
        private readonly ITestOutputHelper log;

        /// <summary>
        /// Initializes a new instance of the <see cref="ConfigurationMixedElevationTests"/> class.
        /// </summary>
        /// <param name="fixture">Unit test fixture.</param>
        /// <param name="log">Log helper.</param>
        public ConfigurationMixedElevationTests(UnitTestFixture fixture, ITestOutputHelper log)
            : base(fixture, log)
        {
            this.fixture = fixture;
            this.log = log;
        }

        /// <summary>
        /// Verifies that applying units of mixed elevation is successful. Also verifies that the elevated processor has a different process id.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Fact]
        public async Task ApplyMixedElevationUnits()
        {
            string resourceName = "E2ETestResourcePID";
            string moduleName = "xE2ETestResource";
            Version version = new Version("0.0.0.1");

            string tempDirectory = Path.Combine(Path.GetTempPath(), Path.GetRandomFileName());
            Directory.CreateDirectory(tempDirectory);

            ConfigurationSet configurationSet = this.ConfigurationSet();
            configurationSet.Metadata.Add(Helpers.Constants.EnableDynamicFactoryTestMode, true);

            ConfigurationUnit unit = this.ConfigurationUnit();
            unit.Metadata.Add("version", version.ToString());
            unit.Metadata.Add("module", moduleName);
            unit.Settings.Add("directoryPath", tempDirectory);
            unit.Type = resourceName;
            unit.Intent = ConfigurationUnitIntent.Apply;

            ConfigurationUnit elevatedUnit = this.ConfigurationUnit();
            elevatedUnit.Metadata.Add("version", version.ToString());
            elevatedUnit.Metadata.Add("module", moduleName);
            elevatedUnit.Metadata.Add("securityContext", "elevated");
            elevatedUnit.Settings.Add("directoryPath", tempDirectory);
            elevatedUnit.Type = resourceName;
            elevatedUnit.Intent = ConfigurationUnitIntent.Apply;

            configurationSet.Units = new ConfigurationUnit[] { unit, elevatedUnit };

            IConfigurationSetProcessorFactory dynamicFactory = await this.fixture.ConfigurationStatics.CreateConfigurationSetProcessorFactoryAsync(Helpers.Constants.DynamicRuntimeHandlerIdentifier);

            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(dynamicFactory);

            ApplyConfigurationSetResult result = processor.ApplySet(configurationSet, ApplyConfigurationSetFlags.None);

            // Get the number of unique PIDs from temp directory.
            int pidCount = Directory.GetFiles(tempDirectory).Length;

            // Clean up temp directory folder.
            Directory.Delete(tempDirectory, true);

            Assert.NotNull(result);
            Assert.Null(result.ResultCode);
            Assert.Equal(2, result.UnitResults.Count);

            foreach (var unitResult in result.UnitResults)
            {
                Assert.NotNull(unitResult);
                Assert.False(unitResult.PreviouslyInDesiredState);
                Assert.False(unitResult.RebootRequired);
                Assert.NotNull(unitResult.ResultInformation);
                Assert.Null(unitResult.ResultInformation.ResultCode);
                Assert.Equal(ConfigurationUnitResultSource.None, unitResult.ResultInformation.ResultSource);
            }

            // There should be exactly 2 unique PIDs, one for each integrity level.
            Assert.Equal(2, pidCount);
        }

        /// <summary>
        /// Verifies that creating a high integrity unit processor for a non elevated unit should return an invalid operation result.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Fact]
        public async Task ApplyUnitNotInLimitationSet()
        {
            string resourceName = "E2ETestResource";
            string moduleName = "xE2ETestResource";
            Version version = new Version("0.0.0.1");

            ConfigurationSet configurationSet = this.ConfigurationSet();
            configurationSet.Metadata.Add(Helpers.Constants.EnableDynamicFactoryTestMode, true);
            configurationSet.Metadata.Add(Helpers.Constants.ForceHighIntegrityLevelUnitsTestGuid, true);
            configurationSet.Metadata.Add(Helpers.Constants.EnableRestrictedIntegrityLevelTestGuid, true);

            ConfigurationUnit unit = this.ConfigurationUnit();
            unit.Metadata.Add("version", version.ToString());
            unit.Metadata.Add("module", moduleName);
            unit.Type = resourceName;
            unit.Intent = ConfigurationUnitIntent.Apply;

            ConfigurationUnit elevatedUnit = this.ConfigurationUnit();
            elevatedUnit.Metadata.Add("version", version.ToString());
            elevatedUnit.Metadata.Add("module", moduleName);
            elevatedUnit.Metadata.Add("securityContext", "elevated");
            elevatedUnit.Type = resourceName;
            elevatedUnit.Intent = ConfigurationUnitIntent.Apply;

            configurationSet.Units = new ConfigurationUnit[] { unit, elevatedUnit };

            IConfigurationSetProcessorFactory dynamicFactory = await this.fixture.ConfigurationStatics.CreateConfigurationSetProcessorFactoryAsync(Helpers.Constants.DynamicRuntimeHandlerIdentifier);

            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(dynamicFactory);

            ApplyConfigurationSetResult result = processor.ApplySet(configurationSet, ApplyConfigurationSetFlags.None);
            Assert.NotNull(result);
            Assert.NotNull(result.ResultCode);
            Assert.Equal(Errors.WINGET_CONFIG_ERROR_SET_APPLY_FAILED, result.ResultCode.HResult);
            Assert.Equal(2, result.UnitResults.Count);

            ApplyConfigurationUnitResult unitResult = result.UnitResults[0];
            Assert.NotNull(unitResult);
            Assert.False(unitResult.PreviouslyInDesiredState);
            Assert.False(unitResult.RebootRequired);
            Assert.NotNull(unitResult.ResultInformation);
            Assert.NotNull(unitResult.ResultInformation.ResultCode);
            Assert.Equal(Errors.CORE_INVALID_OPERATION, unitResult.ResultInformation.ResultCode.HResult);
            Assert.Equal(ConfigurationUnitResultSource.Internal, unitResult.ResultInformation.ResultSource);

            // Elevated unit should still succeed when applied.
            ApplyConfigurationUnitResult elevatedUnitResult = result.UnitResults[1];
            Assert.NotNull(elevatedUnitResult);
            Assert.False(elevatedUnitResult.PreviouslyInDesiredState);
            Assert.False(elevatedUnitResult.RebootRequired);
            Assert.NotNull(elevatedUnitResult.ResultInformation);
            Assert.Null(elevatedUnitResult.ResultInformation.ResultCode);
            Assert.Equal(ConfigurationUnitResultSource.None, elevatedUnitResult.ResultInformation.ResultSource);
        }
    }
}
