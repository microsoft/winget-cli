// -----------------------------------------------------------------------------
// <copyright file="OpenConfigurationSetException.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Configuration.Engine.Exceptions
{
    using System;
    using System.Text;
    using Microsoft.Management.Configuration;
    using Microsoft.WinGet.Resources;

    /// <summary>
    /// Exception thrown when failed to open a configuration set.
    /// </summary>
    public class OpenConfigurationSetException : Exception
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="OpenConfigurationSetException"/> class.
        /// </summary>
        /// <param name="openResult">Open Result.</param>
        /// <param name="configurationFile">Configuration file.</param>
        internal OpenConfigurationSetException(OpenConfigurationSetResult openResult, string configurationFile)
            : base(GetMessage(openResult, configurationFile))
        {
        }

        private static string GetMessage(OpenConfigurationSetResult openResult, string configurationFile)
        {
            var sb = new StringBuilder();
            sb.Append($"Failed to open configuration set at {configurationFile} with error 0x{openResult.ResultCode.HResult:X} ");

            switch (openResult.ResultCode.HResult)
            {
                case ErrorCodes.WingetConfigErrorInvalidFieldType:
                    sb.Append(string.Format(Resources.ConfigurationFieldInvalidType, openResult.Field));
                    break;
                case ErrorCodes.WingetConfigErrorInvalidFieldValue:
                    sb.Append(string.Format(Resources.ConfigurationFieldInvalidValue, openResult.Field, openResult.Value));
                    break;
                case ErrorCodes.WingetConfigErrorMissingField:
                    sb.Append(string.Format(Resources.ConfigurationFieldMissing, openResult.Field));
                    break;
                case ErrorCodes.WingetConfigErrorUnknownConfigurationFileVersion:
                    sb.Append(string.Format(Resources.ConfigurationFileVersionUnknown, openResult.Value));
                    break;
                case ErrorCodes.WingetConfigErrorInvalidConfigurationFile:
                case ErrorCodes.WingetConfigErrorInvalidYaml:
                default:
                    sb.Append(Resources.ConfigurationFileInvalid);
                    break;
            }

            if (openResult.Line != 0)
            {
                sb.Append($" {string.Format(Resources.SeeLineAndColumn, openResult.Line, openResult.Column)}");
            }

            return sb.ToString();
        }
    }
}
