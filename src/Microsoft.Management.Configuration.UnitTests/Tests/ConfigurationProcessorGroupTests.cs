// -----------------------------------------------------------------------------
// <copyright file="ConfigurationProcessorGroupTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Tests
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Threading;
    using Microsoft.Management.Configuration.UnitTests.Fixtures;
    using Microsoft.Management.Configuration.UnitTests.Helpers;
    using Xunit;
    using Xunit.Abstractions;

    /// <summary>
    /// Unit tests for running group processing.
    /// </summary>
    [Collection("UnitTestCollection")]
    [InProc]
    public class ConfigurationProcessorGroupTests : ConfigurationProcessorTestBase
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="ConfigurationProcessorGroupTests"/> class.
        /// </summary>
        /// <param name="fixture">Unit test fixture.</param>
        /// <param name="log">Log helper.</param>
        public ConfigurationProcessorGroupTests(UnitTestFixture fixture, ITestOutputHelper log)
            : base(fixture, log)
        {
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

        /// <summary>
        /// Apply a set that was parsed with schema 0.3.
        /// </summary>
        [Fact]
        public void ApplySet_Parsed_0_3()
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

            ApplyConfigurationSetResult result = processor.ApplySet(configurationSet, ApplyConfigurationSetFlags.None);
            Assert.NotNull(result);
            Assert.Null(result.ResultCode);
            Assert.Equal(unitCount, result.UnitResults.Count);

            for (int i = 0; i < unitCount; ++i)
            {
                var unitResult = result.UnitResults[i];
                Assert.NotNull(unitResult);

                if (i == 0)
                {
                    Assert.False(unitResult.PreviouslyInDesiredState);
                }
                else
                {
                    Assert.True(unitResult.PreviouslyInDesiredState);
                }

                Assert.False(unitResult.RebootRequired);
                Assert.NotNull(unitResult.ResultInformation);
                Assert.Null(unitResult.ResultInformation.ResultCode);
                Assert.Equal(ConfigurationUnitResultSource.None, unitResult.ResultInformation.ResultSource);
            }

            for (int i = 0; i < unitCount; ++i)
            {
                Assert.Equal(1, unitProcessors[i].TestSettingsCalls);
                Assert.Equal(0, unitProcessors[i].GetSettingsCalls);
                if (i == 0)
                {
                    Assert.Equal(1, unitProcessors[i].ApplySettingsCalls);
                }
                else
                {
                    Assert.Equal(0, unitProcessors[i].ApplySettingsCalls);
                }
            }

            this.VerifySummaryEvent(configurationSet, result, ConfigurationUnitResultSource.None);
        }

        /// <summary>
        /// Test when the set processor is a group processor.
        /// </summary>
        [Fact]
        public void ApplySet_SetGroupProcessor()
        {
            ConfigurationSet configurationSet = this.ConfigurationSet();

            ConfigurationUnit configurationUnitNegative = this.ConfigurationUnit();
            configurationUnitNegative.Settings[TestConfigurationUnitGroupProcessor.TestResultSetting] = ConfigurationTestResult.Negative.ToString();
            ConfigurationUnit configurationUnitPositive = this.ConfigurationUnit();
            configurationUnitPositive.Settings[TestConfigurationUnitGroupProcessor.TestResultSetting] = ConfigurationTestResult.Positive.ToString();
            configurationSet.Units = new ConfigurationUnit[] { configurationUnitNegative, configurationUnitPositive };

            TestConfigurationProcessorFactory factory = new TestConfigurationProcessorFactory();
            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(factory);
            ManualResetEvent startProcessing = new ManualResetEvent(false);
            factory.CreateSetProcessorDelegate = (f, c) =>
            {
                startProcessing.WaitOne();
                return f.CreateTestGroupProcessor(c);
            };

            var operation = processor.ApplySetAsync(configurationSet, ApplyConfigurationSetFlags.None);

            List<ConfigurationSetChangeData> progressValues = new List<ConfigurationSetChangeData>();
            operation.Progress = (Windows.Foundation.IAsyncOperationWithProgress<ApplyConfigurationSetResult, ConfigurationSetChangeData> op, ConfigurationSetChangeData unitResult) => { progressValues.Add(unitResult); };
            startProcessing.Set();
            operation.AsTask().Wait();
            ApplyConfigurationSetResult result = operation.GetResults();

            Assert.NotNull(result);
            Assert.Null(result.ResultCode);
            Assert.NotNull(result.UnitResults);
            Assert.Equal(configurationSet.Units.Count, result.UnitResults.Count);
            Assert.Equal((configurationSet.Units.Count * 2) + 2, progressValues.Count);

            ApplyConfigurationUnitResult negativeResult = result.UnitResults.First(x => x.Unit == configurationUnitNegative);

            Assert.NotNull(negativeResult);
            Assert.NotNull(negativeResult.ResultInformation);
            Assert.Null(negativeResult.ResultInformation.ResultCode);
            Assert.Equal(ConfigurationUnitResultSource.None, negativeResult.ResultInformation.ResultSource);
            Assert.Equal(ConfigurationUnitState.Completed, negativeResult.State);
            Assert.False(negativeResult.PreviouslyInDesiredState);

            IEnumerable<ConfigurationSetChangeData> negativeProgress = progressValues.Where(x => x.Unit == configurationUnitNegative);
            Assert.Equal(2, negativeProgress.Count());

            foreach (ConfigurationSetChangeData change in negativeProgress)
            {
                Assert.Equal(ConfigurationSetChangeEventType.UnitStateChanged, change.Change);
                Assert.Equal(ConfigurationSetState.InProgress, change.SetState);
                Assert.NotNull(change.ResultInformation);
                Assert.Null(change.ResultInformation.ResultCode);
                Assert.Equal(ConfigurationUnitResultSource.None, change.ResultInformation.ResultSource);
            }

            Assert.Single(negativeProgress.Where(x => x.UnitState == ConfigurationUnitState.InProgress));
            Assert.Single(negativeProgress.Where(x => x.UnitState == ConfigurationUnitState.Completed));

            ApplyConfigurationUnitResult positiveResult = result.UnitResults.First(x => x.Unit == configurationUnitPositive);

            Assert.NotNull(positiveResult);
            Assert.NotNull(positiveResult.ResultInformation);
            Assert.Null(positiveResult.ResultInformation.ResultCode);
            Assert.Equal(ConfigurationUnitResultSource.None, positiveResult.ResultInformation.ResultSource);
            Assert.Equal(ConfigurationUnitState.Completed, positiveResult.State);
            Assert.True(positiveResult.PreviouslyInDesiredState);

            IEnumerable<ConfigurationSetChangeData> positiveProgress = progressValues.Where(x => x.Unit == configurationUnitPositive);
            Assert.Equal(2, positiveProgress.Count());

            foreach (ConfigurationSetChangeData change in positiveProgress)
            {
                Assert.Equal(ConfigurationSetChangeEventType.UnitStateChanged, change.Change);
                Assert.Equal(ConfigurationSetState.InProgress, change.SetState);
                Assert.NotNull(change.ResultInformation);
                Assert.Null(change.ResultInformation.ResultCode);
                Assert.Equal(ConfigurationUnitResultSource.None, change.ResultInformation.ResultSource);
            }

            Assert.Single(positiveProgress.Where(x => x.UnitState == ConfigurationUnitState.InProgress));
            Assert.Single(positiveProgress.Where(x => x.UnitState == ConfigurationUnitState.Completed));

            this.VerifySummaryEvent(configurationSet, result, ConfigurationUnitResultSource.None);
        }

        /// <summary>
        /// Test when the set processor is a group processor and contains a unit that is also a group.
        /// </summary>
        [Fact]
        public void ApplySet_SetGroupProcessor_WithGroupUnit()
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
            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(factory);
            ManualResetEvent startProcessing = new ManualResetEvent(false);
            factory.CreateSetProcessorDelegate = (f, c) =>
            {
                startProcessing.WaitOne();
                return f.CreateTestGroupProcessor(c);
            };

            var operation = processor.ApplySetAsync(configurationSet, ApplyConfigurationSetFlags.None);

            List<ConfigurationSetChangeData> progressValues = new List<ConfigurationSetChangeData>();
            operation.Progress = (Windows.Foundation.IAsyncOperationWithProgress<ApplyConfigurationSetResult, ConfigurationSetChangeData> op, ConfigurationSetChangeData unitResult) => { progressValues.Add(unitResult); };
            startProcessing.Set();
            operation.AsTask().Wait();
            ApplyConfigurationSetResult result = operation.GetResults();

            Assert.NotNull(result);
            Assert.Null(result.ResultCode);
            Assert.NotNull(result.UnitResults);
            Assert.Equal(3, result.UnitResults.Count);
            Assert.Equal(2 + (3 * 2), progressValues.Count);

            foreach (ConfigurationUnit unit in new ConfigurationUnit[] { configurationUnit, configurationUnitGroup, configurationUnitGroupMember })
            {
                ApplyConfigurationUnitResult unitResult = result.UnitResults.First(x => x.Unit == unit);

                Assert.NotNull(unitResult);
                Assert.NotNull(unitResult.ResultInformation);
                Assert.Null(unitResult.ResultInformation.ResultCode);
                Assert.Equal(ConfigurationUnitResultSource.None, unitResult.ResultInformation.ResultSource);
                Assert.Equal(ConfigurationUnitState.Completed, unitResult.State);
                Assert.True(unitResult.PreviouslyInDesiredState);

                IEnumerable<ConfigurationSetChangeData> unitProgress = progressValues.Where(x => x.Unit == unit);
                Assert.Equal(2, unitProgress.Count());

                foreach (ConfigurationSetChangeData change in unitProgress)
                {
                    Assert.Equal(ConfigurationSetChangeEventType.UnitStateChanged, change.Change);
                    Assert.Equal(ConfigurationSetState.InProgress, change.SetState);
                    Assert.NotNull(change.ResultInformation);
                    Assert.Null(change.ResultInformation.ResultCode);
                    Assert.Equal(ConfigurationUnitResultSource.None, change.ResultInformation.ResultSource);
                }

                Assert.True(unitProgress.Where(x => x.UnitState == ConfigurationUnitState.InProgress).Count() <= 1);
                Assert.True(unitProgress.Where(x => x.UnitState == ConfigurationUnitState.Completed).Count() <= 1);
            }

            this.VerifySummaryEvent(configurationSet, result, ConfigurationUnitResultSource.None);
        }

        /// <summary>
        /// Test when the standard set processor is used and there is a group unit.
        /// </summary>
        [Fact]
        public void ApplySet_UnitGroupProcessor_WithGroupUnit()
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

            ApplyConfigurationSetResult result = processor.ApplySet(configurationSet, ApplyConfigurationSetFlags.None);

            Assert.NotNull(result);
            Assert.Null(result.ResultCode);
            Assert.NotNull(result.UnitResults);
            Assert.Equal(3, result.UnitResults.Count);

            foreach (ConfigurationUnit unit in new ConfigurationUnit[] { configurationUnit, configurationUnitGroup, configurationUnitGroupMember })
            {
                ApplyConfigurationUnitResult unitResult = result.UnitResults.First(x => x.Unit == unit);

                Assert.NotNull(unitResult);
                Assert.NotNull(unitResult.ResultInformation);
                Assert.Null(unitResult.ResultInformation.ResultCode);
                Assert.Equal(ConfigurationUnitResultSource.None, unitResult.ResultInformation.ResultSource);
                Assert.Equal(ConfigurationUnitState.Completed, unitResult.State);
                Assert.True(unitResult.PreviouslyInDesiredState);
            }

            this.VerifySummaryEvent(configurationSet, result, ConfigurationUnitResultSource.None);
        }

        /// <summary>
        /// Test when the standard set processor is used and there is a non-group unit that still exposes a group processor.
        /// </summary>
        [Fact]
        public void ApplySet_UnitGroupProcessor_WithNonGroupUnit()
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

            ApplyConfigurationSetResult result = processor.ApplySet(configurationSet, ApplyConfigurationSetFlags.None);

            Assert.NotNull(result);
            Assert.Null(result.ResultCode);
            Assert.NotNull(result.UnitResults);
            Assert.Equal(2, result.UnitResults.Count);

            foreach (ConfigurationUnit unit in new ConfigurationUnit[] { configurationUnit, configurationUnitGroup })
            {
                ApplyConfigurationUnitResult unitResult = result.UnitResults.First(x => x.Unit == unit);

                Assert.NotNull(unitResult);
                Assert.NotNull(unitResult.ResultInformation);
                Assert.Null(unitResult.ResultInformation.ResultCode);
                Assert.Equal(ConfigurationUnitResultSource.None, unitResult.ResultInformation.ResultSource);
                Assert.Equal(ConfigurationUnitState.Completed, unitResult.State);
                Assert.True(unitResult.PreviouslyInDesiredState);
            }

            this.VerifySummaryEvent(configurationSet, result, ConfigurationUnitResultSource.None);
        }
    }
}
