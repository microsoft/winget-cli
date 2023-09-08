// -----------------------------------------------------------------------------
// <copyright file="PSGetConfigurationDetailsResult.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Configuration.Engine.PSObjects
{
    using System;
    using Microsoft.Management.Configuration;
    using Microsoft.WinGet.Configuration.Engine.Exceptions;

    /// <summary>
    /// Result for getting the details of a unit.
    /// </summary>
    public class PSGetConfigurationDetailsResult
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="PSGetConfigurationDetailsResult"/> class.
        /// </summary>
        /// <param name="result">Get unit details result.</param>
        internal PSGetConfigurationDetailsResult(GetConfigurationUnitDetailsResult result)
        {
            this.Type = result.Unit.Type;
            this.ResultCode = result.ResultInformation?.ResultCode?.HResult ?? ErrorCodes.S_OK;

            if (result.ResultInformation?.ResultCode != null)
            {
                this.ErrorMessage = $"Failed to get unit details for {this.Type} 0x{this.ResultCode:X}" +
                    $"{Environment.NewLine}Description: '{result.ResultInformation.Description}'{Environment.NewLine}Details: '{result.ResultInformation.Details}'";
            }
        }

        /// <summary>
        /// Gets the unit type.
        /// </summary>
        public string Type { get; private init; }

        /// <summary>
        /// Gets the result code.
        /// </summary>
        public int ResultCode { get; private init; }

        /// <summary>
        /// Gets the error message.
        /// </summary>
        public string? ErrorMessage { get; private init; }
    }
}
