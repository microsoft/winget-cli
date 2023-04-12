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
    /// </summary>
    internal class WinGetAssemblyLoadContext : AssemblyLoadContext
    {
        private static readonly string ExpectedDependencyPath = Path.Combine(
            Path.GetDirectoryName(typeof(WinGetAssemblyLoadContext).Assembly.Location),
            "Dependencies");

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
            string path = $"{Path.Combine(ExpectedDependencyPath, assemblyName.Name)}.dll";
            if (File.Exists(path))
            {
                return WinGetAcl.LoadFromAssemblyName(assemblyName);
            }

            return null;
        }

        /// <summary>
        /// Handler to resolve unmanaged dlls.
        /// </summary>
        /// <param name="assembly">Assembly.</param>
        /// <param name="unmanagedDllName">Unmanaged dll name.</param>
        /// <returns>The assembly, or zero if not found.</returns>
        internal static IntPtr ResolvingUnmanagedDllHandler(Assembly assembly, string unmanagedDllName)
        {
            string fullPath = $"{Path.Combine(ExpectedDependencyPath, unmanagedDllName)}";
            if (File.Exists(fullPath))
            {
                return WinGetAcl.LoadUnmanagedDll(fullPath);
            }

            return IntPtr.Zero;
        }

        /// <inheritdoc/>
        protected override Assembly Load(AssemblyName assemblyName)
        {
            string path = $"{Path.Combine(ExpectedDependencyPath, assemblyName.Name)}.dll";
            if (File.Exists(path))
            {
                return this.LoadFromAssemblyPath(path);
            }

            return null;
        }

        /// <inheritdoc/>
        protected override IntPtr LoadUnmanagedDll(string unmanagedDllName)
        {
            if (File.Exists(unmanagedDllName))
            {
                return this.LoadUnmanagedDllFromPath(unmanagedDllName);
            }

            return IntPtr.Zero;
        }
    }
}
#endif
