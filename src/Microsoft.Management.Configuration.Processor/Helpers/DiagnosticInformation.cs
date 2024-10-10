// -----------------------------------------------------------------------------
// <copyright file="DiagnosticInformation.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Helpers
{
    using System;
    using Microsoft.Management.Configuration;

    /// <summary>
    /// Implements IDiagnosticInformation.
    /// </summary>
    internal sealed partial class DiagnosticInformation : IDiagnosticInformation
    {
        /// <inheritdoc/>
        public DiagnosticLevel Level { get; internal set; }

        /// <inheritdoc/>
        public string? Message { get; internal set; }
    }
}
