// -----------------------------------------------------------------------------
// <copyright file="ModuleInit.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Resolver
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Management.Automation;
    using System.Runtime.InteropServices;
    using System.Runtime.Loader;

    /// <summary>
    /// Initialization class for this module.
    /// </summary>
    public class ModuleInit : IModuleAssemblyInitializer, IModuleAssemblyCleanup
    {
        private static readonly IEnumerable<Architecture> ValidArchs = new Architecture[] { Architecture.X86, Architecture.X64, Architecture.Arm64 };

        /// <inheritdoc/>
        public void OnImport()
        {
            var arch = RuntimeInformation.ProcessArchitecture;
            if (!ValidArchs.Contains(arch))
            {
                throw new NotSupportedException(arch.ToString());
            }

            AssemblyLoadContext.Default.Resolving += WinGetAssemblyLoadContext.ResolvingHandler;
            AssemblyLoadContext.Default.ResolvingUnmanagedDll += WinGetAssemblyLoadContext.ResolvingUnmanagedDllHandler;
        }

        /// <inheritdoc/>
        public void OnRemove(PSModuleInfo module)
        {
            AssemblyLoadContext.Default.ResolvingUnmanagedDll -= WinGetAssemblyLoadContext.ResolvingUnmanagedDllHandler;
            AssemblyLoadContext.Default.Resolving -= WinGetAssemblyLoadContext.ResolvingHandler;
        }
    }
}
