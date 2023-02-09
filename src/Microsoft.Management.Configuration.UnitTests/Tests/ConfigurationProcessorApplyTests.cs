// -----------------------------------------------------------------------------
// <copyright file="ConfigurationProcessorApplyTests.cs" company="Microsoft Corporation">
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
    using System.Threading;
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
            ConfigurationSet configurationSet = new ConfigurationSet();

            TestConfigurationProcessorFactory factory = new TestConfigurationProcessorFactory();
            factory.Exceptions.Add(configurationSet, new FileNotFoundException());

            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(factory);

            Assert.Throws<FileNotFoundException>(() => processor.ApplySet(configurationSet, ApplyConfigurationSetFlags.None));
        }

        /// <summary>
        /// Multiple configuration units with the same identifier.
        /// </summary>
        [Fact]
        public void ApplySet_DuplicateIdentifiers()
        {
            ConfigurationSet configurationSet = new ConfigurationSet();
            ConfigurationUnit configurationUnit1 = new ConfigurationUnit();
            ConfigurationUnit configurationUnit2 = new ConfigurationUnit();
            ConfigurationUnit configurationUnitDifferentIdentifier = new ConfigurationUnit();
            string sharedIdentifier = "SameIdentifier";
            configurationUnit1.Identifier = sharedIdentifier;
            configurationUnit2.Identifier = sharedIdentifier;
            configurationSet.ConfigurationUnits = new ConfigurationUnit[] { configurationUnit1, configurationUnit2, configurationUnitDifferentIdentifier };

            TestConfigurationProcessorFactory factory = new TestConfigurationProcessorFactory();

            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(factory);

            ApplyConfigurationSetResult result = processor.ApplySet(configurationSet, ApplyConfigurationSetFlags.None);
            Assert.NotNull(result);
            Assert.NotNull(result.ResultCode);
            Assert.Equal(Errors.WINGET_CONFIG_ERROR_DUPLICATE_IDENTIFIER, result.ResultCode.HResult);
            Assert.Equal(3, result.UnitResults.Count);

            foreach (var configurationUnit in new ConfigurationUnit[] { configurationUnit1, configurationUnit2 })
            {
                ApplyConfigurationUnitResult unitResult = result.UnitResults.First(x => x.Unit == configurationUnit);
                Assert.NotNull(unitResult);
                Assert.False(unitResult.PreviouslyInDesiredState);
                Assert.False(unitResult.RebootRequired);
                Assert.NotNull(unitResult.ResultInformation);
                Assert.NotNull(unitResult.ResultInformation.ResultCode);
                Assert.Equal(Errors.WINGET_CONFIG_ERROR_DUPLICATE_IDENTIFIER, unitResult.ResultInformation.ResultCode.HResult);
            }

            ApplyConfigurationUnitResult unitResultDifferentIdentifier = result.UnitResults.First(x => x.Unit == configurationUnitDifferentIdentifier);
            Assert.NotNull(unitResultDifferentIdentifier);
            Assert.False(unitResultDifferentIdentifier.PreviouslyInDesiredState);
            Assert.False(unitResultDifferentIdentifier.RebootRequired);
            Assert.NotNull(unitResultDifferentIdentifier.ResultInformation);
            Assert.Null(unitResultDifferentIdentifier.ResultInformation.ResultCode);
        }

        /// <summary>
        /// A configuration unit has a dependency that is not in the set.
        /// </summary>
        [Fact]
        public void ApplySet_MissingDependency()
        {
            ConfigurationSet configurationSet = new ConfigurationSet();
            ConfigurationUnit configurationUnit = new ConfigurationUnit();
            ConfigurationUnit configurationUnitMissingDependency = new ConfigurationUnit();
            configurationUnit.Identifier = "Identifier";
            configurationUnitMissingDependency.Dependencies = new string[] { "Dependency" };
            configurationSet.ConfigurationUnits = new ConfigurationUnit[] { configurationUnit, configurationUnitMissingDependency };

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

            unitResult = result.UnitResults.First(x => x.Unit == configurationUnitMissingDependency);
            Assert.NotNull(unitResult);
            Assert.False(unitResult.PreviouslyInDesiredState);
            Assert.False(unitResult.RebootRequired);
            Assert.NotNull(unitResult.ResultInformation);
            Assert.NotNull(unitResult.ResultInformation.ResultCode);
            Assert.Equal(Errors.WINGET_CONFIG_ERROR_MISSING_DEPENDENCY, unitResult.ResultInformation.ResultCode.HResult);
        }

        /// <summary>
        /// The configuration set has a dependency cycle.
        /// </summary>
        [Fact]
        public void ApplySet_DependencyCycle()
        {
            ConfigurationSet configurationSet = new ConfigurationSet();
            ConfigurationUnit configurationUnit1 = new ConfigurationUnit();
            ConfigurationUnit configurationUnit2 = new ConfigurationUnit();
            ConfigurationUnit configurationUnit3 = new ConfigurationUnit();
            configurationUnit1.Identifier = "Identifier1";
            configurationUnit2.Identifier = "Identifier2";
            configurationUnit3.Identifier = "Identifier3";
            configurationUnit1.Dependencies = new string[] { "Identifier3" };
            configurationUnit2.Dependencies = new string[] { "Identifier1" };
            configurationUnit3.Dependencies = new string[] { "Identifier2" };
            configurationSet.ConfigurationUnits = new ConfigurationUnit[] { configurationUnit1, configurationUnit2, configurationUnit3 };

            TestConfigurationProcessorFactory factory = new TestConfigurationProcessorFactory();

            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(factory);

            ApplyConfigurationSetResult result = processor.ApplySet(configurationSet, ApplyConfigurationSetFlags.None);
            Assert.NotNull(result);
            Assert.NotNull(result.ResultCode);
            Assert.Equal(Errors.WINGET_CONFIG_ERROR_DEPENDENCY_UNSATISFIED, result.ResultCode.HResult);
            Assert.Equal(3, result.UnitResults.Count);

            foreach (var unitResult in result.UnitResults)
            {
                Assert.NotNull(unitResult);
                Assert.False(unitResult.PreviouslyInDesiredState);
                Assert.False(unitResult.RebootRequired);
                Assert.NotNull(unitResult.ResultInformation);
                Assert.NotNull(unitResult.ResultInformation.ResultCode);
                Assert.Equal(Errors.WINGET_CONFIG_ERROR_DEPENDENCY_UNSATISFIED, unitResult.ResultInformation.ResultCode.HResult);
            }
        }

        /// <summary>
        /// Checks that the intent for configuration units is handled properly.
        /// </summary>
        [Fact]
        public void ApplySet_IntentRespected()
        {
            ConfigurationSet configurationSet = new ConfigurationSet();
            ConfigurationUnit configurationUnitAssert = new ConfigurationUnit { Intent = ConfigurationUnitIntent.Assert };
            ConfigurationUnit configurationUnitInform = new ConfigurationUnit { Intent = ConfigurationUnitIntent.Inform };
            ConfigurationUnit configurationUnitApply = new ConfigurationUnit { Intent = ConfigurationUnitIntent.Apply };
            configurationSet.ConfigurationUnits = new ConfigurationUnit[] { configurationUnitInform, configurationUnitApply, configurationUnitAssert };

            TestConfigurationProcessorFactory factory = new TestConfigurationProcessorFactory();
            TestConfigurationSetProcessor setProcessor = factory.CreateTestProcessor(configurationSet);
            TestConfigurationUnitProcessor unitProcessorAssert = setProcessor.CreateTestProcessor(configurationUnitAssert);
            TestConfigurationUnitProcessor unitProcessorInform = setProcessor.CreateTestProcessor(configurationUnitInform);
            TestConfigurationUnitProcessor unitProcessorApply = setProcessor.CreateTestProcessor(configurationUnitApply);
            unitProcessorApply.TestSettingsDelegate = () => new TestSettingsResult { TestResult = ConfigurationTestResult.Negative };

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
        }

        /// <summary>
        /// An assertion fails to run.
        /// </summary>
        [Fact]
        public void ApplySet_AssertionFailure()
        {
            ConfigurationSet configurationSet = new ConfigurationSet();
            ConfigurationUnit configurationUnitAssert = new ConfigurationUnit { Intent = ConfigurationUnitIntent.Assert };
            ConfigurationUnit configurationUnitApply = new ConfigurationUnit { Intent = ConfigurationUnitIntent.Apply };
            configurationSet.ConfigurationUnits = new ConfigurationUnit[] { configurationUnitApply, configurationUnitAssert };

            TestConfigurationProcessorFactory factory = new TestConfigurationProcessorFactory();
            TestConfigurationSetProcessor setProcessor = factory.CreateTestProcessor(configurationSet);
            TestConfigurationUnitProcessor unitProcessorAssert = setProcessor.CreateTestProcessor(configurationUnitAssert);
            unitProcessorAssert.TestSettingsDelegate = () => throw new NullReferenceException();
            TestConfigurationUnitProcessor unitProcessorApply = setProcessor.CreateTestProcessor(configurationUnitApply);
            unitProcessorApply.TestSettingsDelegate = () => new TestSettingsResult { TestResult = ConfigurationTestResult.Negative };

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

            unitResult = result.UnitResults.First(x => x.Unit == configurationUnitApply);
            Assert.NotNull(unitResult);
            Assert.False(unitResult.PreviouslyInDesiredState);
            Assert.False(unitResult.RebootRequired);
            Assert.NotNull(unitResult.ResultInformation);
            Assert.NotNull(unitResult.ResultInformation.ResultCode);
            Assert.Equal(Errors.WINGET_CONFIG_ERROR_ASSERTION_FAILED, unitResult.ResultInformation.ResultCode.HResult);
        }

        /// <summary>
        /// An assertion is found to be false.
        /// </summary>
        [Fact]
        public void ApplySet_AssertionNegative()
        {
            ConfigurationSet configurationSet = new ConfigurationSet();
            ConfigurationUnit configurationUnitAssert = new ConfigurationUnit { Intent = ConfigurationUnitIntent.Assert };
            ConfigurationUnit configurationUnitApply = new ConfigurationUnit { Intent = ConfigurationUnitIntent.Apply };
            configurationSet.ConfigurationUnits = new ConfigurationUnit[] { configurationUnitApply, configurationUnitAssert };

            TestConfigurationProcessorFactory factory = new TestConfigurationProcessorFactory();
            TestConfigurationSetProcessor setProcessor = factory.CreateTestProcessor(configurationSet);
            TestConfigurationUnitProcessor unitProcessorAssert = setProcessor.CreateTestProcessor(configurationUnitAssert);
            unitProcessorAssert.TestSettingsDelegate = () => new TestSettingsResult { TestResult = ConfigurationTestResult.Negative };
            TestConfigurationUnitProcessor unitProcessorApply = setProcessor.CreateTestProcessor(configurationUnitApply);
            unitProcessorApply.TestSettingsDelegate = () => new TestSettingsResult { TestResult = ConfigurationTestResult.Negative };

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
            }
        }

        /// <summary>
        /// A unit in the correct state is not applied again.
        /// </summary>
        [Fact]
        public void ApplySet_UnitAlreadyInCorrectState()
        {
            ConfigurationSet configurationSet = new ConfigurationSet();
            ConfigurationUnit configurationUnit = new ConfigurationUnit { Intent = ConfigurationUnitIntent.Apply };
            configurationSet.ConfigurationUnits = new ConfigurationUnit[] { configurationUnit };

            TestConfigurationProcessorFactory factory = new TestConfigurationProcessorFactory();
            TestConfigurationSetProcessor setProcessor = factory.CreateTestProcessor(configurationSet);
            TestConfigurationUnitProcessor unitProcessor = setProcessor.CreateTestProcessor(configurationUnit);
            unitProcessor.TestSettingsDelegate = () => new TestSettingsResult { TestResult = ConfigurationTestResult.Positive };

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
        }

        /// <summary>
        /// Checks the progress reporting.
        /// </summary>
        [Fact]
        public void ApplySet_Progress()
        {
            ConfigurationSet configurationSet = new ConfigurationSet();
            ConfigurationUnit assert1 = new ConfigurationUnit() { Intent = ConfigurationUnitIntent.Assert, Identifier = "Assert1" };
            ConfigurationUnit assert2 = new ConfigurationUnit() { Intent = ConfigurationUnitIntent.Assert, Identifier = "Assert2", Dependencies = new string[] { assert1.Identifier } };
            ConfigurationUnit inform1 = new ConfigurationUnit() { Intent = ConfigurationUnitIntent.Inform, Identifier = "Inform1" };
            ConfigurationUnit apply1 = new ConfigurationUnit() { Intent = ConfigurationUnitIntent.Apply, Identifier = "Apply1" };
            ConfigurationUnit apply2 = new ConfigurationUnit() { Intent = ConfigurationUnitIntent.Apply, Identifier = "Apply2" };
            ConfigurationUnit apply3 = new ConfigurationUnit() { Intent = ConfigurationUnitIntent.Apply, Identifier = "Apply3", Dependencies = new string[] { apply1.Identifier, apply2.Identifier } };
            ConfigurationUnit apply4 = new ConfigurationUnit() { Intent = ConfigurationUnitIntent.Apply, Identifier = "Apply4", ShouldApply = false };
            ConfigurationUnit apply5 = new ConfigurationUnit() { Intent = ConfigurationUnitIntent.Apply, Identifier = "Apply5", Dependencies = new string[] { apply4.Identifier } };
            configurationSet.ConfigurationUnits = new ConfigurationUnit[] { assert2, assert1, inform1, apply1, apply3, apply4,  apply2, apply5 };

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
            Assert.Equal(configurationSet.ConfigurationUnits.Count, result.UnitResults.Count);

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
                        }
                        else
                        {
                            Assert.NotNull(progressEvents[i].ResultInformation.ResultCode);
                            Assert.Equal(expectedProgress[i].HResult, progressEvents[i].ResultInformation.ResultCode.HResult);
                        }

                        break;
                    default:
                        Assert.Fail("Unexpected ConfigurationSetChangeEventType value");
                        break;
                }
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
