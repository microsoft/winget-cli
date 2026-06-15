// -----------------------------------------------------------------------------
// <copyright file="ConfigurationProcessorGetTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Tests
{
    using System.IO;
    using Microsoft.Management.Configuration.UnitTests.Fixtures;
    using Microsoft.Management.Configuration.UnitTests.Helpers;
    using Microsoft.VisualBasic;
    using Xunit;
    using Xunit.Abstractions;

    /// <summary>
    /// Unit tests for getting details on processors.
    /// </summary>
    [Collection("UnitTestCollection")]
    [InProc]
    [OutOfProc]
    public class ConfigurationProcessorGetTests : ConfigurationProcessorTestBase
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="ConfigurationProcessorGetTests"/> class.
        /// </summary>
        /// <param name="fixture">Unit test fixture.</param>
        /// <param name="log">Log helper.</param>
        public ConfigurationProcessorGetTests(UnitTestFixture fixture, ITestOutputHelper log)
            : base(fixture, log)
        {
        }

        /// <summary>
        /// Getting unit details throws an error.
        /// </summary>
        [Fact]
        public void GetUnitDetailsError()
        {
            ConfigurationUnit configurationUnitThrows = this.ConfigurationUnit();

            TestConfigurationProcessorFactory factory = new TestConfigurationProcessorFactory();
            factory.NullProcessor = new TestConfigurationSetProcessor(null);
            var thrownException = new FileNotFoundException();
            factory.NullProcessor.Exceptions.Add(configurationUnitThrows, thrownException);

            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(factory);

            GetConfigurationUnitDetailsResult result = processor.GetUnitDetails(configurationUnitThrows, ConfigurationUnitDetailFlags.Local);

            Assert.Null(result.Details);
            Assert.Equal(configurationUnitThrows, result.Unit);
            Assert.Equal(thrownException.HResult, result.ResultInformation.ResultCode.HResult);
            Assert.Equal(ConfigurationUnitResultSource.Internal, result.ResultInformation.ResultSource);
        }

        /// <summary>
        /// Getting unit details retrieves a value.
        /// </summary>
        [Fact]
        public void GetUnitDetailsSuccess()
        {
            ConfigurationUnit configurationUnit = this.ConfigurationUnit();
            TestConfigurationProcessorFactory factory = new TestConfigurationProcessorFactory();

            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(factory);

            Assert.Null(configurationUnit.Details);
            processor.GetUnitDetails(configurationUnit, ConfigurationUnitDetailFlags.Local);
            Assert.NotNull(configurationUnit.Details);
        }

        /// <summary>
        /// Getting set details throws an error.
        /// </summary>
        [Fact]
        public void GetSetDetailsError()
        {
            ConfigurationSet configurationSet = this.ConfigurationSet();
            ConfigurationUnit configurationUnitWorks = this.ConfigurationUnit();
            ConfigurationUnit configurationUnitThrows = this.ConfigurationUnit();
            configurationSet.Units = new ConfigurationUnit[] { configurationUnitWorks, configurationUnitThrows };

            TestConfigurationProcessorFactory factory = new TestConfigurationProcessorFactory();
            TestConfigurationSetProcessor setProcessor = factory.CreateTestProcessor(configurationSet);
            var thrownException = new InvalidDataException();
            setProcessor.Exceptions.Add(configurationUnitThrows, thrownException);

            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(factory);

            GetConfigurationSetDetailsResult result = processor.GetSetDetails(configurationSet, ConfigurationUnitDetailFlags.Local);
            var unitResults = result.UnitResults;
            Assert.Equal(2, unitResults.Count);

            Assert.Equal(configurationUnitWorks, unitResults[0].Unit);
            Assert.Null(unitResults[0].ResultInformation.ResultCode);
            Assert.NotNull(configurationUnitWorks.Details);

            Assert.Equal(configurationUnitThrows, unitResults[1].Unit);
            Assert.NotNull(unitResults[1].ResultInformation.ResultCode);
            Assert.Equal(thrownException.HResult, unitResults[1].ResultInformation.ResultCode.HResult);
            Assert.Null(configurationUnitThrows.Details);
        }

        /// <summary>
        /// Getting set details retrieves all values.
        /// </summary>
        [Fact]
        public void GetSetDetailsSuccess()
        {
            ConfigurationSet configurationSet = this.ConfigurationSet();
            ConfigurationUnit configurationUnit1 = this.ConfigurationUnit();
            ConfigurationUnit configurationUnit2 = this.ConfigurationUnit();
            configurationSet.Units = new ConfigurationUnit[] { configurationUnit1, configurationUnit2 };

            TestConfigurationProcessorFactory factory = new TestConfigurationProcessorFactory();

            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(factory);

            processor.GetSetDetails(configurationSet, ConfigurationUnitDetailFlags.Local);
            Assert.NotNull(configurationUnit1.Details);
            Assert.NotNull(configurationUnit2.Details);
        }

        /// <summary>
        /// The unit settings processor returns an error HRESULT.
        /// </summary>
        [Fact]
        public void GetSettings_ProcessorSettingsError()
        {
            ConfigurationUnit configurationUnit = this.ConfigurationUnit();

            TestConfigurationProcessorFactory factory = new TestConfigurationProcessorFactory();
            factory.NullProcessor = new TestConfigurationSetProcessor(null);
            TestConfigurationUnitProcessor unitProcessor = factory.NullProcessor.CreateTestProcessor(configurationUnit);
            unitProcessor.GetSettingsDelegate = () => throw new FileNotFoundException();

            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(factory);

            GetConfigurationUnitSettingsResult result = processor.GetUnitSettings(configurationUnit);

            Assert.NotNull(result);
            Assert.NotNull(result.Settings);
            Assert.NotNull(result.ResultInformation);
            Assert.NotNull(result.ResultInformation.ResultCode);
            Assert.IsType<FileNotFoundException>(result.ResultInformation.ResultCode);
        }

        /// <summary>
        /// The unit settings processor returns an error in it's result.
        /// </summary>
        [Fact]
        public void GetSettings_ProcessorSettingsFailedResult()
        {
            ConfigurationUnit configurationUnit = this.ConfigurationUnit();

            TestConfigurationProcessorFactory factory = new TestConfigurationProcessorFactory();
            factory.NullProcessor = new TestConfigurationSetProcessor(null);
            TestConfigurationUnitProcessor unitProcessor = factory.NullProcessor.CreateTestProcessor(configurationUnit);
            GetSettingsResultInstance getSettingsResult = new GetSettingsResultInstance(configurationUnit);
            getSettingsResult.InternalResult.ResultCode = new InvalidDataException();
            getSettingsResult.InternalResult.Description = "We fail because we must";
            unitProcessor.GetSettingsDelegate = () => getSettingsResult;

            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(factory);

            GetConfigurationUnitSettingsResult result = processor.GetUnitSettings(configurationUnit);

            Assert.NotNull(result);
            Assert.Null(result.Settings);
            Assert.NotNull(result.ResultInformation);
            Assert.Equal(getSettingsResult.ResultInformation.ResultCode.HResult, result.ResultInformation.ResultCode.HResult);
            Assert.Equal(getSettingsResult.ResultInformation.Description, result.ResultInformation.Description);
        }

        /// <summary>
        /// The unit settings processor returns good settings.
        /// </summary>
        [Fact]
        public void GetSettings_ProcessorSettingsSuccess()
        {
            ConfigurationUnit configurationUnit = this.ConfigurationUnit();

            TestConfigurationProcessorFactory factory = new TestConfigurationProcessorFactory();
            factory.NullProcessor = new TestConfigurationSetProcessor(null);
            TestConfigurationUnitProcessor unitProcessor = factory.NullProcessor.CreateTestProcessor(configurationUnit);
            GetSettingsResultInstance getSettingsResult = new GetSettingsResultInstance(configurationUnit);
            getSettingsResult.Settings = new Windows.Foundation.Collections.ValueSet();
            getSettingsResult.Settings.Add("key", "value");
            unitProcessor.GetSettingsDelegate = () => getSettingsResult;

            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(factory);

            GetConfigurationUnitSettingsResult result = processor.GetUnitSettings(configurationUnit);

            Assert.NotNull(result);
            Assert.NotNull(result.ResultInformation);
            Assert.Null(result.ResultInformation.ResultCode);
            Assert.Empty(result.ResultInformation.Description);
            Assert.NotNull(result.Settings);
            Assert.NotEmpty(result.Settings);
            Assert.Contains("key", result.Settings);
            Assert.IsType<string>(result.Settings["key"]);
            Assert.Equal("value", (string)result.Settings["key"]);
        }
    }
}
