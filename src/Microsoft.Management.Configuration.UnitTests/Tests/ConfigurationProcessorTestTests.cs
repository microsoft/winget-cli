// -----------------------------------------------------------------------------
// <copyright file="ConfigurationProcessorTestTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Tests
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using System.Runtime.InteropServices;
    using Microsoft.Management.Configuration.UnitTests.Fixtures;
    using Microsoft.Management.Configuration.UnitTests.Helpers;
    using Microsoft.VisualBasic;
    using Microsoft.VisualStudio.TestPlatform.ObjectModel;
    using Xunit;
    using Xunit.Abstractions;

    /// <summary>
    /// Unit tests for running test on the processor.
    /// </summary>
    [Collection("UnitTestCollection")]
    public class ConfigurationProcessorTestTests : ConfigurationProcessorTestBase
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="ConfigurationProcessorTestTests"/> class.
        /// </summary>
        /// <param name="fixture">Unit test fixture.</param>
        /// <param name="log">Log helper.</param>
        public ConfigurationProcessorTestTests(UnitTestFixture fixture, ITestOutputHelper log)
            : base(fixture, log)
        {
        }

        /// <summary>
        /// An error creating the set processor results in an error for the function.
        /// </summary>
        [Fact]
        public void TestSet_SetProcessorError()
        {
            ConfigurationSet configurationSet = new ConfigurationSet();

            TestConfigurationProcessorFactory factory = new TestConfigurationProcessorFactory();
            factory.Exceptions.Add(configurationSet, new FileNotFoundException());

            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(factory);

            Assert.Throws<FileNotFoundException>(() => processor.TestSet(configurationSet));
        }

        /// <summary>
        /// An error creating a unit processor results in an error for that unit.
        /// </summary>
        [Fact]
        public void TestSet_UnitProcessorCreationError()
        {
            ConfigurationSet configurationSet = new ConfigurationSet();
            ConfigurationUnit configurationUnitThrows = new ConfigurationUnit();
            ConfigurationUnit configurationUnitWorks = new ConfigurationUnit();
            configurationSet.ConfigurationUnits = new ConfigurationUnit[] { configurationUnitThrows, configurationUnitWorks };

            TestConfigurationProcessorFactory factory = new TestConfigurationProcessorFactory();
            TestConfigurationSetProcessor setProcessor = factory.CreateTestProcessor(configurationSet);
            setProcessor.Exceptions.Add(configurationUnitThrows, new NullReferenceException());

            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(factory);

            TestConfigurationSetResult result = processor.TestSet(configurationSet);

            Assert.NotNull(result);
            Assert.Equal(ConfigurationTestResult.Failed, result.TestResult);
            Assert.NotNull(result.UnitResults);
            Assert.Equal(2, result.UnitResults.Count);

            TestConfigurationUnitResult throwsResult = result.UnitResults.First(x => x.Unit == configurationUnitThrows);
            Assert.NotNull(throwsResult);
            Assert.Equal(ConfigurationTestResult.Failed, throwsResult.TestResult);
            Assert.NotNull(throwsResult.ResultInformation);
            Assert.NotNull(throwsResult.ResultInformation.ResultCode);
            Assert.IsType<NullReferenceException>(throwsResult.ResultInformation.ResultCode);

            TestConfigurationUnitResult worksResult = result.UnitResults.First(x => x.Unit == configurationUnitWorks);
            Assert.NotNull(worksResult);
            Assert.Equal(ConfigurationTestResult.Positive, worksResult.TestResult);
            Assert.NotNull(worksResult.ResultInformation);
            Assert.Null(worksResult.ResultInformation.ResultCode);
        }

        /// <summary>
        /// An error running a unit processor results in an error for that unit.
        /// </summary>
        [Fact]
        public void TestSet_UnitProcessorExecutionError()
        {
            ConfigurationSet configurationSet = new ConfigurationSet();
            ConfigurationUnit configurationUnitThrows = new ConfigurationUnit();
            ConfigurationUnit configurationUnitWorks = new ConfigurationUnit();
            configurationSet.ConfigurationUnits = new ConfigurationUnit[] { configurationUnitWorks, configurationUnitThrows };

            TestConfigurationProcessorFactory factory = new TestConfigurationProcessorFactory();
            TestConfigurationSetProcessor setProcessor = factory.CreateTestProcessor(configurationSet);
            TestConfigurationUnitProcessor unitProcessor = setProcessor.CreateTestProcessor(configurationUnitThrows);
            unitProcessor.TestSettingsDelegate = () => throw new NullReferenceException();

            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(factory);

            TestConfigurationSetResult result = processor.TestSet(configurationSet);

            Assert.NotNull(result);
            Assert.Equal(ConfigurationTestResult.Failed, result.TestResult);
            Assert.NotNull(result.UnitResults);
            Assert.Equal(2, result.UnitResults.Count);

            TestConfigurationUnitResult throwsResult = result.UnitResults.First(x => x.Unit == configurationUnitThrows);
            Assert.NotNull(throwsResult);
            Assert.Equal(ConfigurationTestResult.Failed, throwsResult.TestResult);
            Assert.NotNull(throwsResult.ResultInformation);
            Assert.NotNull(throwsResult.ResultInformation.ResultCode);
            Assert.IsType<NullReferenceException>(throwsResult.ResultInformation.ResultCode);

            TestConfigurationUnitResult worksResult = result.UnitResults.First(x => x.Unit == configurationUnitWorks);
            Assert.NotNull(worksResult);
            Assert.Equal(ConfigurationTestResult.Positive, worksResult.TestResult);
            Assert.NotNull(worksResult.ResultInformation);
            Assert.Null(worksResult.ResultInformation.ResultCode);
        }

        /// <summary>
        /// An error running a unit processor results in an error for that unit.
        /// </summary>
        [Fact]
        public void TestSet_UnitProcessorResultError()
        {
            ConfigurationSet configurationSet = new ConfigurationSet();
            ConfigurationUnit configurationUnitThrows = new ConfigurationUnit();
            ConfigurationUnit configurationUnitWorks = new ConfigurationUnit();
            configurationSet.ConfigurationUnits = new ConfigurationUnit[] { configurationUnitWorks, configurationUnitThrows };

            TestConfigurationProcessorFactory factory = new TestConfigurationProcessorFactory();
            TestConfigurationSetProcessor setProcessor = factory.CreateTestProcessor(configurationSet);
            TestConfigurationUnitProcessor unitProcessor = setProcessor.CreateTestProcessor(configurationUnitThrows);
            TestSettingsResult testResult = new TestSettingsResult();
            testResult.TestResult = ConfigurationTestResult.Failed;
            testResult.ResultInformation.ResultCode = new NullReferenceException();
            testResult.ResultInformation.Description = "Failed again";
            unitProcessor.TestSettingsDelegate = () => testResult;

            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(factory);

            TestConfigurationSetResult result = processor.TestSet(configurationSet);

            Assert.NotNull(result);
            Assert.Equal(ConfigurationTestResult.Failed, result.TestResult);
            Assert.NotNull(result.UnitResults);
            Assert.Equal(2, result.UnitResults.Count);

            TestConfigurationUnitResult throwsResult = result.UnitResults.First(x => x.Unit == configurationUnitThrows);
            Assert.NotNull(throwsResult);
            Assert.Equal(ConfigurationTestResult.Failed, throwsResult.TestResult);
            Assert.NotNull(throwsResult.ResultInformation);
            Assert.NotNull(throwsResult.ResultInformation.ResultCode);
            Assert.IsType<NullReferenceException>(throwsResult.ResultInformation.ResultCode);
            Assert.Equal(testResult.ResultInformation.Description, throwsResult.ResultInformation.Description);

            TestConfigurationUnitResult worksResult = result.UnitResults.First(x => x.Unit == configurationUnitWorks);
            Assert.NotNull(worksResult);
            Assert.Equal(ConfigurationTestResult.Positive, worksResult.TestResult);
            Assert.NotNull(worksResult.ResultInformation);
            Assert.Null(worksResult.ResultInformation.ResultCode);
        }

        /// <summary>
        /// Ensures that the expected TestResult comes back for all types.
        /// </summary>
        [Fact]
        public void TestSet_ResultTypes()
        {
            this.RunTestSetTestForResultTypes(new ConfigurationTestResult[] { ConfigurationTestResult.Positive, ConfigurationTestResult.Negative, ConfigurationTestResult.Failed, ConfigurationTestResult.NotRun }, ConfigurationTestResult.Failed);
        }

        /// <summary>
        /// Ensures that a single negative makes the overall result negative.
        /// </summary>
        [Fact]
        public void TestSet_Negative()
        {
            this.RunTestSetTestForResultTypes(new ConfigurationTestResult[] { ConfigurationTestResult.Positive, ConfigurationTestResult.Negative, ConfigurationTestResult.Positive, ConfigurationTestResult.NotRun }, ConfigurationTestResult.Negative);
        }

        /// <summary>
        /// Ensures that a single not run test does not impact the positive result.
        /// </summary>
        [Fact]
        public void TestSet_Positive()
        {
            this.RunTestSetTestForResultTypes(new ConfigurationTestResult[] { ConfigurationTestResult.Positive, ConfigurationTestResult.Positive, ConfigurationTestResult.Positive, ConfigurationTestResult.NotRun }, ConfigurationTestResult.Positive);
        }

        /// <summary>
        /// Creates a test scenario where the units produce the given test results.
        /// </summary>
        /// <param name="resultTypes">The result types for each unit.</param>
        /// <param name="overallResult">The expected overall test result.</param>
        private void RunTestSetTestForResultTypes(ConfigurationTestResult[] resultTypes, ConfigurationTestResult overallResult)
        {
            ConfigurationSet configurationSet = new ConfigurationSet();
            ConfigurationUnit[] configurationUnits = new ConfigurationUnit[resultTypes.Length];

            TestConfigurationProcessorFactory factory = new TestConfigurationProcessorFactory();
            TestConfigurationSetProcessor setProcessor = factory.CreateTestProcessor(configurationSet);

            TestSettingsResult positiveResult = new TestSettingsResult();
            positiveResult.TestResult = ConfigurationTestResult.Positive;

            TestSettingsResult negativeResult = new TestSettingsResult();
            negativeResult.TestResult = ConfigurationTestResult.Negative;

            TestSettingsResult failedResult = new TestSettingsResult();
            failedResult.TestResult = ConfigurationTestResult.Failed;
            failedResult.ResultInformation.ResultCode = new NullReferenceException();
            failedResult.ResultInformation.Description = "Failed again";

            for (int i = 0; i < resultTypes.Length; ++i)
            {
                configurationUnits[i] = new ConfigurationUnit();
                configurationUnits[i].UnitName = $"Unit {i}";
                TestConfigurationUnitProcessor unitProcessor = setProcessor.CreateTestProcessor(configurationUnits[i]);

                switch (resultTypes[i])
                {
                    case ConfigurationTestResult.Positive:
                        unitProcessor.TestSettingsDelegate = () => positiveResult;
                        break;
                    case ConfigurationTestResult.Negative:
                        unitProcessor.TestSettingsDelegate = () => negativeResult;
                        break;
                    case ConfigurationTestResult.NotRun:
                        configurationUnits[i].Intent = ConfigurationUnitIntent.Inform;
                        break;
                    case ConfigurationTestResult.Failed:
                        unitProcessor.TestSettingsDelegate = () => failedResult;
                        break;
                }
            }

            configurationSet.ConfigurationUnits = configurationUnits;

            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(factory);

            TestConfigurationSetResult result = processor.TestSet(configurationSet);

            Assert.NotNull(result);
            Assert.Equal(overallResult, result.TestResult);
            Assert.NotNull(result.UnitResults);
            Assert.Equal(resultTypes.Length, result.UnitResults.Count);

            for (int i = 0; i < resultTypes.Length; ++i)
            {
                TestConfigurationUnitResult unitResult = result.UnitResults.First(x => x.Unit == configurationUnits[i]);

                Assert.NotNull(unitResult);
                Assert.Equal(resultTypes[i], unitResult.TestResult);
                Assert.NotNull(unitResult.ResultInformation);

                switch (resultTypes[i])
                {
                    case ConfigurationTestResult.Positive:
                    case ConfigurationTestResult.Negative:
                    case ConfigurationTestResult.NotRun:
                        Assert.Null(unitResult.ResultInformation.ResultCode);
                        Assert.Empty(unitResult.ResultInformation.Description);
                        break;
                    case ConfigurationTestResult.Failed:
                        Assert.NotNull(unitResult.ResultInformation.ResultCode);
                        Assert.IsType<NullReferenceException>(unitResult.ResultInformation.ResultCode);
                        Assert.Equal(failedResult.ResultInformation.Description, unitResult.ResultInformation.Description);
                        break;
                }
            }
        }
    }
}
