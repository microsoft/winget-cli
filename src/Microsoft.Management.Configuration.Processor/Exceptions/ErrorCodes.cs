// -----------------------------------------------------------------------------
// <copyright file="ErrorCodes.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Exceptions
{
    /// <summary>
    /// This should match the ones in AppInstallerErrors.h.
    /// </summary>
    internal static class ErrorCodes
    {
        /// <summary>
        /// The module of the unit was installed, but the unit was not found.
        /// </summary>
        internal const int WinGetConfigUnitNotFound = unchecked((int)0x8A15C101);

        /// <summary>
        /// The unit couldn't be found in the repository.
        /// </summary>
        internal const int WinGetConfigUnitNotFoundRepository = unchecked((int)0x8A15C102);

        /// <summary>
        /// Multiple units found with the same criteria.
        /// </summary>
        internal const int WinGetConfigUnitMultipleMatches = unchecked((int)0x8A15C103);

        /// <summary>
        /// Internal error calling Invoke-DscResource Get.
        /// </summary>
        internal const int WinGetConfigUnitInvokeGet = unchecked((int)0x8A15C104);

        /// <summary>
        /// Internal error calling Invoke-DscResource Test.
        /// </summary>
        internal const int WinGetConfigUnitInvokeTest = unchecked((int)0x8A15C105);

        /// <summary>
        /// Internal error calling Invoke-DscResource Set.
        /// </summary>
        internal const int WinGetConfigUnitInvokeSet = unchecked((int)0x8A15C106);

        /// <summary>
        /// Internal error calling Get-DscResource. More than one module found with the same version.
        /// </summary>
        internal const int WinGetConfigUnitModuleConflict = unchecked((int)0x8A15C107);

        /// <summary>
        /// The module where the DSC resource is implemented cannot be imported.
        /// </summary>
        internal const int WinGetConfigUnitImportModule = unchecked((int)0x8A15C108);
    }
}
