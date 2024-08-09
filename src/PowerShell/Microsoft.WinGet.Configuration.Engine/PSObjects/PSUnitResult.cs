// -----------------------------------------------------------------------------
// <copyright file="PSUnitResult.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Configuration.Engine.PSObjects
{
    using Microsoft.Management.Configuration;
    using Microsoft.WinGet.Configuration.Engine.Exceptions;
    using Microsoft.WinGet.Resources;

    /// <summary>
    /// Unit result.
    /// </summary>
    public abstract class PSUnitResult
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="PSUnitResult"/> class.
        /// </summary>
        /// <param name="unit">Unit.</param>
        /// <param name="resultInfo">Result info.</param>
        internal PSUnitResult(ConfigurationUnit unit, IConfigurationUnitResultInformation resultInfo)
        {
            this.Type = unit.Type;
            this.ResultCode = resultInfo.ResultCode?.HResult ?? ErrorCodes.S_OK;

            if (this.ResultCode != ErrorCodes.S_OK)
            {
                this.Message = this.GetUnitMessage(unit, resultInfo);
                this.Description = resultInfo.Description.Trim();
                this.Details = resultInfo.Details;
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
        /// Gets the message.
        /// </summary>
        public string? Message { get; private init; }

        /// <summary>
        /// Gets the short description.
        /// </summary>
        public string? Description { get; private init; }

        /// <summary>
        /// Gets detailed information.
        /// </summary>
        public string? Details { get; private init; }

        private string GetUnitMessage(ConfigurationUnit unit, IConfigurationUnitResultInformation resultInfo)
        {
            if (resultInfo.ResultCode == null)
            {
                if (unit.State == ConfigurationUnitState.Skipped)
                {
                    return string.Format(Resources.ConfigurationUnitSkipped, "null");
                }

                return string.Format(Resources.ConfigurationUnitFailed, "null");
            }

            int resultCode = resultInfo.ResultCode.HResult;
            switch (resultCode)
            {
                case ErrorCodes.WingetConfigErrorDuplicateIdentifier:
                    return string.Format(Resources.ConfigurationUnitHasDuplicateIdentifier, unit.Identifier);
                case ErrorCodes.WingetConfigErrorMissingDependency:
                    return string.Format(Resources.ConfigurationUnitHasMissingDependency, resultInfo.Details);
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
                case ErrorCodes.WinGetConfigUnitSettingConfigRoot:
                    return Resources.WinGetConfigUnitSettingConfigRoot;
                case ErrorCodes.WinGetConfigUnitImportModuleAdmin:
                    return Resources.WinGetConfigUnitImportModuleAdmin;
            }

            switch (resultInfo.ResultSource)
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

            if (unit.State == ConfigurationUnitState.Skipped)
            {
                return string.Format(Resources.ConfigurationUnitSkipped, resultCode);
            }

            return string.Format(Resources.ConfigurationUnitFailed, resultCode);
        }
    }
}
