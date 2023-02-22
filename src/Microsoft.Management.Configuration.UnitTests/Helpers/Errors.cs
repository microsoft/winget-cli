// -----------------------------------------------------------------------------
// <copyright file="Errors.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Helpers
{
    /// <summary>
    /// Contains the error codes used by Microsoft.Management.Configuration
    /// </summary>
    internal static class Errors
    {
#pragma warning disable SA1310 // Field names should not contain underscore
#pragma warning disable SA1600 // Elements should be documented
#pragma warning disable SA1025 // Code should not contain multiple whitespace in a row

        public static readonly int WINGET_CONFIG_ERROR_INVALID_CONFIGURATION_FILE          = unchecked((int)0x8A15C001);
        public static readonly int WINGET_CONFIG_ERROR_INVALID_YAML                        = unchecked((int)0x8A15C002);
        public static readonly int WINGET_CONFIG_ERROR_INVALID_FIELD                       = unchecked((int)0x8A15C003);
        public static readonly int WINGET_CONFIG_ERROR_UNKNOWN_CONFIGURATION_FILE_VERSION  = unchecked((int)0x8A15C004);
        public static readonly int WINGET_CONFIG_ERROR_SET_APPLY_FAILED                    = unchecked((int)0x8A15C005);
        public static readonly int WINGET_CONFIG_ERROR_DUPLICATE_IDENTIFIER                = unchecked((int)0x8A15C006);
        public static readonly int WINGET_CONFIG_ERROR_MISSING_DEPENDENCY                  = unchecked((int)0x8A15C007);
        public static readonly int WINGET_CONFIG_ERROR_DEPENDENCY_UNSATISFIED              = unchecked((int)0x8A15C008);
        public static readonly int WINGET_CONFIG_ERROR_ASSERTION_FAILED                    = unchecked((int)0x8A15C009);
        public static readonly int WINGET_CONFIG_ERROR_MANUALLY_SKIPPED                    = unchecked((int)0x8A15C00A);

#pragma warning restore SA1025 // Code should not contain multiple whitespace in a row
#pragma warning restore SA1600 // Elements should be documented
#pragma warning restore SA1310 // Field names should not contain underscore
    }
}
