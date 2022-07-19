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
    using System.Runtime.InteropServices;
    using System.Security.Principal;
    using Microsoft.Management.Deployment;
    using Microsoft.WinGet.Client.Factories;
    using Microsoft.WinGet.Client.Helpers;

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
            var admin = CheckIfAdministrator();
            if (admin)
            {
                throw new Exception(Constants.ResourceManager.GetString("ExceptionMessages_NoAdmin"));
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

#pragma warning disable IDE0051 // Remove unused private members
        private static bool InitializedUndockedRegFreeWinRT { get; set; } = InitializeUndockedRegFreeWinRT();
#pragma warning restore IDE0051 // Remove unused private members

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
                        ?? throw new ArgumentException(Constants.ResourceManager.GetString("ExceptionMessages_SourceNotFound")),
                };
            }
        }

        [DllImport("winrtact.dll", PreserveSig = false)]
#pragma warning disable SA1300 // Element should begin with upper-case letter
        private static extern void winrtact_Initialize();
#pragma warning restore SA1300 // Element should begin with upper-case letter

        private static bool InitializeUndockedRegFreeWinRT()
        {
            winrtact_Initialize();
            return true;
        }

        private static bool CheckIfAdministrator()
        {
            WindowsIdentity identity = WindowsIdentity.GetCurrent();
            var principal = new WindowsPrincipal(identity);
            return principal.IsInRole(WindowsBuiltInRole.Administrator);
        }
    }
}
