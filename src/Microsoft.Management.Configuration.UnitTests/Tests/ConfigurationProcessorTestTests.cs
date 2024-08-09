// -----------------------------------------------------------------------------
// <copyright file="ConfigurationProcessorTestTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Tests
{
    using System;
    using System.IO;
    using System.Linq;
    using Microsoft.Management.Configuration.UnitTests.Fixtures;
    using Microsoft.Management.Configuration.UnitTests.Helpers;
    using Microsoft.VisualBasic;
    using Xunit;
    using Xunit.Abstractions;

    /// <summary>
    /// Unit tests for running test on the processor.
    /// </summary>
    [Collection("UnitTestCollection")]
    [InProc]
    [OutOfProc]
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
            ConfigurationSet configurationSet = this.ConfigurationSet();

            TestConfigurationProcessorFactory factory = new TestConfigurationProcessorFactory();
            factory.Exceptions.Add(configurationSet, new FileNotFoundException());

            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(factory);

            Assert.Throws<FileNotFoundException>(() => processor.TestSet(configurationSet));

            Assert.Empty(this.EventSink.Events);
        }

        /// <summary>
        /// An error creating a unit processor results in an error for that unit.
        /// </summary>
        [Fact]
        public void TestSet_UnitProcessorCreationError()
        {
            ConfigurationSet configurationSet = this.ConfigurationSet();
            ConfigurationUnit configurationUnitThrows = this.ConfigurationUnit();
            ConfigurationUnit configurationUnitWorks = this.ConfigurationUnit();
            configurationSet.Units = new ConfigurationUnit[] { configurationUnitThrows, configurationUnitWorks };

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
            Assert.Equal(ConfigurationUnitResultSource.Internal, throwsResult.ResultInformation.ResultSource);

            TestConfigurationUnitResult worksResult = result.UnitResults.First(x => x.Unit == configurationUnitWorks);
            Assert.NotNull(worksResult);
            Assert.Equal(ConfigurationTestResult.Positive, worksResult.TestResult);
            Assert.NotNull(worksResult.ResultInformation);
            Assert.Null(worksResult.ResultInformation.ResultCode);
            Assert.Equal(ConfigurationUnitResultSource.None, worksResult.ResultInformation.ResultSource);

            this.VerifySummaryEvent(configurationSet, result, throwsResult.ResultInformation.ResultCode.HResult, ConfigurationUnitResultSource.Internal);
        }

        /// <summary>
        /// An error running a unit processor results in an error for that unit.
        /// </summary>
        [Fact]
        public void TestSet_UnitProcessorExecutionError()
        {
            ConfigurationSet configurationSet = this.ConfigurationSet();
            ConfigurationUnit configurationUnitThrows = this.ConfigurationUnit();
            ConfigurationUnit configurationUnitWorks = this.ConfigurationUnit();
            configurationSet.Units = new ConfigurationUnit[] { configurationUnitWorks, configurationUnitThrows };

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
            Assert.Equal(ConfigurationUnitResultSource.Internal, throwsResult.ResultInformation.ResultSource);

            TestConfigurationUnitResult worksResult = result.UnitResults.First(x => x.Unit == configurationUnitWorks);
            Assert.NotNull(worksResult);
            Assert.Equal(ConfigurationTestResult.Positive, worksResult.TestResult);
            Assert.NotNull(worksResult.ResultInformation);
            Assert.Null(worksResult.ResultInformation.ResultCode);
            Assert.Equal(ConfigurationUnitResultSource.None, worksResult.ResultInformation.ResultSource);

            this.VerifySummaryEvent(configurationSet, result, throwsResult.ResultInformation.ResultCode.HResult, ConfigurationUnitResultSource.Internal);
        }

        /// <summary>
        /// An error running a unit processor results in an error for that unit.
        /// </summary>
        [Fact]
        public void TestSet_UnitProcessorResultError()
        {
            ConfigurationSet configurationSet = this.ConfigurationSet();
            ConfigurationUnit configurationUnitThrows = this.ConfigurationUnit();
            ConfigurationUnit configurationUnitWorks = this.ConfigurationUnit();
            configurationSet.Units = new ConfigurationUnit[] { configurationUnitWorks, configurationUnitThrows };

            TestConfigurationProcessorFactory factory = new TestConfigurationProcessorFactory();
            TestConfigurationSetProcessor setProcessor = factory.CreateTestProcessor(configurationSet);
            TestConfigurationUnitProcessor unitProcessor = setProcessor.CreateTestProcessor(configurationUnitThrows);
            TestSettingsResultInstance testResult = new TestSettingsResultInstance(configurationUnitThrows);
            testResult.TestResult = ConfigurationTestResult.Failed;
            testResult.InternalResult.ResultCode = new NullReferenceException();
            testResult.InternalResult.Description = "Failed again";
            testResult.InternalResult.ResultSource = ConfigurationUnitResultSource.UnitProcessing;
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
            Assert.Equal(testResult.ResultInformation.ResultSource, throwsResult.ResultInformation.ResultSource);

            TestConfigurationUnitResult worksResult = result.UnitResults.First(x => x.Unit == configurationUnitWorks);
            Assert.NotNull(worksResult);
            Assert.Equal(ConfigurationTestResult.Positive, worksResult.TestResult);
            Assert.NotNull(worksResult.ResultInformation);
            Assert.Null(worksResult.ResultInformation.ResultCode);
            Assert.Equal(ConfigurationUnitResultSource.None, worksResult.ResultInformation.ResultSource);

            this.VerifySummaryEvent(configurationSet, result, testResult.ResultInformation.ResultCode.HResult, testResult.ResultInformation.ResultSource);
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

        private TestSettingsResultInstance PositiveResult(ConfigurationUnit unit)
        {
            TestSettingsResultInstance positiveResult = new TestSettingsResultInstance(unit);
            positiveResult.TestResult = ConfigurationTestResult.Positive;
            return positiveResult;
        }

        private TestSettingsResultInstance NegativeResult(ConfigurationUnit unit)
        {
            TestSettingsResultInstance negativeResult = new TestSettingsResultInstance(unit);
            negativeResult.TestResult = ConfigurationTestResult.Negative;
            return negativeResult;
        }

        private TestSettingsResultInstance FailedResult(ConfigurationUnit unit, string description, ConfigurationUnitResultSource resultSource)
        {
            TestSettingsResultInstance failedResult = new TestSettingsResultInstance(unit);
            failedResult.TestResult = ConfigurationTestResult.Failed;
            failedResult.InternalResult.ResultCode = new NullReferenceException();
            failedResult.InternalResult.Description = description;
            failedResult.InternalResult.ResultSource = resultSource;
            return failedResult;
        }

        /// <summary>
        /// Creates a test scenario where the units produce the given test results.
        /// </summary>
        /// <param name="resultTypes">The result types for each unit.</param>
        /// <param name="overallResult">The expected overall test result.</param>
        private void RunTestSetTestForResultTypes(ConfigurationTestResult[] resultTypes, ConfigurationTestResult overallResult)
        {
            ConfigurationSet configurationSet = this.ConfigurationSet();
            ConfigurationUnit[] configurationUnits = new ConfigurationUnit[resultTypes.Length];

            TestConfigurationProcessorFactory factory = new TestConfigurationProcessorFactory();
            TestConfigurationSetProcessor setProcessor = factory.CreateTestProcessor(configurationSet);

            string failedDescription = "Failed again";
            ConfigurationUnitResultSource failedResultSource = ConfigurationUnitResultSource.UnitProcessing;

            for (int i = 0; i < resultTypes.Length; ++i)
            {
                configurationUnits[i] = this.ConfigurationUnit();
                configurationUnits[i].Type = $"Unit {i}";
                TestConfigurationUnitProcessor unitProcessor = setProcessor.CreateTestProcessor(configurationUnits[i]);

                switch (resultTypes[i])
                {
                    case ConfigurationTestResult.Positive:
                        unitProcessor.TestSettingsDelegateWithUnit = (ConfigurationUnit unit) => this.PositiveResult(unit);
                        break;
                    case ConfigurationTestResult.Negative:
                        unitProcessor.TestSettingsDelegateWithUnit = (ConfigurationUnit unit) => this.NegativeResult(unit);
                        break;
                    case ConfigurationTestResult.NotRun:
                        configurationUnits[i].Intent = ConfigurationUnitIntent.Inform;
                        break;
                    case ConfigurationTestResult.Failed:
                        unitProcessor.TestSettingsDelegateWithUnit = (ConfigurationUnit unit) => this.FailedResult(unit, failedDescription, failedResultSource);
                        break;
                }
            }

            configurationSet.Units = configurationUnits;

            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(factory);

            TestConfigurationSetResult result = processor.TestSet(configurationSet);

            Assert.NotNull(result);
            Assert.Equal(overallResult, result.TestResult);
            Assert.NotNull(result.UnitResults);
            Assert.Equal(resultTypes.Length, result.UnitResults.Count);

            int summaryEventResult = 0;
            ConfigurationUnitResultSource resultSource = ConfigurationUnitResultSource.None;

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
                        Assert.Equal(ConfigurationUnitResultSource.None, unitResult.ResultInformation.ResultSource);
                        break;
                    case ConfigurationTestResult.Failed:
                        Assert.NotNull(unitResult.ResultInformation.ResultCode);
                        Assert.IsType<NullReferenceException>(unitResult.ResultInformation.ResultCode);
                        Assert.Equal(failedDescription, unitResult.ResultInformation.Description);
                        Assert.Equal(failedResultSource, unitResult.ResultInformation.ResultSource);
                        summaryEventResult = unitResult.ResultInformation.ResultCode.HResult;
                        resultSource = unitResult.ResultInformation.ResultSource;
                        break;
                }
            }

            this.VerifySummaryEvent(configurationSet, result, summaryEventResult, resultSource);
        }
    }
}
