// -----------------------------------------------------------------------------
// <copyright file="WinGetAssemblyLoadContext.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------
#if !POWERSHELL_WINDOWS
namespace Microsoft.WinGet.Resolver
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using System.Reflection;
    using System.Runtime.InteropServices;
    using System.Runtime.Loader;

    /// <summary>
    /// Custom assembly load context for this module.
    /// This helps us load our dependencies without carrying about apps importing this module.
    /// All dependencies except the Engine dll needs to be under a Dependencies directory.
    /// </summary>
    internal class WinGetAssemblyLoadContext : AssemblyLoadContext
    {
        // The assemblies must be loaded in the default context.
        // Loading WinRT.Runtime.dll in an ALC when is already loaded in the default context
        // will result on 'Attempt to update previously set global instance.'
        private static readonly IEnumerable<string> DefaultContextAssemblies = new string[]
        {
            @"WinRT.Runtime.dll",
        };

        private static readonly string SharedDependencyPath;
        private static readonly string SharedArchDependencyPath;
        private static readonly string DirectDependencyPath;

        private static readonly WinGetAssemblyLoadContext WinGetAcl = new ();

        static WinGetAssemblyLoadContext()
        {
            var self = typeof(WinGetAssemblyLoadContext).Assembly;
            SharedDependencyPath = Path.Combine(
                Path.GetDirectoryName(self.Location),
                "SharedDependencies");
            SharedArchDependencyPath = Path.Combine(
                SharedDependencyPath,
                RuntimeInformation.ProcessArchitecture.ToString().ToLower());
            DirectDependencyPath = Path.Combine(
                Path.GetDirectoryName(self.Location),
                "DirectDependencies");
        }

        private WinGetAssemblyLoadContext()
            : base("WinGetAssemblyLoadContext", isCollectible: false)
        {
        }

        /// <summary>
        /// Handler to resolve assemblies.
        /// </summary>
        /// <param name="context">Assembly load context.</param>
        /// <param name="assemblyName">Assembly name.</param>
        /// <returns>The assembly, null if not in our assembly location.</returns>
        internal static Assembly ResolvingHandler(AssemblyLoadContext context, AssemblyName assemblyName)
        {
            string name = $"{assemblyName.Name}.dll";
            if (DefaultContextAssemblies.Any(a => a.Equals(name, StringComparison.OrdinalIgnoreCase)))
            {
                string sharedPath = Path.Combine(SharedDependencyPath, name);
                if (File.Exists(sharedPath))
                {
                    return AssemblyLoadContext.Default.LoadFromAssemblyPath(sharedPath);
                }
            }

            string path = Path.Combine(DirectDependencyPath, name);
            if (File.Exists(path))
            {
                return WinGetAcl.LoadFromAssemblyName(assemblyName);
            }

            return null;
        }

        /// <summary>
        /// Handler to resolve unmanaged assemblies.
        /// </summary>
        /// <param name="assembly">Assembly initiating the unmanaged load.</param>
        /// <param name="unmanagedDllName">Unmanaged dll name.</param>
        /// <returns>The assembly ptr, zero if not in our assembly location.</returns>
        internal static IntPtr ResolvingUnmanagedDllHandler(Assembly assembly, string unmanagedDllName)
        {
            return WinGetAcl.LoadUnmanagedDll(unmanagedDllName);
        }

        /// <inheritdoc/>
        protected override Assembly Load(AssemblyName assemblyName)
        {
            string name = $"{assemblyName.Name}.dll";
            if (DefaultContextAssemblies.Any(a => a.Equals(name, StringComparison.OrdinalIgnoreCase)))
            {
                return null;
            }

            string path = Path.Combine(SharedDependencyPath, name);
            if (File.Exists(path))
            {
                return this.LoadFromAssemblyPath(path);
            }

            path = Path.Combine(SharedArchDependencyPath, name);
            if (File.Exists(path))
            {
                return this.LoadFromAssemblyPath(path);
            }

            path = Path.Combine(DirectDependencyPath, name);
            if (File.Exists(path))
            {
                return this.LoadFromAssemblyPath(path);
            }

            return null;
        }

        /// <inheritdoc/>
        protected override IntPtr LoadUnmanagedDll(string unmanagedDllName)
        {
            string path = Path.Combine(SharedArchDependencyPath, unmanagedDllName);
            if (File.Exists(path))
            {
                return this.LoadUnmanagedDllFromPath(path);
            }

            return IntPtr.Zero;
        }
    }
}
#endif
