// -----------------------------------------------------------------------------
// <copyright file="DiagnosticsEventSink.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Helpers
{
    using System.Collections.Generic;
    using Microsoft.Management.Configuration.UnitTests.Fixtures;
    using Xunit.Abstractions;
    using Xunit.Sdk;

    /// <summary>
    /// This class aids in getting diagnostics data from the <see cref="ConfigurationProcessor"/> out to the xUnit infrastructure.
    /// </summary>
    public class DiagnosticsEventSink
    {
        private readonly UnitTestFixture fixture;
        private readonly ITestOutputHelper log;

        /// <summary>
        /// Initializes a new instance of the <see cref="DiagnosticsEventSink"/> class.
        /// </summary>
        /// <param name="fixture">Unit test fixture.</param>
        /// <param name="log">Log helper.</param>
        public DiagnosticsEventSink(UnitTestFixture fixture, ITestOutputHelper log)
        {
            this.fixture = fixture;
            this.log = log;
        }

        /// <summary>
        /// Gets the telemetry events that have been seen.
        /// </summary>
        public List<TelemetryEvent> Events { get; private set; } = new List<TelemetryEvent>();

        /// <summary>
        /// Handles diagnostic information from a <see cref="ConfigurationProcessor"/>.
        /// </summary>
        /// <param name="sender">The object sending the information.</param>
        /// <param name="e">The diagnostic information.</param>
        internal void DiagnosticsHandler(object? sender, IDiagnosticInformation e)
        {
            if (e.Message.Contains(TelemetryEvent.Preamble))
            {
                this.Events.Add(new TelemetryEvent(e.Message));
            }

            if (e.Level == DiagnosticLevel.Verbose)
            {
                this.fixture.MessageSink.OnMessage(new DiagnosticMessage(e.Message));
            }
            else
            {
                this.log.WriteLine(e.Message);
            }
        }
    }
}
