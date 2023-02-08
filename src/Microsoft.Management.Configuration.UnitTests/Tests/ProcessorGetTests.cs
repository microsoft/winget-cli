// -----------------------------------------------------------------------------
// <copyright file="ProcessorGetTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Tests
{
    using System.IO;
    using System.Runtime.InteropServices;
    using Microsoft.Management.Configuration.UnitTests.Fixtures;
    using Microsoft.Management.Configuration.UnitTests.Helpers;
    using Microsoft.VisualBasic;
    using Xunit;
    using Xunit.Abstractions;

    /// <summary>
    /// Unit tests for getting details on processors.
    /// </summary>
    [Collection("UnitTestCollection")]
    public class ProcessorGetTests : ConfigurationProcessorTestBase
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="ProcessorGetTests"/> class.
        /// </summary>
        /// <param name="fixture">Unit test fixture.</param>
        /// <param name="log">Log helper.</param>
        public ProcessorGetTests(UnitTestFixture fixture, ITestOutputHelper log)
            : base(fixture, log)
        {
        }

        /// <summary>
        /// Getting unit details throws an error.
        /// </summary>
        [Fact]
        public void GetUnitDetailsError()
        {
            ConfigurationUnit configurationUnitThrows = new ConfigurationUnit();

            TestConfigurationProcessorFactory factory = new TestConfigurationProcessorFactory();
            factory.NullProcessor = new TestConfigurationSetProcessor(null);
            factory.NullProcessor.Exceptions.Add(configurationUnitThrows, new FileNotFoundException());

            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(factory);

            Assert.Throws<FileNotFoundException>(() => processor.GetUnitDetails(configurationUnitThrows, ConfigurationUnitDetailLevel.Local));
        }

        /// <summary>
        /// Getting unit details retrieves a value.
        /// </summary>
        [Fact]
        public void GetUnitDetailsSuccess()
        {
            ConfigurationUnit configurationUnit = new ConfigurationUnit();
            TestConfigurationProcessorFactory factory = new TestConfigurationProcessorFactory();

            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(factory);

            Assert.Null(configurationUnit.Details);
            processor.GetUnitDetails(configurationUnit, ConfigurationUnitDetailLevel.Local);
            Assert.NotNull(configurationUnit.Details);
        }

        /// <summary>
        /// Getting set details throws an error.
        /// </summary>
        [Fact]
        public void GetSetDetailsError()
        {
            ConfigurationSet configurationSet = new ConfigurationSet();
            ConfigurationUnit configurationUnitWorks = new ConfigurationUnit();
            ConfigurationUnit configurationUnitThrows = new ConfigurationUnit();
            configurationSet.ConfigurationUnits = new ConfigurationUnit[] { configurationUnitWorks, configurationUnitThrows };

            TestConfigurationProcessorFactory factory = new TestConfigurationProcessorFactory();
            TestConfigurationSetProcessor setProcessor = factory.CreateTestProcessor(configurationSet);
            setProcessor.Exceptions.Add(configurationUnitThrows, new InvalidDataException());

            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(factory);

            Assert.Throws<InvalidDataException>(() => processor.GetSetDetails(configurationSet, ConfigurationUnitDetailLevel.Local));
            Assert.NotNull(configurationUnitWorks.Details);
        }

        /// <summary>
        /// Getting set details retrieves all values.
        /// </summary>
        [Fact]
        public void GetSetDetailsSuccess()
        {
            ConfigurationSet configurationSet = new ConfigurationSet();
            ConfigurationUnit configurationUnit1 = new ConfigurationUnit();
            ConfigurationUnit configurationUnit2 = new ConfigurationUnit();
            configurationSet.ConfigurationUnits = new ConfigurationUnit[] { configurationUnit1, configurationUnit2 };

            TestConfigurationProcessorFactory factory = new TestConfigurationProcessorFactory();

            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(factory);

            processor.GetSetDetails(configurationSet, ConfigurationUnitDetailLevel.Local);
            Assert.NotNull(configurationUnit1.Details);
            Assert.NotNull(configurationUnit2.Details);
        }

        /// <summary>
        /// The unit settings processor returns an error HRESULT.
        /// </summary>
        [Fact]
        public void GetSettings_ProcessorSettingsError()
        {
            ConfigurationUnit configurationUnit = new ConfigurationUnit();

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
            ConfigurationUnit configurationUnit = new ConfigurationUnit();

            TestConfigurationProcessorFactory factory = new TestConfigurationProcessorFactory();
            factory.NullProcessor = new TestConfigurationSetProcessor(null);
            TestConfigurationUnitProcessor unitProcessor = factory.NullProcessor.CreateTestProcessor(configurationUnit);
            GetSettingsResult getSettingsResult = new GetSettingsResult();
            getSettingsResult.ResultInformation.ResultCode = new InvalidDataException();
            getSettingsResult.ResultInformation.Description = "We fail because we must";
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
            ConfigurationUnit configurationUnit = new ConfigurationUnit();

            TestConfigurationProcessorFactory factory = new TestConfigurationProcessorFactory();
            factory.NullProcessor = new TestConfigurationSetProcessor(null);
            TestConfigurationUnitProcessor unitProcessor = factory.NullProcessor.CreateTestProcessor(configurationUnit);
            GetSettingsResult getSettingsResult = new GetSettingsResult();
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
