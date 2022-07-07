// -----------------------------------------------------------------------------
// <copyright file="BaseClientCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Commands
{
    using System;
    using System.Collections.Generic;
    using System.Management.Automation;
    using System.Security.Principal;
    using Microsoft.Management.Deployment;
    using Microsoft.WinGet.Client.Factories;

    /// <summary>
    /// This is the base class for all commands in this module.
    /// </summary>
    public class BaseClientCommand : PSCmdlet
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="BaseClientCommand"/> class.
        /// </summary>
        public BaseClientCommand()
            : base()
        {
            var identity = WindowsIdentity.GetCurrent();
            var principal = new WindowsPrincipal(identity);
            var admin = principal.IsInRole(WindowsBuiltInRole.Administrator);
            if (admin)
            {
                throw new Exception(@"This cmdlet is currently not available in an administrator context.");
            }
        }

        /// <summary>
        /// Gets the instance of the <see cref="ComObjectFactory" /> class.
        /// </summary>
        protected static Lazy<ComObjectFactory> ComObjectFactory { get; } = new Lazy<ComObjectFactory>();

        /// <summary>
        /// Gets the instance of the <see cref="PackageManager" /> class.
        /// </summary>
        protected static Lazy<PackageManager> PackageManager { get; } = new Lazy<PackageManager>(() => ComObjectFactory.Value.CreatePackageManager());

        /// <summary>
        /// Retrieves the specified source or all sources if <paramref name="source" /> is null.
        /// </summary>
        /// <returns>A list of <see cref="PackageCatalogReference" /> instances.</returns>
        /// <param name="source">The name of the source to retrieve. If null, then all sources are returned.</param>
        /// <exception cref="ArgumentException">The source does not exist.</exception>
        protected static IReadOnlyList<PackageCatalogReference> GetPackageCatalogReferences(string source)
        {
            if (source is null)
            {
                return PackageManager.Value.GetPackageCatalogs();
            }
            else
            {
                return new List<PackageCatalogReference>()
                {
                    PackageManager.Value.GetPackageCatalogByName(source)
                        ?? throw new ArgumentException("No sources match the given value: " + source),
                };
            }
        }

    }
}
