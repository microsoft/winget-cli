// -----------------------------------------------------------------------------
// <copyright file="ConfigurationMixedElevationTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Tests
{
    using System;
    using System.Threading.Tasks;
    using Microsoft.Management.Configuration.UnitTests.Fixtures;
    using Microsoft.Management.Configuration.UnitTests.Helpers;
    using Microsoft.VisualBasic;
    using Xunit;
    using Xunit.Abstractions;

    /// <summary>
    /// Unit tests for verifying the processor behavior for handling mixed elevation scenarios.
    /// </summary>
    [Collection("UnitTestCollection")]
    [OutOfProc]
    [OutOfProcOnly]
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
        /// Verifies that a set of units with mixed elevation can run successfully.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Fact]
        public async Task ApplyUnitsWithMixedElevation()
        {
            string resourceName = "E2ETestResource";
            string moduleName = "xE2ETestResource";
            Version version = new Version("0.0.0.1");

            ConfigurationSet configurationSet = this.ConfigurationSet();
            configurationSet.Metadata.Add(Helpers.Constants.DisableRunAsTestGuid, true);

            ConfigurationUnit elevationRequiredUnit = this.ConfigurationUnit();
            elevationRequiredUnit.Metadata.Add("securityContext", "elevated");
            elevationRequiredUnit.Metadata.Add("version", version.ToString());
            elevationRequiredUnit.Metadata.Add("module", moduleName);

            elevationRequiredUnit.Settings.Add("secretCode", "123456789");
            elevationRequiredUnit.Type = resourceName;
            elevationRequiredUnit.Intent = ConfigurationUnitIntent.Apply;

            ConfigurationUnit unit = this.ConfigurationUnit();
            unit.Metadata.Add("version", version.ToString());
            unit.Metadata.Add("module", moduleName);
            unit.Settings.Add("secretCode", "123456789");
            unit.Type = resourceName;
            unit.Intent = ConfigurationUnitIntent.Apply;

            configurationSet.Units = new ConfigurationUnit[] { elevationRequiredUnit, unit };

            IConfigurationSetProcessorFactory dynamicFactory = await this.fixture.ConfigurationStatics.CreateConfigurationSetProcessorFactoryAsync(Helpers.Constants.DynamicRuntimeHandlerIdentifier);

            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(dynamicFactory);

            ApplyConfigurationSetResult result = processor.ApplySet(configurationSet, ApplyConfigurationSetFlags.None);
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
        }

        /// <summary>
        /// Verifies that creating a high integrity unit processor for a non elevated unit should return an invalid operation result.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Fact]
        public async Task ApplyElevatedUnitNotInLimitationSet()
        {
            string resourceName = "E2ETestResource";
            string moduleName = "xE2ETestResource";
            Version version = new Version("0.0.0.1");

            ConfigurationSet configurationSet = this.ConfigurationSet();
            configurationSet.Metadata.Add(Helpers.Constants.DisableRunAsTestGuid, true);
            configurationSet.Metadata.Add(Helpers.Constants.ForceHighIntegrityUnitProcessorsTestGuid, true);

            ConfigurationUnit elevationRequiredUnit = this.ConfigurationUnit();
            elevationRequiredUnit.Metadata.Add("securityContext", "elevated");
            elevationRequiredUnit.Metadata.Add("version", version.ToString());
            elevationRequiredUnit.Metadata.Add("module", moduleName);

            elevationRequiredUnit.Settings.Add("secretCode", "123456789");
            elevationRequiredUnit.Type = resourceName;
            elevationRequiredUnit.Intent = ConfigurationUnitIntent.Apply;

            ConfigurationUnit nonElevatedUnit = this.ConfigurationUnit();
            nonElevatedUnit.Metadata.Add("version", version.ToString());
            nonElevatedUnit.Metadata.Add("module", moduleName);

            nonElevatedUnit.Settings.Add("secretCode", "123456789");
            nonElevatedUnit.Type = resourceName;
            nonElevatedUnit.Intent = ConfigurationUnitIntent.Apply;

            configurationSet.Units = new ConfigurationUnit[] { elevationRequiredUnit, nonElevatedUnit };

            IConfigurationSetProcessorFactory dynamicFactory = await this.fixture.ConfigurationStatics.CreateConfigurationSetProcessorFactoryAsync(Helpers.Constants.DynamicRuntimeHandlerIdentifier);

            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(dynamicFactory);

            ApplyConfigurationSetResult result = processor.ApplySet(configurationSet, ApplyConfigurationSetFlags.None);
            Assert.NotNull(result);
            Assert.NotNull(result.ResultCode);
            Assert.Equal(Errors.WINGET_CONFIG_ERROR_SET_APPLY_FAILED, result.ResultCode.HResult);
            Assert.Equal(2, result.UnitResults.Count);

            // First unit should succeed since it is a unit that requires elevation.
            ApplyConfigurationUnitResult firstUnitResult = result.UnitResults[0];
            Assert.NotNull(firstUnitResult);
            Assert.False(firstUnitResult.PreviouslyInDesiredState);
            Assert.False(firstUnitResult.RebootRequired);
            Assert.NotNull(firstUnitResult.ResultInformation);
            Assert.Null(firstUnitResult.ResultInformation.ResultCode);
            Assert.Equal(ConfigurationUnitResultSource.None, firstUnitResult.ResultInformation.ResultSource);

            // Second unit should fail as it was not included in the limitation set.
            ApplyConfigurationUnitResult secondUnitResult = result.UnitResults[1];
            Assert.NotNull(secondUnitResult);
            Assert.False(secondUnitResult.PreviouslyInDesiredState);
            Assert.False(secondUnitResult.RebootRequired);
            Assert.NotNull(secondUnitResult.ResultInformation);
            Assert.NotNull(secondUnitResult.ResultInformation.ResultCode);
            Assert.Equal(Errors.COR_E_INVALIDOPERATION, secondUnitResult.ResultInformation.ResultCode.HResult);
            Assert.Equal(ConfigurationUnitResultSource.None, secondUnitResult.ResultInformation.ResultSource);
        }
    }
}
