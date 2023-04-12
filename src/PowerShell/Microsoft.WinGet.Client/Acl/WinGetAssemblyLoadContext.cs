// -----------------------------------------------------------------------------
// <copyright file="WinGetAssemblyLoadContext.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------
#if !POWERSHELL_WINDOWS
namespace Microsoft.WinGet.Client.Acl
{
    using System.IO;
    using System.Reflection;
    using System.Runtime.Loader;

    /// <summary>
    /// Custom assembly load context for this module.
    /// This helps us load our dependencies without carrying about apps importing this module.
    /// </summary>
    internal class WinGetAssemblyLoadContext : AssemblyLoadContext
    {
        private static readonly WinGetAssemblyLoadContext WinGetAcl = new (Path.Combine(Path.GetDirectoryName(typeof(WinGetAssemblyLoadContext).Assembly.Location)));

        private readonly string location;

        private WinGetAssemblyLoadContext(string location)
            : base("WinGetAssemblyLoadContext", isCollectible: false)
        {
            this.location = location;
        }

        /// <summary>
        /// Handler to resolve assemblies.
        /// </summary>
        /// <param name="context">Assembly load context.</param>
        /// <param name="name">Assembly name.</param>
        /// <returns>The assembly, null if not in our assembly location.</returns>
        internal static Assembly ResolvingHandler(AssemblyLoadContext context, AssemblyName name)
        {
            return WinGetAcl.LoadFromAssemblyName(name);
        }

        /// <inheritdoc/>
        protected override Assembly Load(AssemblyName assemblyName)
        {
            string path = $"{Path.Combine(this.location, "Dependencies", assemblyName.Name)}.dll";

            if (File.Exists(path))
            {
                return this.LoadFromAssemblyPath(path);
            }

            return null;
        }
    }
}
#endif
