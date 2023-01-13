// -----------------------------------------------------------------------------
// <copyright file="BaseClientCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Commands.Common
{
    using System;
    using System.Collections.Generic;
    using System.Runtime.InteropServices;
    using Microsoft.Management.Deployment;
    using Microsoft.WinGet.Client.Exceptions;
    using Microsoft.WinGet.Client.Factories;

    /// <summary>
    /// This is the base class for all of the commands in this module that use the COM APIs.
    /// </summary>
    public abstract class BaseClientCommand : BaseCommand
    {
        static BaseClientCommand()
        {
            InitializeUndockedRegFreeWinRT();
        }

        /// <summary>
        /// Gets the instance of the <see cref="ComObjectFactory" /> class.
        /// </summary>
        protected static Lazy<ComObjectFactory> ComObjectFactory { get; } = new ();

        /// <summary>
        /// Gets the instance of the <see cref="PackageManager" /> class.
        /// </summary>
        protected static Lazy<PackageManager> PackageManager { get; } = new (() => ComObjectFactory.Value.CreatePackageManager());

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
                        ?? throw new InvalidSourceException(source),
                };
            }
        }

        [DllImport("winrtact.dll", EntryPoint = "winrtact_Initialize", ExactSpelling = true, PreserveSig = true)]
        private static extern void InitializeUndockedRegFreeWinRT();
    }
}