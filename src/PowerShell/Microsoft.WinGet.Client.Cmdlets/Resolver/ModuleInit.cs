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

#if !POWERSHELL_WINDOWS
    using System.Runtime.Loader;
#else
    using Microsoft.WinGet.Client.Cmdlets.Resolver;
#endif

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

#if !POWERSHELL_WINDOWS
            AssemblyLoadContext.Default.Resolving += WinGetAssemblyLoadContext.ResolvingHandler;
#else
            // If we really need to avoid dependency conflicts, we could create a custom domain and handle the serialization boundaries.
            // PowerShell doesn't recommended because its complications.
            AppDomain currentDomain = AppDomain.CurrentDomain;
            currentDomain.AssemblyResolve += WinGetAppDomain.Handler;
#endif
        }

        /// <inheritdoc/>
        public void OnRemove(PSModuleInfo module)
        {
#if !POWERSHELL_WINDOWS
            AssemblyLoadContext.Default.Resolving -= WinGetAssemblyLoadContext.ResolvingHandler;
#else
            AppDomain currentDomain = AppDomain.CurrentDomain;
            currentDomain.AssemblyResolve -= WinGetAppDomain.Handler;
#endif
        }
    }
}
