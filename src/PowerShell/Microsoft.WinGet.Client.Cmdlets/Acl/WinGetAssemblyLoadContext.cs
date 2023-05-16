﻿// -----------------------------------------------------------------------------
// <copyright file="WinGetAssemblyLoadContext.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------
#if !POWERSHELL_WINDOWS
namespace Microsoft.WinGet.Client.Acl
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using System.Reflection;
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

        private static readonly string SharedDependencyPath = Path.Combine(
            Path.GetDirectoryName(typeof(WinGetAssemblyLoadContext).Assembly.Location),
            "SharedDependencies");

        private static readonly string DirectDependencyPath = Path.Combine(
            Path.GetDirectoryName(typeof(WinGetAssemblyLoadContext).Assembly.Location),
            "DirectDependencies");

        private static readonly WinGetAssemblyLoadContext WinGetAcl = new ();

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

            string path = $"{Path.Combine(DirectDependencyPath, assemblyName.Name)}.dll";
            if (File.Exists(path))
            {
                return WinGetAcl.LoadFromAssemblyName(assemblyName);
            }

            return null;
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
            string path = Path.Combine(SharedDependencyPath, unmanagedDllName);
            if (File.Exists(path))
            {
                return this.LoadUnmanagedDllFromPath(path);
            }

            return IntPtr.Zero;
        }
    }
}
#endif
