// -----------------------------------------------------------------------------
// <copyright file="ConfigurationProcessorGetAllTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Tests
{
    using System.Collections.Generic;
    using System.IO;
    using Microsoft.Management.Configuration.UnitTests.Fixtures;
    using Microsoft.Management.Configuration.UnitTests.Helpers;
    using Microsoft.VisualBasic;
    using Windows.Foundation.Collections;
    using Xunit;
    using Xunit.Abstractions;

    /// <summary>
    /// Unit tests for getting details on processors.
    /// </summary>
    [Collection("UnitTestCollection")]
    [InProc]
    [OutOfProc]
    public class ConfigurationProcessorGetAllTests : ConfigurationProcessorTestBase
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="ConfigurationProcessorGetAllTests"/> class.
        /// </summary>
        /// <param name="fixture">Unit test fixture.</param>
        /// <param name="log">Log helper.</param>
        public ConfigurationProcessorGetAllTests(UnitTestFixture fixture, ITestOutputHelper log)
            : base(fixture, log)
        {
        }

        /// <summary>
        /// The unit settings processor returns an error HRESULT.
        /// </summary>
        [Fact]
        public void GetAllSettings_ProcessorSettingsError()
        {
            ConfigurationUnit configurationUnit = this.ConfigurationUnit();

            TestConfigurationProcessorFactory factory = new TestConfigurationProcessorFactory();
            factory.NullProcessor = new TestConfigurationSetProcessor(null);
            var unitProcessor = factory.NullProcessor.CreateGetAllSettingsTestProcessor(configurationUnit);
            unitProcessor.GetAllSettingsDelegate = () => throw new FileNotFoundException();

            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(factory);

            GetAllConfigurationUnitSettingsResult result = processor.GetAllUnitSettings(configurationUnit);

            Assert.NotNull(result);
            Assert.Null(result.Settings);
            Assert.NotNull(result.ResultInformation);
            Assert.NotNull(result.ResultInformation.ResultCode);
            Assert.IsType<FileNotFoundException>(result.ResultInformation.ResultCode);
        }

        /// <summary>
        /// The unit settings processor returns an error in its result.
        /// </summary>
        [Fact]
        public void GetAllSettings_ProcessorSettingsFailedResult()
        {
            ConfigurationUnit configurationUnit = this.ConfigurationUnit();

            TestConfigurationProcessorFactory factory = new TestConfigurationProcessorFactory();
            factory.NullProcessor = new TestConfigurationSetProcessor(null);
            var unitProcessor = factory.NullProcessor.CreateGetAllSettingsTestProcessor(configurationUnit);

            GetAllSettingsResultInstance getAllSettingsResult = new GetAllSettingsResultInstance(configurationUnit);
            getAllSettingsResult.InternalResult.ResultCode = new InvalidDataException();
            getAllSettingsResult.InternalResult.Description = "We fail because we must";
            unitProcessor.GetAllSettingsDelegate = () => getAllSettingsResult;

            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(factory);

            GetAllConfigurationUnitSettingsResult result = processor.GetAllUnitSettings(configurationUnit);

            Assert.NotNull(result);
            Assert.Null(result.Settings);
            Assert.NotNull(result.ResultInformation);
            Assert.Equal(getAllSettingsResult.ResultInformation.ResultCode.HResult, result.ResultInformation.ResultCode.HResult);
            Assert.Equal(getAllSettingsResult.ResultInformation.Description, result.ResultInformation.Description);
        }

        /// <summary>
        /// The unit settings processor returns good settings.
        /// </summary>
        [Fact]
        public void GetAllSettings_ProcessorSettingsSuccess()
        {
            ConfigurationUnit configurationUnit = this.ConfigurationUnit();

            TestConfigurationProcessorFactory factory = new TestConfigurationProcessorFactory();
            factory.NullProcessor = new TestConfigurationSetProcessor(null);
            var unitProcessor = factory.NullProcessor.CreateGetAllSettingsTestProcessor(configurationUnit);

            GetAllSettingsResultInstance getAllSettingsResult = new GetAllSettingsResultInstance(configurationUnit);
            getAllSettingsResult.Settings = new List<ValueSet>()
            {
                new ValueSet
                {
                    { "key", "value" },
                },
            };
            unitProcessor.GetAllSettingsDelegate = () => getAllSettingsResult;

            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(factory);

            GetAllConfigurationUnitSettingsResult result = processor.GetAllUnitSettings(configurationUnit);

            Assert.NotNull(result);
            Assert.NotNull(result.ResultInformation);
            Assert.Null(result.ResultInformation.ResultCode);
            Assert.Empty(result.ResultInformation.Description);
            Assert.NotNull(result.Settings);
            Assert.Equal(1, result.Settings.Count);
            Assert.Contains("key", result.Settings[0]);
            Assert.IsType<string>(result.Settings[0]["key"]);
            Assert.Equal("value", (string)result.Settings[0]["key"]);
        }

        /// <summary>
        /// The unit settings processor does not support GetAllSettings.
        /// </summary>
        [Fact]
        public void GetAllSettings_UnsupportedByProcessor()
        {
            ConfigurationUnit configurationUnit = this.ConfigurationUnit();

            TestConfigurationProcessorFactory factory = new TestConfigurationProcessorFactory();
            factory.NullProcessor = new TestConfigurationSetProcessor(null);
            var unitProcessor = factory.NullProcessor.CreateTestProcessor(configurationUnit);

            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(factory);

            GetAllConfigurationUnitSettingsResult result = processor.GetAllUnitSettings(configurationUnit);

            Assert.NotNull(result);
            Assert.NotNull(result.ResultInformation);
            Assert.NotNull(result.ResultInformation.ResultCode);
            Assert.Equal(Errors.WINGET_CONFIG_ERROR_NOT_SUPPORTED_BY_PROCESSOR, result.ResultInformation.ResultCode.HResult);
        }
    }
}
