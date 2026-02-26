// -----------------------------------------------------------------------------
// <copyright file="Errors.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Helpers
{
    /// <summary>
    /// Contains the error codes used by Microsoft.Management.Configuration.
    /// </summary>
    internal static class Errors
    {
#pragma warning disable SA1310 // Field names should not contain underscore
#pragma warning disable SA1600 // Elements should be documented
#pragma warning disable SA1025 // Code should not contain multiple whitespace in a row

        public static readonly int WINGET_CONFIG_ERROR_INVALID_CONFIGURATION_FILE           = unchecked((int)0x8A15C001);
        public static readonly int WINGET_CONFIG_ERROR_INVALID_YAML                         = unchecked((int)0x8A15C002);
        public static readonly int WINGET_CONFIG_ERROR_INVALID_FIELD_TYPE                   = unchecked((int)0x8A15C003);
        public static readonly int WINGET_CONFIG_ERROR_UNKNOWN_CONFIGURATION_FILE_VERSION   = unchecked((int)0x8A15C004);
        public static readonly int WINGET_CONFIG_ERROR_SET_APPLY_FAILED                     = unchecked((int)0x8A15C005);
        public static readonly int WINGET_CONFIG_ERROR_DUPLICATE_IDENTIFIER                 = unchecked((int)0x8A15C006);
        public static readonly int WINGET_CONFIG_ERROR_MISSING_DEPENDENCY                   = unchecked((int)0x8A15C007);
        public static readonly int WINGET_CONFIG_ERROR_DEPENDENCY_UNSATISFIED               = unchecked((int)0x8A15C008);
        public static readonly int WINGET_CONFIG_ERROR_ASSERTION_FAILED                     = unchecked((int)0x8A15C009);
        public static readonly int WINGET_CONFIG_ERROR_MANUALLY_SKIPPED                     = unchecked((int)0x8A15C00A);
        public static readonly int WINGET_CONFIG_ERROR_WARNING_NOT_ACCEPTED                 = unchecked((int)0x8A15C00B);
        public static readonly int WINGET_CONFIG_ERROR_SET_DEPENDENCY_CYCLE                 = unchecked((int)0x8A15C00C);
        public static readonly int WINGET_CONFIG_ERROR_INVALID_FIELD_VALUE                  = unchecked((int)0x8A15C00D);
        public static readonly int WINGET_CONFIG_ERROR_MISSING_FIELD                        = unchecked((int)0x8A15C00E);
        public static readonly int WINGET_CONFIG_ERROR_TEST_FAILED                          = unchecked((int)0x8A15C00F);
        public static readonly int WINGET_CONFIG_ERROR_TEST_NOT_RUN                         = unchecked((int)0x8A15C010);
        public static readonly int WINGET_CONFIG_ERROR_GET_FAILED                           = unchecked((int)0x8A15C011);

        // Configuration Processor Errors
        public static readonly int WINGET_CONFIG_ERROR_UNIT_NOT_INSTALLED                   = unchecked((int)0x8A15C101);
        public static readonly int WINGET_CONFIG_ERROR_UNIT_NOT_FOUND_REPOSITORY            = unchecked((int)0x8A15C102);
        public static readonly int WINGET_CONFIG_ERROR_UNIT_MULTIPLE_MATCHES                = unchecked((int)0x8A15C103);
        public static readonly int WINGET_CONFIG_ERROR_UNIT_INVOKE_GET                      = unchecked((int)0x8A15C104);
        public static readonly int WINGET_CONFIG_ERROR_UNIT_INVOKE_TEST                     = unchecked((int)0x8A15C105);
        public static readonly int WINGET_CONFIG_ERROR_UNIT_INVOKE_SET                      = unchecked((int)0x8A15C106);
        public static readonly int WINGET_CONFIG_ERROR_UNIT_MODULE_CONFLICT                 = unchecked((int)0x8A15C107);
        public static readonly int WINGET_CONFIG_ERROR_UNIT_IMPORT_MODULE                   = unchecked((int)0x8A15C108);
        public static readonly int WINGET_CONFIG_ERROR_UNIT_INVOKE_INVALID_RESULT           = unchecked((int)0x8A15C109);
        public static readonly int WINGET_CONFIG_ERROR_UNIT_SETTING_CONFIG_ROOT             = unchecked((int)0x8A15C110);
        public static readonly int WINGET_CONFIG_ERROR_UNIT_IMPORT_MODULE_ADMIN             = unchecked((int)0x8A15C111);
        public static readonly int WINGET_CONFIG_ERROR_NOT_SUPPORTED_BY_PROCESSOR           = unchecked((int)0x8A15C112);
        public static readonly int WINGET_CONFIG_ERROR_PARAMETER_INTEGRITY_BOUNDARY         = unchecked((int)0x8A15C013);

        // Limitation Set Errors
        public static readonly int CORE_INVALID_OPERATION = unchecked((int)0x80131509);

#pragma warning restore SA1025 // Code should not contain multiple whitespace in a row
#pragma warning restore SA1600 // Elements should be documented
#pragma warning restore SA1310 // Field names should not contain underscore
    }
}
