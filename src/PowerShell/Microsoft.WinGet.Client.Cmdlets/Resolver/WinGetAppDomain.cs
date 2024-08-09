// -----------------------------------------------------------------------------
// <copyright file="WinGetAppDomain.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------
#if POWERSHELL_WINDOWS

namespace Microsoft.WinGet.Client.Cmdlets.Resolver
{
    using System;
    using System.IO;
    using System.Reflection;
    using System.Runtime.InteropServices;

    /// <summary>
    /// Resolver for assemblies.
    /// </summary>
    internal static class WinGetAppDomain
    {
        private static readonly string SharedDependencyPath;
        private static readonly string SharedArchDependencyPath;
        private static readonly string DirectDependencyPath;

        static WinGetAppDomain()
        {
            var self = typeof(WinGetAppDomain).Assembly;
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

        /// <summary>
        /// Handler to register in the AppDomain.
        /// </summary>
        /// <param name="source">Source.</param>
        /// <param name="args">Event args.</param>
        /// <returns>The assembly if found.</returns>
        public static Assembly Handler(object source, ResolveEventArgs args)
        {
            string name = $"{new AssemblyName(args.Name).Name}.dll";
            string path = Path.Combine(SharedDependencyPath, name);
            if (File.Exists(path))
            {
                return Assembly.LoadFile(path);
            }

            path = Path.Combine(SharedArchDependencyPath, name);
            if (File.Exists(path))
            {
                return Assembly.LoadFile(path);
            }

            path = Path.Combine(DirectDependencyPath, name);
            if (File.Exists(path))
            {
                return Assembly.LoadFile(path);
            }

            return null;
        }
    }
}
#endif
