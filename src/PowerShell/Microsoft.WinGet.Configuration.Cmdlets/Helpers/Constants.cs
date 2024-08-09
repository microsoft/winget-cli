// -----------------------------------------------------------------------------
// <copyright file="Constants.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Configuration.Helpers
{
    /// <summary>
    /// Constants.
    /// </summary>
    internal static class Constants
    {
#pragma warning disable SA1600 // ElementsMustBeDocumented
        internal static class ParameterSet
        {
            internal const string OpenConfigurationSetFromFile = "OpenConfigurationSetFromFile";
            internal const string OpenConfigurationSetFromString = "OpenConfigurationSetFromString";
            internal const string OpenConfigurationSetFromHistory = "OpenConfigurationSetFromHistory";
            internal const string OpenAllConfigurationSetsFromHistory = "OpenAllConfigurationSetsFromHistory";
        }
#pragma warning restore SA1600 // ElementsMustBeDocumented
    }
}
