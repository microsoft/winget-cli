// -----------------------------------------------------------------------------
// <copyright file="Utilities.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Common
{
    using System.Reflection;
    using System.Resources;
    using System.Security.Principal;

    /// <summary>
    /// This class contains various helper methods for this project.
    /// </summary>
    public static class Utilities
    {
        /// <summary>
        /// Gets the <see cref="ResourceManager" /> instance for the executing assembly.
        /// </summary>
        public static ResourceManager ResourceManager
        {
            get
            {
                Assembly assembly = Assembly.GetExecutingAssembly();
                return new ResourceManager(Constants.ResourcesPath, assembly);
            }
        }

        /// <summary>
        /// Gets a value indicating whether the current assembly is executing in an administrative context.
        /// </summary>
        public static bool ExecutingAsAdministrator
        {
            get
            {
                WindowsIdentity identity = WindowsIdentity.GetCurrent();
                WindowsPrincipal principal = new (identity);
                return principal.IsInRole(WindowsBuiltInRole.Administrator);
            }
        }
    }
}
