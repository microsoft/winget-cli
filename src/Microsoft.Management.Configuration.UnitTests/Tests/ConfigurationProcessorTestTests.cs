// -----------------------------------------------------------------------------
// <copyright file="ConfigurationProcessorTestTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Tests
{
    using System;
    using System.Collections.Generic;
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

        /// <summary>
        /// Test a set that was parsed with schema 0.3.
        /// </summary>
        [Fact]
        public void TestSet_Parsed_0_3()
        {
            TestConfigurationProcessorFactory factory = new TestConfigurationProcessorFactory();
            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(factory);

            OpenConfigurationSetResult openResult = processor.OpenConfigurationSet(this.CreateStream(@"
$schema: https://raw.githubusercontent.com/PowerShell/DSC/main/schemas/2023/08/config/document.json
resources:
  - name: Name
    type: Module/Resource
    properties:
      c: 3
      d: '4'
  - name: Name2
    type: Module/Resource2
    properties:
      l: '10'
"));

            Assert.Null(openResult.ResultCode);
            Assert.NotNull(openResult.Set);
            Assert.Equal(string.Empty, openResult.Field);
            Assert.Equal(string.Empty, openResult.Value);
            Assert.Equal(0U, openResult.Line);
            Assert.Equal(0U, openResult.Column);

            ConfigurationSet configurationSet = openResult.Set;
            int unitCount = configurationSet.Units.Count;

            TestConfigurationSetProcessor setProcessor = factory.CreateTestProcessor(configurationSet);

            TestConfigurationUnitProcessor[] unitProcessors = new TestConfigurationUnitProcessor[unitCount];
            for (int i = 0; i < unitCount; ++i)
            {
                unitProcessors[i] = setProcessor.CreateTestProcessor(configurationSet.Units[i]);
                if (i == 0)
                {
                    unitProcessors[i].TestSettingsDelegateWithUnit = (ConfigurationUnit unit) => new TestSettingsResultInstance(unit) { TestResult = ConfigurationTestResult.Negative };
                }
                else
                {
                    unitProcessors[i].TestSettingsDelegateWithUnit = (ConfigurationUnit unit) => new TestSettingsResultInstance(unit) { TestResult = ConfigurationTestResult.Positive };
                }
            }

            TestConfigurationSetResult result = processor.TestSet(configurationSet);
            Assert.NotNull(result);
            Assert.Equal(ConfigurationTestResult.Negative, result.TestResult);
            Assert.Equal(unitCount, result.UnitResults.Count);

            for (int i = 0; i < unitCount; ++i)
            {
                var unitResult = result.UnitResults[i];
                Assert.NotNull(unitResult);

                if (i == 0)
                {
                    Assert.Equal(ConfigurationTestResult.Negative, unitResult.TestResult);
                }
                else
                {
                    Assert.Equal(ConfigurationTestResult.Positive, unitResult.TestResult);
                }

                Assert.NotNull(unitResult.ResultInformation);
                Assert.Null(unitResult.ResultInformation.ResultCode);
                Assert.Equal(ConfigurationUnitResultSource.None, unitResult.ResultInformation.ResultSource);
            }

            this.VerifySummaryEvent(configurationSet, result, 0, ConfigurationUnitResultSource.None);
        }

        /// <summary>
        /// Test when the set processor is a group processor.
        /// </summary>
        [Fact]
        public void TestSet_SetGroupProcessor()
        {
            ConfigurationSet configurationSet = this.ConfigurationSet();
            configurationSet.Metadata[TestConfigurationUnitGroupProcessor.TestResultSetting] = ConfigurationTestResult.Negative.ToString();
            ConfigurationUnit configurationUnitNegative = this.ConfigurationUnit();
            configurationUnitNegative.Settings[TestConfigurationUnitGroupProcessor.TestResultSetting] = ConfigurationTestResult.Negative.ToString();
            ConfigurationUnit configurationUnitPositive = this.ConfigurationUnit();
            configurationUnitPositive.Settings[TestConfigurationUnitGroupProcessor.TestResultSetting] = ConfigurationTestResult.Positive.ToString();
            configurationSet.Units = new ConfigurationUnit[] { configurationUnitNegative, configurationUnitPositive };

            TestConfigurationProcessorFactory factory = new TestConfigurationProcessorFactory();
            TestConfigurationSetGroupProcessor setProcessor = factory.CreateTestGroupProcessor(configurationSet);
            setProcessor.ShouldWaitOnAsyncEvent = true;

            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(factory);
            List<TestConfigurationUnitResult> progressValues = new List<TestConfigurationUnitResult>();

            var operation = processor.TestSetAsync(configurationSet);
            operation.Progress = (Windows.Foundation.IAsyncOperationWithProgress<TestConfigurationSetResult, TestConfigurationUnitResult> op, TestConfigurationUnitResult unitResult) => { progressValues.Add(unitResult); };
            setProcessor.SignalAsyncEvent();

            operation.AsTask().Wait();
            TestConfigurationSetResult result = operation.GetResults();

            Assert.NotNull(result);
            Assert.Equal(ConfigurationTestResult.Negative, result.TestResult);
            Assert.NotNull(result.UnitResults);
            Assert.Equal(2, result.UnitResults.Count);
            Assert.Equal(2, progressValues.Count);

            TestConfigurationUnitResult negativeResult = result.UnitResults.First(x => x.Unit == configurationUnitNegative);
            TestConfigurationUnitResult negativeProgress = progressValues.First(x => x.Unit == configurationUnitNegative);

            foreach (TestConfigurationUnitResult unitResult in new TestConfigurationUnitResult[] { negativeResult, negativeProgress })
            {
                Assert.NotNull(unitResult);
                Assert.Equal(ConfigurationTestResult.Negative, unitResult.TestResult);
                Assert.NotNull(unitResult.ResultInformation);
                Assert.Null(unitResult.ResultInformation.ResultCode);
                Assert.Equal(ConfigurationUnitResultSource.None, unitResult.ResultInformation.ResultSource);
            }

            TestConfigurationUnitResult positiveResult = result.UnitResults.First(x => x.Unit == configurationUnitPositive);
            TestConfigurationUnitResult positiveProgress = progressValues.First(x => x.Unit == configurationUnitPositive);

            foreach (TestConfigurationUnitResult unitResult in new TestConfigurationUnitResult[] { positiveResult, positiveProgress })
            {
                Assert.NotNull(unitResult);
                Assert.Equal(ConfigurationTestResult.Positive, unitResult.TestResult);
                Assert.NotNull(unitResult.ResultInformation);
                Assert.Null(unitResult.ResultInformation.ResultCode);
                Assert.Equal(ConfigurationUnitResultSource.None, unitResult.ResultInformation.ResultSource);
            }

            this.VerifySummaryEvent(configurationSet, result, 0, ConfigurationUnitResultSource.None);
        }

        /// <summary>
        /// Test when the set processor is a group processor and contains a unit that is also a group.
        /// </summary>
        [Fact]
        public void TestSet_SetGroupProcessor_WithGroupUnit()
        {
            ConfigurationSet configurationSet = this.ConfigurationSet();
            configurationSet.Metadata[TestConfigurationUnitGroupProcessor.TestResultSetting] = ConfigurationTestResult.Negative.ToString();

            ConfigurationUnit configurationUnit = this.ConfigurationUnit();
            ConfigurationUnit configurationUnitGroup = this.ConfigurationUnit();

            configurationSet.Units = new ConfigurationUnit[] { configurationUnit, configurationUnitGroup };

            ConfigurationUnit configurationUnitGroupMember = this.ConfigurationUnit();
            configurationUnitGroup.IsGroup = true;
            configurationUnitGroup.Units = new ConfigurationUnit[] { configurationUnitGroupMember };

            TestConfigurationProcessorFactory factory = new TestConfigurationProcessorFactory();
            TestConfigurationSetGroupProcessor setProcessor = factory.CreateTestGroupProcessor(configurationSet);
            setProcessor.ShouldWaitOnAsyncEvent = true;

            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(factory);
            List<TestConfigurationUnitResult> progressValues = new List<TestConfigurationUnitResult>();

            var operation = processor.TestSetAsync(configurationSet);
            operation.Progress = (Windows.Foundation.IAsyncOperationWithProgress<TestConfigurationSetResult, TestConfigurationUnitResult> op, TestConfigurationUnitResult unitResult) => { progressValues.Add(unitResult); };
            setProcessor.SignalAsyncEvent();

            operation.AsTask().Wait();
            TestConfigurationSetResult result = operation.GetResults();

            Assert.NotNull(result);
            Assert.Equal(ConfigurationTestResult.Positive, result.TestResult);
            Assert.NotNull(result.UnitResults);
            Assert.Equal(3, result.UnitResults.Count);
            Assert.Equal(3, progressValues.Count);

            foreach (ConfigurationUnit unit in new ConfigurationUnit[] { configurationUnit, configurationUnitGroup, configurationUnitGroupMember })
            {
                foreach (IReadOnlyList<TestConfigurationUnitResult> unitResults in new IReadOnlyList<TestConfigurationUnitResult>[] { result.UnitResults, progressValues })
                {
                    TestConfigurationUnitResult unitResult = unitResults.First(x => x.Unit == unit);

                    Assert.NotNull(unitResult);
                    Assert.Equal(ConfigurationTestResult.Positive, unitResult.TestResult);
                    Assert.NotNull(unitResult.ResultInformation);
                    Assert.Null(unitResult.ResultInformation.ResultCode);
                    Assert.Equal(ConfigurationUnitResultSource.None, unitResult.ResultInformation.ResultSource);
                }
            }

            this.VerifySummaryEvent(configurationSet, result, 0, ConfigurationUnitResultSource.None);
        }

        /// <summary>
        /// Test when the standard set processor is used and there is a group unit.
        /// </summary>
        [Fact]
        public void TestSet_UnitGroupProcessor_WithGroupUnit()
        {
            ConfigurationSet configurationSet = this.ConfigurationSet();
            configurationSet.Metadata[TestConfigurationUnitGroupProcessor.TestResultSetting] = ConfigurationTestResult.Negative.ToString();

            ConfigurationUnit configurationUnit = this.ConfigurationUnit();
            ConfigurationUnit configurationUnitGroup = this.ConfigurationUnit();

            configurationSet.Units = new ConfigurationUnit[] { configurationUnit, configurationUnitGroup };

            ConfigurationUnit configurationUnitGroupMember = this.ConfigurationUnit();
            configurationUnitGroup.IsGroup = true;
            configurationUnitGroup.Units = new ConfigurationUnit[] { configurationUnitGroupMember };

            TestConfigurationProcessorFactory factory = new TestConfigurationProcessorFactory();
            TestConfigurationSetProcessor setProcessor = factory.CreateTestProcessor(configurationSet);
            setProcessor.CreateTestProcessor(configurationUnit);
            setProcessor.CreateTestGroupProcessor(configurationUnitGroup);

            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(factory);

            TestConfigurationSetResult result = processor.TestSet(configurationSet);

            Assert.NotNull(result);
            Assert.Equal(ConfigurationTestResult.Positive, result.TestResult);
            Assert.NotNull(result.UnitResults);
            Assert.Equal(3, result.UnitResults.Count);

            foreach (ConfigurationUnit unit in new ConfigurationUnit[] { configurationUnit, configurationUnitGroup, configurationUnitGroupMember })
            {
                TestConfigurationUnitResult unitResult = result.UnitResults.First(x => x.Unit == unit);

                Assert.NotNull(unitResult);
                Assert.Equal(ConfigurationTestResult.Positive, unitResult.TestResult);
                Assert.NotNull(unitResult.ResultInformation);
                Assert.Null(unitResult.ResultInformation.ResultCode);
                Assert.Equal(ConfigurationUnitResultSource.None, unitResult.ResultInformation.ResultSource);
            }

            this.VerifySummaryEvent(configurationSet, result, 0, ConfigurationUnitResultSource.None);
        }

        /// <summary>
        /// Test when the standard set processor is used and there is a non-group unit that still exposes a group processor.
        /// </summary>
        [Fact]
        public void TestSet_UnitGroupProcessor_WithNonGroupUnit()
        {
            ConfigurationSet configurationSet = this.ConfigurationSet();
            configurationSet.Metadata[TestConfigurationUnitGroupProcessor.TestResultSetting] = ConfigurationTestResult.Negative.ToString();

            ConfigurationUnit configurationUnit = this.ConfigurationUnit();
            ConfigurationUnit configurationUnitGroup = this.ConfigurationUnit();

            configurationSet.Units = new ConfigurationUnit[] { configurationUnit, configurationUnitGroup };

            TestConfigurationProcessorFactory factory = new TestConfigurationProcessorFactory();
            TestConfigurationSetProcessor setProcessor = factory.CreateTestProcessor(configurationSet);
            setProcessor.CreateTestProcessor(configurationUnit);
            setProcessor.CreateTestGroupProcessor(configurationUnitGroup);

            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(factory);

            TestConfigurationSetResult result = processor.TestSet(configurationSet);

            Assert.NotNull(result);
            Assert.Equal(ConfigurationTestResult.Positive, result.TestResult);
            Assert.NotNull(result.UnitResults);
            Assert.Equal(2, result.UnitResults.Count);

            foreach (ConfigurationUnit unit in new ConfigurationUnit[] { configurationUnit, configurationUnitGroup })
            {
                TestConfigurationUnitResult unitResult = result.UnitResults.First(x => x.Unit == unit);

                Assert.NotNull(unitResult);
                Assert.Equal(ConfigurationTestResult.Positive, unitResult.TestResult);
                Assert.NotNull(unitResult.ResultInformation);
                Assert.Null(unitResult.ResultInformation.ResultCode);
                Assert.Equal(ConfigurationUnitResultSource.None, unitResult.ResultInformation.ResultSource);
            }

            this.VerifySummaryEvent(configurationSet, result, 0, ConfigurationUnitResultSource.None);
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
