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
        protected ConfigurationProcessor CreateConfigurationProcessorWithDiagnostics(IConfigurationProcessorFactory? factory = null)
        {
            ConfigurationProcessor result = new ConfigurationProcessor(factory);
            result.Diagnostics += this.diagnosticsEventSink.DiagnosticsHandler;
            return result;
        }

        /// <summary>
        /// Creates an input stream from the given string.
        /// </summary>
        /// <param name="contents">The contents that the stream should contain.</param>
        /// <returns>The created stream.</returns>
        protected IInputStream CreateStream(string contents)
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
    }
}
