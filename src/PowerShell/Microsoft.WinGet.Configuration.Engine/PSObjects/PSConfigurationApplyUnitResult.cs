// -----------------------------------------------------------------------------
// <copyright file="PSConfigurationApplyUnitResult.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Configuration.Engine.PSObjects
{
    using Microsoft.Management.Configuration;
    using Microsoft.WinGet.Configuration.Engine.Exceptions;
    using Microsoft.WinGet.Configuration.Engine.Resources;

    /// <summary>
    /// The apply result of a configuration unit.
    /// </summary>
    public class PSConfigurationApplyUnitResult
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="PSConfigurationApplyUnitResult"/> class.
        /// </summary>
        /// <param name="unitResult">Apply unit result.</param>
        internal PSConfigurationApplyUnitResult(ApplyConfigurationUnitResult unitResult)
        {
            this.Type = unitResult.Unit.Type;
            this.ResultCode = unitResult.ResultInformation?.ResultCode?.HResult ?? 0;

            if (unitResult.ResultInformation != null)
            {
                string description = unitResult.ResultInformation.Description.Trim();
                var message = this.GetUnitMessage(unitResult);
                this.ErrorMessage = $"Configuration unit {this.Type}[{unitResult.Unit.Identifier}] failed with code 0x{this.ResultCode:X}" +
                    $" and error message:\n{description}\n{unitResult.ResultInformation.Details}\n{message}";
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
        /// Gets the failure message.
        /// </summary>
        public string? ErrorMessage { get; private init; }

        private string GetUnitMessage(ApplyConfigurationUnitResult unitResult)
        {
            if (unitResult.ResultInformation.ResultCode == null)
            {
                if (unitResult.Unit.State == ConfigurationUnitState.Skipped)
                {
                    return string.Format(Resources.ConfigurationUnitSkipped, "null");
                }

                return string.Format(Resources.ConfigurationUnitFailed, "null");
            }

            int resultCode = unitResult.ResultInformation.ResultCode.HResult;
            switch (resultCode)
            {
                case ErrorCodes.WingetConfigErrorDuplicateIdentifier:
                    return string.Format(Resources.ConfigurationUnitHasDuplicateIdentifier, unitResult.Unit.Identifier);
                case ErrorCodes.WingetConfigErrorMissingDependency:
                    return string.Format(Resources.ConfigurationUnitHasMissingDependency, unitResult.ResultInformation.Details);
                case ErrorCodes.WingetConfigErrorAssertionFailed:
                    return Resources.ConfigurationUnitAssertHadNegativeResult;
                case ErrorCodes.WinGetConfigUnitNotFound:
                    return Resources.ConfigurationUnitNotFoundInModule;
                case ErrorCodes.WinGetConfigUnitNotFoundRepository:
                    return Resources.ConfigurationUnitNotFound;
                case ErrorCodes.WinGetConfigUnitMultipleMatches:
                    return Resources.ConfigurationUnitMultipleMatches;
                case ErrorCodes.WinGetConfigUnitInvokeGet:
                    return Resources.ConfigurationUnitFailedDuringGet;
                case ErrorCodes.WinGetConfigUnitInvokeTest:
                    return Resources.ConfigurationUnitFailedDuringTest;
                case ErrorCodes.WinGetConfigUnitInvokeSet:
                    return Resources.ConfigurationUnitFailedDuringSet;
                case ErrorCodes.WinGetConfigUnitModuleConflict:
                    return Resources.ConfigurationUnitModuleConflict;
                case ErrorCodes.WinGetConfigUnitImportModule:
                    return Resources.ConfigurationUnitModuleImportFailed;
                case ErrorCodes.WinGetConfigUnitInvokeInvalidResult:
                    return Resources.ConfigurationUnitReturnedInvalidResult;
                case ErrorCodes.WingetConfigErrorManuallySkipped:
                    return Resources.ConfigurationUnitManuallySkipped;
                case ErrorCodes.WingetConfigErrorDependencyUnsatisfied:
                    return Resources.ConfigurationUnitNotRunDueToDependency;
            }

            switch (unitResult.ResultInformation.ResultSource)
            {
                case ConfigurationUnitResultSource.ConfigurationSet:
                    return string.Format(Resources.ConfigurationUnitFailedConfigSet, resultCode);
                case ConfigurationUnitResultSource.Internal:
                    return string.Format(Resources.ConfigurationUnitFailedInternal, resultCode);
                case ConfigurationUnitResultSource.Precondition:
                    return string.Format(Resources.ConfigurationUnitFailedPrecondition, resultCode);
                case ConfigurationUnitResultSource.SystemState:
                    return string.Format(Resources.ConfigurationUnitFailedSystemState, resultCode);
                case ConfigurationUnitResultSource.UnitProcessing:
                    return string.Format(Resources.ConfigurationUnitFailedUnitProcessing, resultCode);
            }

            if (unitResult.Unit.State == ConfigurationUnitState.Skipped)
            {
                return string.Format(Resources.ConfigurationUnitSkipped, resultCode);
            }

            return string.Format(Resources.ConfigurationUnitFailed, resultCode);
        }
    }
}
