// -----------------------------------------------------------------------------
// <copyright file="ModuleInit.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------
#if !POWERSHELL_WINDOWS
namespace Microsoft.WinGet.Client.Acl
{
    using System.Management.Automation;
    using System.Runtime.Loader;

    /// <summary>
    /// Initialization class for this module.
    /// </summary>
    public class ModuleInit : IModuleAssemblyInitializer, IModuleAssemblyCleanup
    {
        /// <inheritdoc/>
        public void OnImport()
        {
            AssemblyLoadContext.Default.Resolving += WinGetAssemblyLoadContext.ResolvingHandler;
        }

        /// <inheritdoc/>
        public void OnRemove(PSModuleInfo module)
        {
            AssemblyLoadContext.Default.Resolving -= WinGetAssemblyLoadContext.ResolvingHandler;
        }
    }
}
#endif