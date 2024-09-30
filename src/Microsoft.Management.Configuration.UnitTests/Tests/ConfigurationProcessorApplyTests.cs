// -----------------------------------------------------------------------------
// <copyright file="ConfigurationProcessorApplyTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Tests
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using System.Threading;
    using Microsoft.Management.Configuration.UnitTests.Fixtures;
    using Microsoft.Management.Configuration.UnitTests.Helpers;
    using Microsoft.VisualBasic;
    using Xunit;
    using Xunit.Abstractions;

    /// <summary>
    /// Unit tests for running apply on the processor.
    /// </summary>
    [Collection("UnitTestCollection")]
    [InProc]
    [OutOfProc]
    public class ConfigurationProcessorApplyTests : ConfigurationProcessorTestBase
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="ConfigurationProcessorApplyTests"/> class.
        /// </summary>
        /// <param name="fixture">Unit test fixture.</param>
        /// <param name="log">Log helper.</param>
        public ConfigurationProcessorApplyTests(UnitTestFixture fixture, ITestOutputHelper log)
            : base(fixture, log)
        {
        }

        /// <summary>
        /// An error creating the set processor results in an error for the function.
        /// </summary>
        [Fact]
        public void ApplySet_SetProcessorError()
        {
            ConfigurationSet configurationSet = this.ConfigurationSet();

            TestConfigurationProcessorFactory factory = new TestConfigurationProcessorFactory();
            factory.Exceptions.Add(configurationSet, new FileNotFoundException());

            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(factory);

            Assert.Throws<FileNotFoundException>(() => processor.ApplySet(configurationSet, ApplyConfigurationSetFlags.None));

            Assert.Empty(this.EventSink.Events);
        }

        /// <summary>
        /// Multiple configuration units with the same identifier.
        /// </summary>
        [Fact]
        public void ApplySet_DuplicateIdentifiers()
        {
            ConfigurationSet configurationSet = this.ConfigurationSet();
            ConfigurationUnit configurationUnit1 = this.ConfigurationUnit();
            ConfigurationUnit configurationUnit2 = this.ConfigurationUnit();
            ConfigurationUnit configurationUnitDifferentIdentifier = this.ConfigurationUnit();
            string sharedIdentifier = "SameIdentifier";
            configurationUnit1.Identifier = sharedIdentifier;
            configurationUnit2.Identifier = sharedIdentifier;
            configurationSet.Units = new ConfigurationUnit[] { configurationUnit1, configurationUnit2, configurationUnitDifferentIdentifier };

            TestConfigurationProcessorFactory factory = new TestConfigurationProcessorFactory();

            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(factory);

            ApplyConfigurationSetResult result = processor.ApplySet(configurationSet, ApplyConfigurationSetFlags.None);
            Assert.NotNull(result);
            Assert.NotNull(result.ResultCode);
            Assert.Equal(Errors.WINGET_CONFIG_ERROR_DUPLICATE_IDENTIFIER, result.ResultCode.HResult);
            Assert.Equal(3, result.UnitResults.Count);

            foreach (var configurationUnit in new ConfigurationUnit[] { configurationUnit1, configurationUnit2 })
            {
                ApplyConfigurationUnitResult? unitResult = result.UnitResults.First(x => x.Unit == configurationUnit);
                Assert.NotNull(unitResult);
                Assert.False(unitResult.PreviouslyInDesiredState);
                Assert.False(unitResult.RebootRequired);
                Assert.NotNull(unitResult.ResultInformation);
                Assert.NotNull(unitResult.ResultInformation.ResultCode);
                Assert.Equal(Errors.WINGET_CONFIG_ERROR_DUPLICATE_IDENTIFIER, unitResult.ResultInformation.ResultCode.HResult);
                Assert.Equal(ConfigurationUnitResultSource.ConfigurationSet, unitResult.ResultInformation.ResultSource);
            }

            ApplyConfigurationUnitResult unitResultDifferentIdentifier = result.UnitResults.First(x => x.Unit == configurationUnitDifferentIdentifier);
            Assert.NotNull(unitResultDifferentIdentifier);
            Assert.False(unitResultDifferentIdentifier.PreviouslyInDesiredState);
            Assert.False(unitResultDifferentIdentifier.RebootRequired);
            Assert.NotNull(unitResultDifferentIdentifier.ResultInformation);
            Assert.Null(unitResultDifferentIdentifier.ResultInformation.ResultCode);
            Assert.Equal(ConfigurationUnitResultSource.None, unitResultDifferentIdentifier.ResultInformation.ResultSource);

            this.VerifySummaryEvent(configurationSet, result, ConfigurationUnitResultSource.ConfigurationSet);
        }

        /// <summary>
        /// A configuration unit has a dependency that is not in the set.
        /// </summary>
        [Fact]
        public void ApplySet_MissingDependency()
        {
            ConfigurationSet configurationSet = this.ConfigurationSet();
            ConfigurationUnit configurationUnit = this.ConfigurationUnit();
            ConfigurationUnit configurationUnitMissingDependency = this.ConfigurationUnit();
            configurationUnit.Identifier = "Identifier";
            configurationUnitMissingDependency.Dependencies = new string[] { "Dependency" };
            configurationSet.Units = new ConfigurationUnit[] { configurationUnit, configurationUnitMissingDependency };

            TestConfigurationProcessorFactory factory = new TestConfigurationProcessorFactory();

            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(factory);

            ApplyConfigurationSetResult result = processor.ApplySet(configurationSet, ApplyConfigurationSetFlags.None);
            Assert.NotNull(result);
            Assert.NotNull(result.ResultCode);
            Assert.Equal(Errors.WINGET_CONFIG_ERROR_MISSING_DEPENDENCY, result.ResultCode.HResult);
            Assert.Equal(2, result.UnitResults.Count);

            ApplyConfigurationUnitResult unitResult = result.UnitResults.First(x => x.Unit == configurationUnit);
            Assert.NotNull(unitResult);
            Assert.False(unitResult.PreviouslyInDesiredState);
            Assert.False(unitResult.RebootRequired);
            Assert.NotNull(unitResult.ResultInformation);
            Assert.Null(unitResult.ResultInformation.ResultCode);
            Assert.Equal(ConfigurationUnitResultSource.None, unitResult.ResultInformation.ResultSource);

            unitResult = result.UnitResults.First(x => x.Unit == configurationUnitMissingDependency);
            Assert.NotNull(unitResult);
            Assert.False(unitResult.PreviouslyInDesiredState);
            Assert.False(unitResult.RebootRequired);
            Assert.NotNull(unitResult.ResultInformation);
            Assert.NotNull(unitResult.ResultInformation.ResultCode);
            Assert.Equal(Errors.WINGET_CONFIG_ERROR_MISSING_DEPENDENCY, unitResult.ResultInformation.ResultCode.HResult);
            Assert.Equal(ConfigurationUnitResultSource.ConfigurationSet, unitResult.ResultInformation.ResultSource);

            this.VerifySummaryEvent(configurationSet, result, ConfigurationUnitResultSource.ConfigurationSet);
        }

        /// <summary>
        /// The configuration set has a dependency cycle.
        /// </summary>
        [Fact]
        public void ApplySet_DependencyCycle()
        {
            ConfigurationSet configurationSet = this.ConfigurationSet();
            ConfigurationUnit configurationUnit1 = this.ConfigurationUnit();
            ConfigurationUnit configurationUnit2 = this.ConfigurationUnit();
            ConfigurationUnit configurationUnit3 = this.ConfigurationUnit();
            configurationUnit1.Identifier = "Identifier1";
            configurationUnit2.Identifier = "Identifier2";
            configurationUnit3.Identifier = "Identifier3";
            configurationUnit1.Dependencies = new string[] { "Identifier3" };
            configurationUnit2.Dependencies = new string[] { "Identifier1" };
            configurationUnit3.Dependencies = new string[] { "Identifier2" };
            configurationSet.Units = new ConfigurationUnit[] { configurationUnit1, configurationUnit2, configurationUnit3 };

            TestConfigurationProcessorFactory factory = new TestConfigurationProcessorFactory();

            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(factory);

            ApplyConfigurationSetResult result = processor.ApplySet(configurationSet, ApplyConfigurationSetFlags.None);
            Assert.NotNull(result);
            Assert.NotNull(result.ResultCode);
            Assert.Equal(Errors.WINGET_CONFIG_ERROR_SET_DEPENDENCY_CYCLE, result.ResultCode.HResult);
            Assert.Equal(3, result.UnitResults.Count);

            foreach (var unitResult in result.UnitResults)
            {
                Assert.NotNull(unitResult);
                Assert.False(unitResult.PreviouslyInDesiredState);
                Assert.False(unitResult.RebootRequired);
                Assert.NotNull(unitResult.ResultInformation);
                Assert.NotNull(unitResult.ResultInformation.ResultCode);
                Assert.Equal(Errors.WINGET_CONFIG_ERROR_DEPENDENCY_UNSATISFIED, unitResult.ResultInformation.ResultCode.HResult);
                Assert.Equal(ConfigurationUnitResultSource.Precondition, unitResult.ResultInformation.ResultSource);
            }

            this.VerifySummaryEvent(configurationSet, result, ConfigurationUnitResultSource.Precondition);
        }

        /// <summary>
        /// Checks that the intent for configuration units is handled properly.
        /// </summary>
        [Fact]
        public void ApplySet_IntentRespected()
        {
            ConfigurationSet configurationSet = this.ConfigurationSet();
            ConfigurationUnit configurationUnitAssert = this.ConfigurationUnit().Assign(new { Intent = ConfigurationUnitIntent.Assert });
            ConfigurationUnit configurationUnitInform = this.ConfigurationUnit().Assign(new { Intent = ConfigurationUnitIntent.Inform });
            ConfigurationUnit configurationUnitApply = this.ConfigurationUnit().Assign(new { Intent = ConfigurationUnitIntent.Apply });
            configurationSet.Units = new ConfigurationUnit[] { configurationUnitInform, configurationUnitApply, configurationUnitAssert };

            TestConfigurationProcessorFactory factory = new TestConfigurationProcessorFactory();
            TestConfigurationSetProcessor setProcessor = factory.CreateTestProcessor(configurationSet);
            TestConfigurationUnitProcessor unitProcessorAssert = setProcessor.CreateTestProcessor(configurationUnitAssert);
            TestConfigurationUnitProcessor unitProcessorInform = setProcessor.CreateTestProcessor(configurationUnitInform);
            TestConfigurationUnitProcessor unitProcessorApply = setProcessor.CreateTestProcessor(configurationUnitApply);
            unitProcessorApply.TestSettingsDelegate = () => new TestSettingsResultInstance(configurationUnitApply) { TestResult = ConfigurationTestResult.Negative };

            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(factory);

            ApplyConfigurationSetResult result = processor.ApplySet(configurationSet, ApplyConfigurationSetFlags.None);
            Assert.NotNull(result);
            Assert.Null(result.ResultCode);
            Assert.Equal(3, result.UnitResults.Count);

            foreach (var unitResult in result.UnitResults)
            {
                Assert.NotNull(unitResult);
                Assert.False(unitResult.PreviouslyInDesiredState);
                Assert.False(unitResult.RebootRequired);
                Assert.NotNull(unitResult.ResultInformation);
                Assert.Null(unitResult.ResultInformation.ResultCode);
                Assert.Equal(ConfigurationUnitResultSource.None, unitResult.ResultInformation.ResultSource);
            }

            Assert.Equal(1, unitProcessorAssert.TestSettingsCalls);
            Assert.Equal(0, unitProcessorAssert.GetSettingsCalls);
            Assert.Equal(0, unitProcessorAssert.ApplySettingsCalls);

            Assert.Equal(0, unitProcessorInform.TestSettingsCalls);
            Assert.Equal(1, unitProcessorInform.GetSettingsCalls);
            Assert.Equal(0, unitProcessorInform.ApplySettingsCalls);

            Assert.Equal(1, unitProcessorApply.TestSettingsCalls);
            Assert.Equal(0, unitProcessorApply.GetSettingsCalls);
            Assert.Equal(1, unitProcessorApply.ApplySettingsCalls);

            this.VerifySummaryEvent(configurationSet, result, ConfigurationUnitResultSource.None);
        }

        /// <summary>
        /// An assertion fails to run.
        /// </summary>
        [Fact]
        public void ApplySet_AssertionFailure()
        {
            ConfigurationSet configurationSet = this.ConfigurationSet();
            ConfigurationUnit configurationUnitAssert = this.ConfigurationUnit().Assign(new { Intent = ConfigurationUnitIntent.Assert });
            ConfigurationUnit configurationUnitApply = this.ConfigurationUnit().Assign(new { Intent = ConfigurationUnitIntent.Apply });
            configurationSet.Units = new ConfigurationUnit[] { configurationUnitApply, configurationUnitAssert };

            TestConfigurationProcessorFactory factory = new TestConfigurationProcessorFactory();
            TestConfigurationSetProcessor setProcessor = factory.CreateTestProcessor(configurationSet);
            TestConfigurationUnitProcessor unitProcessorAssert = setProcessor.CreateTestProcessor(configurationUnitAssert);
            unitProcessorAssert.TestSettingsDelegate = () => throw new NullReferenceException();
            TestConfigurationUnitProcessor unitProcessorApply = setProcessor.CreateTestProcessor(configurationUnitApply);
            unitProcessorApply.TestSettingsDelegate = () => new TestSettingsResultInstance(configurationUnitApply) { TestResult = ConfigurationTestResult.Negative };

            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(factory);

            ApplyConfigurationSetResult result = processor.ApplySet(configurationSet, ApplyConfigurationSetFlags.None);
            Assert.NotNull(result);
            Assert.NotNull(result.ResultCode);
            Assert.Equal(Errors.WINGET_CONFIG_ERROR_ASSERTION_FAILED, result.ResultCode.HResult);
            Assert.Equal(2, result.UnitResults.Count);

            ApplyConfigurationUnitResult unitResult = result.UnitResults.First(x => x.Unit == configurationUnitAssert);
            Assert.NotNull(unitResult);
            Assert.False(unitResult.PreviouslyInDesiredState);
            Assert.False(unitResult.RebootRequired);
            Assert.NotNull(unitResult.ResultInformation);
            Assert.NotNull(unitResult.ResultInformation.ResultCode);
            Assert.IsType<NullReferenceException>(unitResult.ResultInformation.ResultCode);
            Assert.Equal(ConfigurationUnitResultSource.Internal, unitResult.ResultInformation.ResultSource);

            unitResult = result.UnitResults.First(x => x.Unit == configurationUnitApply);
            Assert.NotNull(unitResult);
            Assert.False(unitResult.PreviouslyInDesiredState);
            Assert.False(unitResult.RebootRequired);
            Assert.NotNull(unitResult.ResultInformation);
            Assert.NotNull(unitResult.ResultInformation.ResultCode);
            Assert.Equal(Errors.WINGET_CONFIG_ERROR_ASSERTION_FAILED, unitResult.ResultInformation.ResultCode.HResult);
            Assert.Equal(ConfigurationUnitResultSource.Precondition, unitResult.ResultInformation.ResultSource);

            this.VerifySummaryEvent(configurationSet, result, ConfigurationUnitResultSource.Internal);
        }

        /// <summary>
        /// An assertion is found to be false.
        /// </summary>
        [Fact]
        public void ApplySet_AssertionNegative()
        {
            ConfigurationSet configurationSet = this.ConfigurationSet();
            ConfigurationUnit configurationUnitAssert = this.ConfigurationUnit().Assign(new { Intent = ConfigurationUnitIntent.Assert });
            ConfigurationUnit configurationUnitApply = this.ConfigurationUnit().Assign(new { Intent = ConfigurationUnitIntent.Apply });
            configurationSet.Units = new ConfigurationUnit[] { configurationUnitApply, configurationUnitAssert };

            TestConfigurationProcessorFactory factory = new TestConfigurationProcessorFactory();
            TestConfigurationSetProcessor setProcessor = factory.CreateTestProcessor(configurationSet);
            TestConfigurationUnitProcessor unitProcessorAssert = setProcessor.CreateTestProcessor(configurationUnitAssert);
            unitProcessorAssert.TestSettingsDelegate = () => new TestSettingsResultInstance(configurationUnitAssert) { TestResult = ConfigurationTestResult.Negative };
            TestConfigurationUnitProcessor unitProcessorApply = setProcessor.CreateTestProcessor(configurationUnitApply);
            unitProcessorApply.TestSettingsDelegate = () => new TestSettingsResultInstance(configurationUnitApply) { TestResult = ConfigurationTestResult.Negative };

            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(factory);

            ApplyConfigurationSetResult result = processor.ApplySet(configurationSet, ApplyConfigurationSetFlags.None);
            Assert.NotNull(result);
            Assert.NotNull(result.ResultCode);
            Assert.Equal(Errors.WINGET_CONFIG_ERROR_ASSERTION_FAILED, result.ResultCode.HResult);
            Assert.Equal(2, result.UnitResults.Count);

            foreach (var unitResult in result.UnitResults)
            {
                Assert.NotNull(unitResult);
                Assert.False(unitResult.PreviouslyInDesiredState);
                Assert.False(unitResult.RebootRequired);
                Assert.NotNull(unitResult.ResultInformation);
                Assert.NotNull(unitResult.ResultInformation.ResultCode);
                Assert.Equal(Errors.WINGET_CONFIG_ERROR_ASSERTION_FAILED, unitResult.ResultInformation.ResultCode.HResult);
                Assert.Equal(ConfigurationUnitResultSource.Precondition, unitResult.ResultInformation.ResultSource);
            }

            this.VerifySummaryEvent(configurationSet, result, ConfigurationUnitResultSource.Precondition);
        }

        /// <summary>
        /// A unit in the correct state is not applied again.
        /// </summary>
        [Fact]
        public void ApplySet_UnitAlreadyInCorrectState()
        {
            ConfigurationSet configurationSet = this.ConfigurationSet();
            ConfigurationUnit configurationUnit = this.ConfigurationUnit().Assign(new { Intent = ConfigurationUnitIntent.Apply });
            configurationSet.Units = new ConfigurationUnit[] { configurationUnit };

            TestConfigurationProcessorFactory factory = new TestConfigurationProcessorFactory();
            TestConfigurationSetProcessor setProcessor = factory.CreateTestProcessor(configurationSet);
            TestConfigurationUnitProcessor unitProcessor = setProcessor.CreateTestProcessor(configurationUnit);
            unitProcessor.TestSettingsDelegate = () => new TestSettingsResultInstance(configurationUnit) { TestResult = ConfigurationTestResult.Positive };

            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(factory);

            ApplyConfigurationSetResult result = processor.ApplySet(configurationSet, ApplyConfigurationSetFlags.None);
            Assert.NotNull(result);
            Assert.Null(result.ResultCode);
            Assert.Equal(1, result.UnitResults.Count);

            ApplyConfigurationUnitResult unitResult = result.UnitResults.First();
            Assert.NotNull(unitResult);
            Assert.True(unitResult.PreviouslyInDesiredState);
            Assert.False(unitResult.RebootRequired);
            Assert.NotNull(unitResult.ResultInformation);
            Assert.Null(unitResult.ResultInformation.ResultCode);
            Assert.Equal(ConfigurationUnitResultSource.None, unitResult.ResultInformation.ResultSource);

            this.VerifySummaryEvent(configurationSet, result, ConfigurationUnitResultSource.None);
        }

        /// <summary>
        /// Checks the progress reporting.
        /// </summary>
        [Fact]
        public void ApplySet_Progress()
        {
            ConfigurationSet configurationSet = this.ConfigurationSet();
            ConfigurationUnit assert1 = this.ConfigurationUnit().Assign(new { Intent = ConfigurationUnitIntent.Assert, Identifier = "Assert1" });
            ConfigurationUnit assert2 = this.ConfigurationUnit().Assign(new { Intent = ConfigurationUnitIntent.Assert, Identifier = "Assert2", Dependencies = new string[] { assert1.Identifier } });
            ConfigurationUnit inform1 = this.ConfigurationUnit().Assign(new { Intent = ConfigurationUnitIntent.Inform, Identifier = "Inform1" });
            ConfigurationUnit apply1 = this.ConfigurationUnit().Assign(new { Intent = ConfigurationUnitIntent.Apply, Identifier = "Apply1" });
            ConfigurationUnit apply2 = this.ConfigurationUnit().Assign(new { Intent = ConfigurationUnitIntent.Apply, Identifier = "Apply2" });
            ConfigurationUnit apply3 = this.ConfigurationUnit().Assign(new { Intent = ConfigurationUnitIntent.Apply, Identifier = "Apply3", Dependencies = new string[] { apply1.Identifier, apply2.Identifier } });
            ConfigurationUnit apply4 = this.ConfigurationUnit().Assign(new { Intent = ConfigurationUnitIntent.Apply, Identifier = "Apply4", IsActive = false });
            ConfigurationUnit apply5 = this.ConfigurationUnit().Assign(new { Intent = ConfigurationUnitIntent.Apply, Identifier = "Apply5", Dependencies = new string[] { apply4.Identifier } });
            configurationSet.Units = new ConfigurationUnit[] { assert2, assert1, inform1, apply1, apply3, apply4,  apply2, apply5 };

            ManualResetEvent startProcessing = new ManualResetEvent(false);

            TestConfigurationProcessorFactory factory = new TestConfigurationProcessorFactory();
            factory.CreateSetProcessorDelegate = (f, c) =>
            {
                startProcessing.WaitOne();
                return f.DefaultCreateSetProcessor(c);
            };
            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(factory);
            List<ConfigurationSetChangeData> progressEvents = new List<ConfigurationSetChangeData>();

            var operation = processor.ApplySetAsync(configurationSet, ApplyConfigurationSetFlags.None);
            operation.Progress = (asyncInfo, progress) => progressEvents.Add(progress);
            startProcessing.Set();
            operation.AsTask().Wait();
            ApplyConfigurationSetResult result = operation.GetResults();

            Assert.NotNull(result);
            Assert.NotNull(result.ResultCode);
            Assert.Equal(Errors.WINGET_CONFIG_ERROR_DEPENDENCY_UNSATISFIED, result.ResultCode.HResult);
            Assert.NotNull(result.UnitResults);
            Assert.Equal(configurationSet.Units.Count, result.UnitResults.Count);

            // Verify that progress events match the expected
            ExpectedConfigurationChangeData[] expectedProgress = new ExpectedConfigurationChangeData[]
            {
                new ExpectedConfigurationChangeData() { Change = ConfigurationSetChangeEventType.SetStateChanged, SetState = ConfigurationSetState.InProgress },
                new ExpectedConfigurationChangeData() { Change = ConfigurationSetChangeEventType.UnitStateChanged, UnitState = ConfigurationUnitState.InProgress, Unit = assert1 },
                new ExpectedConfigurationChangeData() { Change = ConfigurationSetChangeEventType.UnitStateChanged, UnitState = ConfigurationUnitState.Completed, Unit = assert1 },
                new ExpectedConfigurationChangeData() { Change = ConfigurationSetChangeEventType.UnitStateChanged, UnitState = ConfigurationUnitState.InProgress, Unit = assert2 },
                new ExpectedConfigurationChangeData() { Change = ConfigurationSetChangeEventType.UnitStateChanged, UnitState = ConfigurationUnitState.Completed, Unit = assert2 },
                new ExpectedConfigurationChangeData() { Change = ConfigurationSetChangeEventType.UnitStateChanged, UnitState = ConfigurationUnitState.InProgress, Unit = inform1 },
                new ExpectedConfigurationChangeData() { Change = ConfigurationSetChangeEventType.UnitStateChanged, UnitState = ConfigurationUnitState.Completed, Unit = inform1 },
                new ExpectedConfigurationChangeData() { Change = ConfigurationSetChangeEventType.UnitStateChanged, UnitState = ConfigurationUnitState.InProgress, Unit = apply1 },
                new ExpectedConfigurationChangeData() { Change = ConfigurationSetChangeEventType.UnitStateChanged, UnitState = ConfigurationUnitState.Completed, Unit = apply1 },
                new ExpectedConfigurationChangeData() { Change = ConfigurationSetChangeEventType.UnitStateChanged, UnitState = ConfigurationUnitState.Skipped, Unit = apply4, HResult = Errors.WINGET_CONFIG_ERROR_MANUALLY_SKIPPED },
                new ExpectedConfigurationChangeData() { Change = ConfigurationSetChangeEventType.UnitStateChanged, UnitState = ConfigurationUnitState.InProgress, Unit = apply2 },
                new ExpectedConfigurationChangeData() { Change = ConfigurationSetChangeEventType.UnitStateChanged, UnitState = ConfigurationUnitState.Completed, Unit = apply2 },
                new ExpectedConfigurationChangeData() { Change = ConfigurationSetChangeEventType.UnitStateChanged, UnitState = ConfigurationUnitState.InProgress, Unit = apply3 },
                new ExpectedConfigurationChangeData() { Change = ConfigurationSetChangeEventType.UnitStateChanged, UnitState = ConfigurationUnitState.Completed, Unit = apply3 },
                new ExpectedConfigurationChangeData() { Change = ConfigurationSetChangeEventType.UnitStateChanged, UnitState = ConfigurationUnitState.Skipped, Unit = apply5, HResult = Errors.WINGET_CONFIG_ERROR_DEPENDENCY_UNSATISFIED },
                new ExpectedConfigurationChangeData() { Change = ConfigurationSetChangeEventType.SetStateChanged, SetState = ConfigurationSetState.Completed },
            };

            // Drop the pending event if it happens to be present
            if (progressEvents.Count > 0 && progressEvents[0].Change == ConfigurationSetChangeEventType.SetStateChanged && progressEvents[0].SetState == ConfigurationSetState.Pending)
            {
                progressEvents.RemoveAt(0);
            }

            Assert.Equal(expectedProgress.Count(), progressEvents.Count);

            for (int i = 0; i < progressEvents.Count; ++i)
            {
                this.Log.WriteLine($"Comparing event {i}");
                Assert.Equal(expectedProgress[i].Change, progressEvents[i].Change);
                switch (expectedProgress[i].Change)
                {
                    case ConfigurationSetChangeEventType.SetStateChanged:
                        Assert.Equal(expectedProgress[i].SetState, progressEvents[i].SetState);
                        break;
                    case ConfigurationSetChangeEventType.UnitStateChanged:
                        Assert.Equal(expectedProgress[i].UnitState, progressEvents[i].UnitState);
                        Assert.Same(expectedProgress[i].Unit, progressEvents[i].Unit);

                        Assert.NotNull(progressEvents[i].ResultInformation);
                        if (expectedProgress[i].HResult == 0)
                        {
                            Assert.Null(progressEvents[i].ResultInformation.ResultCode);
                            Assert.Equal(ConfigurationUnitResultSource.None, progressEvents[i].ResultInformation.ResultSource);
                        }
                        else
                        {
                            Assert.NotNull(progressEvents[i].ResultInformation.ResultCode);
                            Assert.Equal(expectedProgress[i].HResult, progressEvents[i].ResultInformation.ResultCode.HResult);
                            Assert.Equal(ConfigurationUnitResultSource.Precondition, progressEvents[i].ResultInformation.ResultSource);
                        }

                        break;
                    default:
                        Assert.Fail("Unexpected ConfigurationSetChangeEventType value");
                        break;
                }
            }

            this.VerifySummaryEvent(configurationSet, result, ConfigurationUnitResultSource.Precondition);
        }

        /// <summary>
        /// Ensures that multiple apply operations are sequenced.
        /// </summary>
        [Fact]
        public void ApplySet_Sequenced()
        {
            ConfigurationSet configurationSet = this.ConfigurationSet();
            ConfigurationUnit configurationUnitApply = this.ConfigurationUnit().Assign(new { Intent = ConfigurationUnitIntent.Apply });
            configurationSet.Units = new ConfigurationUnit[] { configurationUnitApply };

            ManualResetEvent startProcessing = new ManualResetEvent(true);
            TestConfigurationProcessorFactory factory = new TestConfigurationProcessorFactory();
            factory.CreateSetProcessorDelegate = (f, c) =>
            {
                WaitOn(startProcessing);
                return f.DefaultCreateSetProcessor(c);
            };

            TestConfigurationSetProcessor setProcessor = factory.CreateTestProcessor(configurationSet);
            TestConfigurationUnitProcessor unitProcessorApply = setProcessor.CreateTestProcessor(configurationUnitApply);
            unitProcessorApply.TestSettingsDelegate = () => new TestSettingsResultInstance(configurationUnitApply) { TestResult = ConfigurationTestResult.Negative };

            ManualResetEvent applyEventWaiting = new ManualResetEvent(false);
            ManualResetEvent completeApplyEvent = new ManualResetEvent(false);
            unitProcessorApply.ApplySettingsDelegate = () =>
            {
                applyEventWaiting.Set();
                WaitOn(completeApplyEvent);
                return new ApplySettingsResultInstance(configurationUnitApply);
            };

            ConfigurationSet configurationSetThatWaits = this.ConfigurationSet();
            ConfigurationUnit configurationUnitThatWaits = this.ConfigurationUnit().Assign(new { Intent = ConfigurationUnitIntent.Apply });
            configurationSetThatWaits.Units = new ConfigurationUnit[] { configurationUnitThatWaits };

            TestConfigurationSetProcessor setThatWaitsProcessor = factory.CreateTestProcessor(configurationSetThatWaits);
            TestConfigurationUnitProcessor unitThatWaitsProcessor = setProcessor.CreateTestProcessor(configurationUnitThatWaits);
            unitThatWaitsProcessor.TestSettingsDelegate = () => new TestSettingsResultInstance(configurationUnitApply) { TestResult = ConfigurationTestResult.Negative };

            ManualResetEvent waitingUnitApply = new ManualResetEvent(false);
            unitThatWaitsProcessor.ApplySettingsDelegate = () =>
            {
                WaitOn(waitingUnitApply);
                return new ApplySettingsResultInstance(configurationUnitThatWaits);
            };

            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(factory);

            var applySetOperation = processor.ApplySetAsync(configurationSet, ApplyConfigurationSetFlags.None);
            WaitOn(applyEventWaiting);

            startProcessing.Reset();
            var waitingSetOperation = processor.ApplySetAsync(configurationSetThatWaits, ApplyConfigurationSetFlags.None);
            AutoResetEvent waitingProgress = new AutoResetEvent(false);
            ConfigurationSetState progressState = ConfigurationSetState.Unknown;
            waitingSetOperation.Progress += (result, changeData) =>
            {
                if (changeData.Change == ConfigurationSetChangeEventType.SetStateChanged)
                {
                    progressState = changeData.SetState;
                    waitingProgress.Set();
                }
            };

            startProcessing.Set();
            WaitOn(waitingProgress);
            Assert.Equal(ConfigurationSetState.Pending, progressState);

            completeApplyEvent.Set();
            WaitOn(waitingProgress);
            Assert.Equal(ConfigurationSetState.InProgress, progressState);

            waitingUnitApply.Set();
            WaitOn(waitingProgress);
            Assert.Equal(ConfigurationSetState.Completed, progressState);

            waitingSetOperation.AsTask().Wait();
        }

        /// <summary>
        /// Ensures that a consistency check apply is not blocked.
        /// </summary>
        [Fact]
        public void ApplySet_ConsistencyCheckNotSequenced()
        {
            ConfigurationSet configurationSet = this.ConfigurationSet();
            ConfigurationUnit configurationUnitApply = this.ConfigurationUnit().Assign(new { Intent = ConfigurationUnitIntent.Apply });
            configurationSet.Units = new ConfigurationUnit[] { configurationUnitApply };

            ManualResetEvent startProcessing = new ManualResetEvent(true);
            TestConfigurationProcessorFactory factory = new TestConfigurationProcessorFactory();
            factory.CreateSetProcessorDelegate = (f, c) =>
            {
                WaitOn(startProcessing);
                return f.DefaultCreateSetProcessor(c);
            };

            TestConfigurationSetProcessor setProcessor = factory.CreateTestProcessor(configurationSet);
            TestConfigurationUnitProcessor unitProcessorApply = setProcessor.CreateTestProcessor(configurationUnitApply);
            unitProcessorApply.TestSettingsDelegate = () => new TestSettingsResultInstance(configurationUnitApply) { TestResult = ConfigurationTestResult.Negative };

            ManualResetEvent applyEventWaiting = new ManualResetEvent(false);
            ManualResetEvent completeApplyEvent = new ManualResetEvent(false);
            unitProcessorApply.ApplySettingsDelegate = () =>
            {
                applyEventWaiting.Set();
                WaitOn(completeApplyEvent);
                return new ApplySettingsResultInstance(configurationUnitApply);
            };

            ConfigurationSet configurationSetThatWaits = this.ConfigurationSet();
            ConfigurationUnit configurationUnitThatWaits = this.ConfigurationUnit().Assign(new { Intent = ConfigurationUnitIntent.Apply });
            configurationSetThatWaits.Units = new ConfigurationUnit[] { configurationUnitThatWaits };

            TestConfigurationSetProcessor setThatWaitsProcessor = factory.CreateTestProcessor(configurationSetThatWaits);
            TestConfigurationUnitProcessor unitThatWaitsProcessor = setProcessor.CreateTestProcessor(configurationUnitThatWaits);
            unitThatWaitsProcessor.TestSettingsDelegate = () => new TestSettingsResultInstance(configurationUnitApply) { TestResult = ConfigurationTestResult.Negative };

            ManualResetEvent waitingUnitApply = new ManualResetEvent(false);
            unitThatWaitsProcessor.ApplySettingsDelegate = () =>
            {
                WaitOn(waitingUnitApply);
                return new ApplySettingsResultInstance(configurationUnitThatWaits);
            };

            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(factory);

            var applySetOperation = processor.ApplySetAsync(configurationSet, ApplyConfigurationSetFlags.None);
            WaitOn(applyEventWaiting);

            startProcessing.Reset();
            var waitingSetOperation = processor.ApplySetAsync(configurationSetThatWaits, ApplyConfigurationSetFlags.PerformConsistencyCheckOnly);
            Assert.True(waitingSetOperation.AsTask().Wait(10000));

            completeApplyEvent.Set();
        }

        private static void WaitOn(WaitHandle waitable)
        {
            if (!waitable.WaitOne(10000))
            {
                throw new TimeoutException();
            }
        }

        private struct ExpectedConfigurationChangeData
        {
            public ConfigurationSetChangeEventType Change;
            public ConfigurationSetState SetState;
            public ConfigurationUnitState UnitState;
            public int HResult;
            public ConfigurationUnit Unit;
        }
    }
}
