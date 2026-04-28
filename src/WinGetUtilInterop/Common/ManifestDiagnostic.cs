// -----------------------------------------------------------------------------
// <copyright file="ManifestDiagnostic.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGetUtil.Common
{
    /// <summary>
    /// Severity level of a manifest diagnostic.
    /// </summary>
    public enum ManifestDiagnosticLevel
    {
        /// <summary>
        /// The diagnostic is a warning; the manifest may still be valid.
        /// </summary>
        Warning,

        /// <summary>
        /// The diagnostic is an error; the manifest is invalid.
        /// </summary>
        Error,
    }

    /// <summary>
    /// Represents a single manifest error or warning returned by <see cref="WinGetCreateManifestOption.ReturnResponseAsJSON"/>.
    /// </summary>
    public class ManifestDiagnostic
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="ManifestDiagnostic"/> class.
        /// </summary>
        /// <param name="errorId">The error identifier.</param>
        /// <param name="message">The human-readable error message.</param>
        /// <param name="context">The field or context associated with the error.</param>
        /// <param name="value">The field value that caused the error, if any.</param>
        /// <param name="line">The 1-based line number in the source file, or 0 if unknown.</param>
        /// <param name="column">The 1-based column number in the source file, or 0 if unknown.</param>
        /// <param name="level">The severity level of the diagnostic.</param>
        /// <param name="file">The source file name, if applicable.</param>
        public ManifestDiagnostic(
            ManifestErrorId errorId,
            string message,
            string context,
            string value,
            long line,
            long column,
            ManifestDiagnosticLevel level,
            string file)
        {
            this.ErrorId = errorId;
            this.Message = message;
            this.Context = context;
            this.Value = value;
            this.Line = line;
            this.Column = column;
            this.Level = level;
            this.File = file;
        }

        /// <summary>
        /// Gets the error identifier.
        /// </summary>
        public ManifestErrorId ErrorId { get; }

        /// <summary>
        /// Gets the human-readable error message.
        /// </summary>
        public string Message { get; }

        /// <summary>
        /// Gets the field or context associated with the error.
        /// </summary>
        public string Context { get; }

        /// <summary>
        /// Gets the field value that caused the error, if any.
        /// </summary>
        public string Value { get; }

        /// <summary>
        /// Gets the 1-based line number in the source file, or 0 if unknown.
        /// </summary>
        public long Line { get; }

        /// <summary>
        /// Gets the 1-based column number in the source file, or 0 if unknown.
        /// </summary>
        public long Column { get; }

        /// <summary>
        /// Gets the severity level of this diagnostic.
        /// </summary>
        public ManifestDiagnosticLevel Level { get; }

        /// <summary>
        /// Gets the source file name associated with this diagnostic, if applicable.
        /// </summary>
        public string File { get; }
    }
}
