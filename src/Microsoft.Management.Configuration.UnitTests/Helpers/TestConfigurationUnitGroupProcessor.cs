// -----------------------------------------------------------------------------
// <copyright file="TestConfigurationUnitGroupProcessor.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Helpers
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Runtime.InteropServices.WindowsRuntime;
    using System.Threading;
    using System.Threading.Tasks;
    using Windows.Foundation;
    using Windows.Foundation.Collections;

    /// <summary>
    /// A test implementation of IConfigurationGroupProcessor.
    /// </summary>
    internal class TestConfigurationUnitGroupProcessor : TestConfigurationUnitProcessor, IConfigurationGroupProcessor
    {
        /// <summary>
        /// The Setting key that will be used to set the TestResult of the unit.
        /// </summary>
        internal const string TestResultSetting = "TestResult";

        /// <summary>
        /// The event that is waited on before actually processing the async operations.
        /// </summary>
        private AutoResetEvent asyncWaitEvent = new AutoResetEvent(false);

        /// <summary>
        /// Initializes a new instance of the <see cref="TestConfigurationUnitGroupProcessor"/> class.
        /// </summary>
        /// <param name="unit">The unit that this processor is for.</param>
        internal TestConfigurationUnitGroupProcessor(ConfigurationUnit unit)
            : base(unit)
        {
        }

        /// <summary>
        /// Gets the group that this processor targets.
        /// </summary>
        public object Group
        {
            get { return this.Unit; }
        }

        /// <summary>
        /// Gets or sets a value indicating whether the async methods should wait on an event before processing.
        /// </summary>
        internal bool ShouldWaitOnAsyncEvent { get; set; } = false;

        /// <summary>
        /// Apply settings for the group.
        /// </summary>
        /// <returns>The operation to apply settings.</returns>
        public IAsyncOperationWithProgress<IApplyGroupSettingsResult, IApplyGroupMemberSettingsResult> ApplyGroupSettingsAsync()
        {
            return AsyncInfo.Run((CancellationToken cancellationToken, IProgress<IApplyGroupMemberSettingsResult> progress) => Task.Run<IApplyGroupSettingsResult>(() =>
            {
                this.WaitOnAsyncEvent(cancellationToken);

                ApplyGroupSettingsResultInstance result = new (this.Group);
                result.UnitResults = new List<IApplyGroupMemberSettingsResult>();

                ApplyGroupSettings(this.Unit.Units, progress, result);

                return result;
            }));
        }

        /// <summary>
        /// Test settings for the group.
        /// </summary>
        /// <returns>The operation to test settings.</returns>
        public IAsyncOperationWithProgress<ITestGroupSettingsResult, ITestSettingsResult> TestGroupSettingsAsync()
        {
            return AsyncInfo.Run((CancellationToken cancellationToken, IProgress<ITestSettingsResult> progress) => Task.Run<ITestGroupSettingsResult>(() =>
            {
                this.WaitOnAsyncEvent(cancellationToken);

                TestGroupSettingsResultInstance result = new (this.Group);
                result.UnitResults = new List<ITestSettingsResult>();

                result.TestResult = GetTestResult(this.Unit.Metadata);
                TestGroupSettings(this.Unit.Units, progress, result);

                return result;
            }));
        }

        /// <summary>
        /// Gets the tests result for the given unit.
        /// </summary>
        /// <param name="unit">The unit.</param>
        /// <returns>The test result for the unit.</returns>
        internal static ConfigurationTestResult GetTestResult(ConfigurationUnit unit)
        {
            return GetTestResult(unit.Settings);
        }

        /// <summary>
        /// Gets the tests result for the given values.
        /// </summary>
        /// <param name="values">The values.</param>
        /// <returns>The test result for the values.</returns>
        internal static ConfigurationTestResult GetTestResult(ValueSet values)
        {
            if (values.ContainsKey(TestResultSetting))
            {
                string? valueString = values[TestResultSetting]?.ToString();
                if (valueString != null)
                {
                    return Enum.Parse<ConfigurationTestResult>(valueString);
                }
            }

            return ConfigurationTestResult.Positive;
        }

        /// <summary>
        /// Applies group settings for the given group members.
        /// </summary>
        /// <param name="groupMembers">The group members.</param>
        /// <param name="progress">The progress reporting object.</param>
        /// <param name="result">The result object.</param>
        internal static void ApplyGroupSettings(IList<ConfigurationUnit>? groupMembers, IProgress<IApplyGroupMemberSettingsResult> progress, ApplyGroupSettingsResultInstance result)
        {
            if (groupMembers != null)
            {
                foreach (ConfigurationUnit unit in groupMembers)
                {
                    ApplyGroupMemberSettingsResultInstance unitResult = new (unit);

                    unitResult.State = ConfigurationUnitState.InProgress;
                    progress.Report(unitResult);

                    unitResult.PreviouslyInDesiredState = GetTestResult(unit) == ConfigurationTestResult.Positive;

                    if (unit.IsGroup)
                    {
                        ApplyGroupSettings(unit.Units, progress, result);
                    }

                    unitResult.State = ConfigurationUnitState.Completed;
                    progress.Report(unitResult);

                    result.UnitResults!.Add(unitResult);
                }
            }
        }

        /// <summary>
        /// Tests group settings for the given group members.
        /// </summary>
        /// <param name="groupMembers">The group members.</param>
        /// <param name="progress">The progress reporting object.</param>
        /// <param name="result">The result object.</param>
        internal static void TestGroupSettings(IList<ConfigurationUnit>? groupMembers, IProgress<ITestSettingsResult> progress, TestGroupSettingsResultInstance result)
        {
            if (groupMembers != null)
            {
                foreach (ConfigurationUnit unit in groupMembers)
                {
                    TestSettingsResultInstance unitResult = new (unit);

                    if (unit.IsGroup)
                    {
                        TestGroupSettings(unit.Units, progress, result);
                    }

                    unitResult.TestResult = GetTestResult(unit);
                    progress.Report(unitResult);

                    result.UnitResults!.Add(unitResult);
                }
            }
        }

        /// <summary>
        /// Signals the async event.
        /// </summary>
        internal void SignalAsyncEvent()
        {
            this.asyncWaitEvent.Set();
        }

        /// <summary>
        /// Waits on the async event.
        /// </summary>
        private void WaitOnAsyncEvent(CancellationToken cancellationToken)
        {
            if (this.ShouldWaitOnAsyncEvent)
            {
                cancellationToken.Register(() => this.asyncWaitEvent.Set());
                if (!this.asyncWaitEvent.WaitOne(10000))
                {
                    throw new TimeoutException();
                }
            }
        }
    }
}
