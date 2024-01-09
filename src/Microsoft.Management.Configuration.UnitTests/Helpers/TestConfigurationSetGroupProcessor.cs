// -----------------------------------------------------------------------------
// <copyright file="TestConfigurationSetGroupProcessor.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Helpers
{
    using System;
    using System.Collections.Generic;
    using System.Runtime.InteropServices.WindowsRuntime;
    using System.Threading;
    using System.Threading.Tasks;
    using Windows.Foundation;
    using Windows.Foundation.Collections;

    /// <summary>
    /// A test implementation of IConfigurationGroupProcessor.
    /// </summary>
    internal class TestConfigurationSetGroupProcessor : TestConfigurationSetProcessor, IConfigurationGroupProcessor
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
        /// Initializes a new instance of the <see cref="TestConfigurationSetGroupProcessor"/> class.
        /// </summary>
        /// <param name="set">The set that this processor is for.</param>
        internal TestConfigurationSetGroupProcessor(ConfigurationSet? set)
            : base(set)
        {
        }

        /// <summary>
        /// Gets the group that this processor targets.
        /// </summary>
        public object? Group
        {
            get { return this.Set; }
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

                if (this.Set != null)
                {
                    foreach (ConfigurationUnit unit in this.Set.Units)
                    {
                        ApplyGroupMemberSettingsResultInstance unitResult = new (unit);

                        unitResult.State = ConfigurationUnitState.InProgress;
                        progress.Report(unitResult);

                        unitResult.PreviouslyInDesiredState = this.GetTestResult(unit) == ConfigurationTestResult.Positive;

                        unitResult.State = ConfigurationUnitState.Completed;
                        progress.Report(unitResult);

                        result.UnitResults.Add(unitResult);
                    }
                }

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

                if (this.Set != null)
                {
                    result.TestResult = this.GetTestResult(this.Set.Metadata);

                    foreach (ConfigurationUnit unit in this.Set.Units)
                    {
                        TestSettingsResultInstance unitResult = new (unit);

                        unitResult.TestResult = this.GetTestResult(unit);
                        progress.Report(unitResult);

                        result.UnitResults.Add(unitResult);
                    }
                }

                return result;
            }));
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
                this.asyncWaitEvent.WaitOne();
            }
        }

        private ConfigurationTestResult GetTestResult(ConfigurationUnit unit)
        {
            return this.GetTestResult(unit.Settings);
        }

        private ConfigurationTestResult GetTestResult(ValueSet values)
        {
            if (values.ContainsKey(TestResultSetting))
            {
                string? valueString = values[TestResultSetting]?.ToString();
                if (valueString != null)
                {
                    return Enum.Parse<ConfigurationTestResult>(valueString);
                }
            }

            return ConfigurationTestResult.Negative;
        }
    }
}
