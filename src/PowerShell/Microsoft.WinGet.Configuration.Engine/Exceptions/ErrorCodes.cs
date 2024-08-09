// -----------------------------------------------------------------------------
// <copyright file="ErrorCodes.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Configuration.Engine.Exceptions
{
    /// <summary>
    /// This should match the ones in AppInstallerErrors.h.
    /// </summary>
    internal static class ErrorCodes
    {
#pragma warning disable SA1600 // ElementsMustBeDocumented
#pragma warning disable SA1310 // Field names should not contain underscore
        internal const int S_OK = 0;

        internal const int WingetConfigErrorInvalidConfigurationFile = unchecked((int)0x8A15C001);
        internal const int WingetConfigErrorInvalidYaml = unchecked((int)0x8A15C002);
        internal const int WingetConfigErrorInvalidFieldType = unchecked((int)0x8A15C003);
        internal const int WingetConfigErrorUnknownConfigurationFileVersion = unchecked((int)0x8A15C004);
        internal const int WingetConfigErrorSetApplyFailed = unchecked((int)0x8A15C005);
        internal const int WingetConfigErrorDuplicateIdentifier = unchecked((int)0x8A15C006);
        internal const int WingetConfigErrorMissingDependency = unchecked((int)0x8A15C007);
        internal const int WingetConfigErrorDependencyUnsatisfied = unchecked((int)0x8A15C008);
        internal const int WingetConfigErrorAssertionFailed = unchecked((int)0x8A15C009);
        internal const int WingetConfigErrorManuallySkipped = unchecked((int)0x8A15C00A);
        internal const int WingetConfigErrorWarningNotAccepted = unchecked((int)0x8A15C00B);
        internal const int WingetConfigErrorSetDependencyCycle = unchecked((int)0x8A15C00C);
        internal const int WingetConfigErrorInvalidFieldValue = unchecked((int)0x8A15C00D);
        internal const int WingetConfigErrorMissingField = unchecked((int)0x8A15C00E);

        internal const int WinGetConfigUnitNotFound = unchecked((int)0x8A15C101);
        internal const int WinGetConfigUnitNotFoundRepository = unchecked((int)0x8A15C102);
        internal const int WinGetConfigUnitMultipleMatches = unchecked((int)0x8A15C103);
        internal const int WinGetConfigUnitInvokeGet = unchecked((int)0x8A15C104);
        internal const int WinGetConfigUnitInvokeTest = unchecked((int)0x8A15C105);
        internal const int WinGetConfigUnitInvokeSet = unchecked((int)0x8A15C106);
        internal const int WinGetConfigUnitModuleConflict = unchecked((int)0x8A15C107);
        internal const int WinGetConfigUnitImportModule = unchecked((int)0x8A15C108);
        internal const int WinGetConfigUnitInvokeInvalidResult = unchecked((int)0x8A15C109);
        internal const int WinGetConfigUnitSettingConfigRoot = unchecked((int)0x8A15C110);
        internal const int WinGetConfigUnitImportModuleAdmin = unchecked((int)0x8A15C111);
#pragma warning restore SA1310 // Field names should not contain underscore
#pragma warning restore SA1600 // ElementsMustBeDocumented
    }
}
