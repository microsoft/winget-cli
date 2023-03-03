// -----------------------------------------------------------------------------
// <copyright file="DiagnosticsEventSink.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Helpers
{
    using Microsoft.Management.Configuration.UnitTests.Fixtures;
    using Xunit.Abstractions;
    using Xunit.Sdk;

    /// <summary>
    /// This class aids in getting diagnostics data from the <see cref="ConfigurationProcessor"/> out to the xUnit infrastructure.
    /// </summary>
    internal class DiagnosticsEventSink
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
        /// Handles diagnostic information from a <see cref="ConfigurationProcessor"/>.
        /// </summary>
        /// <param name="sender">The object sending the information.</param>
        /// <param name="e">The diagnostic information.</param>
        public void DiagnosticsHandler(object? sender, DiagnosticInformation e)
        {
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
