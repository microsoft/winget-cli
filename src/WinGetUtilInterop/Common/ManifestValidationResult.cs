// -----------------------------------------------------------------------------
// <copyright file="ManifestValidationResult.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGetUtil.Common
{
    /// <summary>
    /// Manifest validation result.
    /// </summary>
    public class ManifestValidationResult
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="ManifestValidationResult"/> class.
        /// </summary>
        /// <param name="isValid">A value indicating whether manifest is valid.</param>
        /// <param name="message">Error message.</param>
        /// <param name="resultCode">Result code.</param>
        public ManifestValidationResult(bool isValid, string message, WinGetValidateManifestResult resultCode)
        {
            this.IsValid = isValid;
            this.Message = message;
            this.ResultCode = resultCode;
        }

        /// <summary>
        /// Gets a value indicating whether manifest is valid.
        /// </summary>
        public bool IsValid { get; private set; }

        /// <summary>
        /// Gets the message associated with the validation.
        /// </summary>
        public string Message { get; private set; }

        /// <summary>
        /// Gets the result code associate with the validation.
        /// </summary>
        public WinGetValidateManifestResult ResultCode { get; private set; }
    }
}
