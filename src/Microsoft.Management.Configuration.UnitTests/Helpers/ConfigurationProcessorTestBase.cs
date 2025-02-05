// -----------------------------------------------------------------------------
// <copyright file="ConfigurationProcessorTestBase.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Helpers
{
    using System;
    using Microsoft.Management.Configuration.UnitTests.Fixtures;
    using Windows.Storage.Streams;
    using Xunit;
    using Xunit.Abstractions;

    /// <summary>
    /// Test base that provides helpers for dealing with <see cref="ConfigurationProcessor"/>.
    /// </summary>
    public class ConfigurationProcessorTestBase
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="ConfigurationProcessorTestBase"/> class.
        /// </summary>
        /// <param name="fixture">Unit test fixture.</param>
        /// <param name="log">Log helper.</param>
        protected ConfigurationProcessorTestBase(UnitTestFixture fixture, ITestOutputHelper log)
        {
            this.Fixture = fixture;
            this.Log = log;
            this.EventSink = new DiagnosticsEventSink(fixture, log);
        }

        /// <summary>
        /// Gets the event sink for this test base.
        /// </summary>
        protected DiagnosticsEventSink EventSink { get; private set; }

        /// <summary>
        /// Gets the test fixture.
        /// </summary>
        protected UnitTestFixture Fixture { get; private init; }

        /// <summary>
        /// Gets the output helper.
        /// </summary>
        protected ITestOutputHelper Log { get; private init; }

        /// <summary>
        /// Create a new <see cref="ConfigurationProcessor"/> with the diagnostics event hooked up.
        /// </summary>
        /// <param name="factory">The factory to use.</param>
        /// <returns>The new <see cref="ConfigurationProcessor"/> object.</returns>
        internal ConfigurationProcessor CreateConfigurationProcessorWithDiagnostics(IConfigurationSetProcessorFactory? factory = null)
        {
            ConfigurationProcessor result = this.Fixture.ConfigurationStatics.CreateConfigurationProcessor(factory);
            result.Diagnostics += this.EventSink.DiagnosticsHandler;
            result.MinimumLevel = DiagnosticLevel.Verbose;
            return result;
        }

        /// <summary>
        /// Creates an input stream from the given string.
        /// </summary>
        /// <param name="contents">The contents that the stream should contain.</param>
        /// <returns>The created stream.</returns>
        internal IInputStream CreateStream(string contents)
        {
            InMemoryRandomAccessStream result = new InMemoryRandomAccessStream();

            using (DataWriter writer = new DataWriter(result))
            {
                writer.UnicodeEncoding = UnicodeEncoding.Utf8;
                writer.WriteString(contents);
                writer.StoreAsync().AsTask().Wait();
                writer.DetachStream();
            }

            result.Seek(0);
            return result;
        }

        /// <summary>
        /// Creates an string from the given output stream.
        /// </summary>
        /// <param name="stream">The output stream.</param>
        /// <returns>The created string.</returns>
        internal string ReadStream(InMemoryRandomAccessStream stream)
        {
            string result = string.Empty;
            using (DataReader reader = new DataReader(stream.GetInputStreamAt(0)))
            {
                reader.UnicodeEncoding = UnicodeEncoding.Utf8;
                reader.LoadAsync((uint)stream.Size).AsTask().Wait();
                uint bytesToRead = reader.UnconsumedBufferLength;

                if (bytesToRead > 0)
                {
                    result = reader.ReadString(bytesToRead);
                }
            }

            return result;
        }

        /// <summary>
        /// Creates a configuration unit via the configuration statics object.
        /// </summary>
        /// <returns>A new configuration unit.</returns>
        internal ConfigurationUnit ConfigurationUnit()
        {
            return this.Fixture.ConfigurationStatics.CreateConfigurationUnit();
        }

        /// <summary>
        /// Creates a configuration parameter via the configuration statics object.
        /// </summary>
        /// <returns>A new configuration parameter.</returns>
        internal ConfigurationParameter ConfigurationParameter()
        {
            return this.Fixture.ConfigurationStatics.CreateConfigurationParameter();
        }

        /// <summary>
        /// Creates a configuration set via the configuration statics object.
        /// </summary>
        /// <returns>A new configuration set.</returns>
        internal ConfigurationSet ConfigurationSet()
        {
            return this.Fixture.ConfigurationStatics.CreateConfigurationSet();
        }

        /// <summary>
        /// Verifies the summary event generated by a processing run.
        /// </summary>
        /// <param name="configurationSet">The configuration set.</param>
        /// <param name="setResult">The set result.</param>
        /// <param name="resultSource">The result source.</param>
        internal void VerifySummaryEvent(ConfigurationSet configurationSet, ApplyConfigurationSetResult setResult, ConfigurationUnitResultSource resultSource)
        {
            TelemetryEvent summary = this.VerifySummaryEventShared(configurationSet, ConfigurationUnitIntent.Apply, resultSource == ConfigurationUnitResultSource.None ? 0 : setResult.ResultCode.HResult, resultSource);

            int[] counts = new int[3];
            int[] runs = new int[3];
            int[] failures = new int[3];

            var unitResults = setResult.UnitResults;
            for (int i = 0; i < unitResults.Count; ++i)
            {
                ApplyConfigurationUnitResult unitResult = unitResults[i];
                SummaryCountByIntent(counts, runs, failures, unitResult.Unit.Intent, unitResult.ResultInformation);
            }

            VerifySummaryCounts(summary, counts, runs, failures);
        }

        /// <summary>
        /// Verifies the summary event generated by a processing run.
        /// </summary>
        /// <param name="configurationSet">The configuration set.</param>
        /// <param name="setResult">The set result.</param>
        /// <param name="resultCode">The result code.</param>
        /// <param name="resultSource">The result source.</param>
        internal void VerifySummaryEvent(ConfigurationSet configurationSet, TestConfigurationSetResult setResult, int resultCode, ConfigurationUnitResultSource resultSource)
        {
            TelemetryEvent summary = this.VerifySummaryEventShared(configurationSet, ConfigurationUnitIntent.Assert, resultCode, resultSource);

            int[] counts = new int[3];
            int[] runs = new int[3];
            int[] failures = new int[3];

            foreach (TestConfigurationUnitResult unitResult in setResult.UnitResults)
            {
                SummaryCountByIntent(counts, runs, failures, unitResult.Unit.Intent, unitResult.ResultInformation);
            }

            VerifySummaryCounts(summary, counts, runs, failures);
        }

        private static void SummaryCountByIntent(int[] counts, int[] runs, int[] failures, ConfigurationUnitIntent intent, IConfigurationUnitResultInformation resultInformation)
        {
            if (intent == ConfigurationUnitIntent.Unknown)
            {
                intent = ConfigurationUnitIntent.Apply;
            }

            int index = (int)intent;

            counts[index]++;

            if (resultInformation.ResultSource != ConfigurationUnitResultSource.ConfigurationSet && resultInformation.ResultSource != ConfigurationUnitResultSource.Precondition)
            {
                runs[index]++;
            }

            if (resultInformation.ResultCode != null)
            {
                failures[index]++;
            }
        }

        private static void VerifySummaryCounts(TelemetryEvent summary, int[] counts, int[] runs, int[] failures)
        {
            Assert.Equal(counts[(int)ConfigurationUnitIntent.Assert].ToString(), summary.Properties[TelemetryEvent.AssertCount]);
            Assert.Equal(runs[(int)ConfigurationUnitIntent.Assert].ToString(), summary.Properties[TelemetryEvent.AssertsRun]);
            Assert.Equal(failures[(int)ConfigurationUnitIntent.Assert].ToString(), summary.Properties[TelemetryEvent.AssertsFailed]);

            Assert.Equal(counts[(int)ConfigurationUnitIntent.Inform].ToString(), summary.Properties[TelemetryEvent.InformCount]);
            Assert.Equal(runs[(int)ConfigurationUnitIntent.Inform].ToString(), summary.Properties[TelemetryEvent.InformsRun]);
            Assert.Equal(failures[(int)ConfigurationUnitIntent.Inform].ToString(), summary.Properties[TelemetryEvent.InformsFailed]);

            Assert.Equal(counts[(int)ConfigurationUnitIntent.Apply].ToString(), summary.Properties[TelemetryEvent.ApplyCount]);
            Assert.Equal(runs[(int)ConfigurationUnitIntent.Apply].ToString(), summary.Properties[TelemetryEvent.AppliesRun]);
            Assert.Equal(failures[(int)ConfigurationUnitIntent.Apply].ToString(), summary.Properties[TelemetryEvent.AppliesFailed]);
        }

        /// <summary>
        /// Verifies the summary event generated by a processing run.
        /// </summary>
        /// <param name="configurationSet">The configuration set.</param>
        /// <param name="runIntent">The run intent.</param>
        /// <param name="resultCode">The result code.</param>
        /// <param name="resultSource">The result source.</param>
        private TelemetryEvent VerifySummaryEventShared(ConfigurationSet configurationSet, ConfigurationUnitIntent runIntent, int resultCode, ConfigurationUnitResultSource resultSource)
        {
            Assert.Single(this.EventSink.Events);
            TelemetryEvent summary = this.EventSink.Events[0];

            Assert.Equal(TelemetryEvent.ConfigProcessingSummaryName, summary.Name);
            Assert.NotEqual(string.Empty, summary.CodeVersion);
            Assert.NotEqual(Guid.Empty, summary.ActivityID);
            Assert.Equal(string.Empty, summary.Caller);
            Assert.Equal(configurationSet.InstanceIdentifier, Guid.Parse(summary.Properties[TelemetryEvent.SetID]));
            Assert.False(int.Parse(summary.Properties[TelemetryEvent.FromHistory]) != 0);
            Assert.Equal(((int)runIntent).ToString(), summary.Properties[TelemetryEvent.RunIntent]);
            Assert.Equal(resultCode.ToString(), summary.Properties[TelemetryEvent.Result]);
            Assert.Equal(((int)resultSource).ToString(), summary.Properties[TelemetryEvent.FailurePoint]);

            return summary;
        }
    }
}
