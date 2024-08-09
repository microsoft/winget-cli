// -----------------------------------------------------------------------------
// <copyright file="UnitSettingConfigRootException.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Exceptions
{
    using System;

    /// <summary>
    /// A setting uses the config root variable and the Path was not set in the ConfigurationSet.
    /// </summary>
    internal class UnitSettingConfigRootException : Exception
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="UnitSettingConfigRootException"/> class.
        /// </summary>
        /// <param name="unitName">Unit name.</param>
        /// <param name="setting">Setting.</param>
        public UnitSettingConfigRootException(string unitName, string setting)
            : base($"Unit: {unitName} Setting {setting} requires the ConfigurationSet Path")
        {
            this.HResult = ErrorCodes.WinGetConfigUnitSettingConfigRoot;
            this.UnitName = unitName;
            this.Setting = setting;
        }

        /// <summary>
        /// Gets the resource name.
        /// </summary>
        public string UnitName { get; }

        /// <summary>
        /// Gets the setting that reference the config root variable.
        /// </summary>
        public string Setting { get; }
    }
}
