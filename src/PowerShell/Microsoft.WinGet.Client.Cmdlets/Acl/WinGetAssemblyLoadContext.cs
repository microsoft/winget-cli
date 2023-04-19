// -----------------------------------------------------------------------------
// <copyright file="WinGetAssemblyLoadContext.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------
#if !POWERSHELL_WINDOWS
namespace Microsoft.WinGet.Client.Acl
{
    using System;
    using System.IO;
    using System.Reflection;
    using System.Runtime.Loader;

    /// <summary>
    /// Custom assembly load context for this module.
    /// This helps us load our dependencies without carrying about apps importing this module.
    /// All dependencies except the Engine dll needs to be under a Dependencies directory.
    /// </summary>
    internal class WinGetAssemblyLoadContext : AssemblyLoadContext
    {
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
            string path = $"{Path.Combine(SharedDependencyPath, assemblyName.Name)}.dll";
            if (File.Exists(path))
            {
                return WinGetAcl.LoadFromAssemblyName(assemblyName);
            }

            return null;
        }

        /// <inheritdoc/>
        protected override Assembly Load(AssemblyName assemblyName)
        {
            string path = $"{Path.Combine(DirectDependencyPath, assemblyName.Name)}.dll";
            if (File.Exists(path))
            {
                return this.LoadFromAssemblyPath(path);
            }

            path = $"{Path.Combine(SharedDependencyPath, assemblyName.Name)}.dll";
            if (File.Exists(path))
            {
                return this.LoadFromAssemblyPath(path);
            }

            return null;
        }

        /// <inheritdoc/>
        protected override IntPtr LoadUnmanagedDll(string unmanagedDllName)
        {
            string path = Path.Combine(DirectDependencyPath, unmanagedDllName);
            if (File.Exists(path))
            {
                return this.LoadUnmanagedDllFromPath(path);
            }

            return IntPtr.Zero;
        }
    }
}
#endif
