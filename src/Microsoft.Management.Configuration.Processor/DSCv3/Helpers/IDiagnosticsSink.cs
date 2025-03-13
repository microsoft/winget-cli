// -----------------------------------------------------------------------------
// <copyright file="IDiagnosticsSink.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.DSCv3.Helpers
{
    /// <summary>
    /// Defines the interface for a diagnostics sink.
    /// </summary>
    internal interface IDiagnosticsSink
    {
        /// <summary>
        /// Sends a diagnostic message.
        /// </summary>
        /// <param name="level">The level of the message.</param>
        /// <param name="message">The message.</param>
        public void OnDiagnostics(DiagnosticLevel level, string message);
    }
}
