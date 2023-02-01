// -----------------------------------------------------------------------------
// <copyright file="OpenConfigurationSetTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Helpers
{
    using Microsoft.Management.Configuration.UnitTests.Fixtures;
    using Xunit.Abstractions;

    /// <summary>
    /// Test base that provides helpers for dealing with <see cref="ConfigurationProcessor"/>.
    /// </summary>
    public class ConfigurationProcessorTestBase
    {
        private readonly DiagnosticsEventSink diagnosticsEventSink;

        /// <summary>
        /// Initializes a new instance of the <see cref="ConfigurationProcessorTestBase"/> class.
        /// </summary>
        /// <param name="fixture">Unit test fixture.</param>
        /// <param name="log">Log helper.</param>
        protected ConfigurationProcessorTestBase(UnitTestFixture fixture, ITestOutputHelper log)
        {
            this.Fixture = fixture;
            this.Log = log;
            this.diagnosticsEventSink = new DiagnosticsEventSink(fixture, log);
        }

        /// <summary>
        /// Gets the test fixture.
        /// </summary>
        protected UnitTestFixture Fixture { get; private set; }

        /// <summary>
        /// Gets the output helper.
        /// </summary>
        protected ITestOutputHelper Log { get; private set; }

        /// <summary>
        /// Createa a new <see cref="ConfigurationProcessor"/> with the diagnostics event hooked up.
        /// </summary>
        /// <param name="factory">The factory to use.</param>
        /// <returns>The new <see cref="ConfigurationProcessor"/> object.</returns>
        protected ConfigurationProcessor CreateConfigurationProcessorWithDiagnostics(IConfigurationProcessorFactory? factory = null)
        {
            ConfigurationProcessor result = new ConfigurationProcessor(factory);
            result.Diagnostics += this.diagnosticsEventSink.DiagnosticsHandler;
            return result;
        }
    }
}
