// -----------------------------------------------------------------------------
// <copyright file="LocalizedStrings.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace WinGetMCPServer
{
    using Windows.ApplicationModel.Resources;

    /// <summary>
    /// Provides localized strings from the WinGet package resources, shared with the CLI.
    /// </summary>
    internal static class LocalizedStrings
    {
        private static readonly Lazy<ResourceLoader?> Loader = new Lazy<ResourceLoader?>(CreateLoader);

        /// <summary>
        /// Gets a localized string. As in the CLI, the resource key is returned when the resources are unavailable.
        /// </summary>
        /// <param name="key">The resource key.</param>
        /// <returns>The localized string.</returns>
        public static string Get(string key)
        {
            try
            {
                string? value = Loader.Value?.GetString(key);
                if (!string.IsNullOrEmpty(value))
                {
                    return value;
                }
            }
            catch (Exception)
            {
            }

            return key;
        }

        /// <summary>
        /// Gets a localized format string and formats it with the provided arguments.
        /// </summary>
        /// <param name="key">The resource key.</param>
        /// <param name="args">The format arguments.</param>
        /// <returns>The formatted localized string.</returns>
        public static string Format(string key, params object?[] args)
        {
            string format = Get(key);

            try
            {
                return string.Format(format, args);
            }
            catch (FormatException)
            {
                return format;
            }
        }

        private static ResourceLoader? CreateLoader()
        {
            try
            {
                // Mirrors AppInstallerSharedLib\Resources.cpp: the default constructor throws a catchable
                // error when resources.pri is not found, whereas GetForViewIndependentUse can fail in a way
                // that cannot be caught. Constructing the default loader first guards against that.
                _ = new ResourceLoader();
                return ResourceLoader.GetForViewIndependentUse("winget");
            }
            catch (Exception)
            {
                return null;
            }
        }
    }
}
