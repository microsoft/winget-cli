// -----------------------------------------------------------------------------
// <copyright file="ClientCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Commands.Common
{
    using System;
    using System.Collections.Generic;
    using System.Management.Automation;
    using System.Runtime.InteropServices;
    using Microsoft.Management.Deployment;
    using Microsoft.WinGet.Client.Engine.Exceptions;
    using Microsoft.WinGet.Client.Engine.Helpers;
    using Microsoft.WinGet.Client.Engine.Properties;

    /// <summary>
    /// This is the base class for all of the commands in this module that use the COM APIs.
    /// </summary>
    public abstract class ClientCommand : BaseCommand
    {
        static ClientCommand()
        {
            InitializeUndockedRegFreeWinRT();
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ClientCommand"/> class.
        /// </summary>
        /// <param name="psCmdlet">psCmdlet.</param>
        internal ClientCommand(PSCmdlet psCmdlet)
            : base(psCmdlet)
        {
#if POWERSHELL_WINDOWS
            throw new NotSupportedException(Resources.WindowsPowerShellNotSupported);
#endif
        }

        /// <summary>
        /// Retrieves the specified source or all sources if <paramref name="source" /> is null.
        /// </summary>
        /// <returns>A list of <see cref="PackageCatalogReference" /> instances.</returns>
        /// <param name="source">The name of the source to retrieve. If null, then all sources are returned.</param>
        /// <exception cref="ArgumentException">The source does not exist.</exception>
        protected static IReadOnlyList<PackageCatalogReference> GetPackageCatalogReferences(string source)
        {
            if (string.IsNullOrEmpty(source))
            {
                return ManagementDeploymentFactory.Instance.GetPackageManager().GetPackageCatalogs();
            }
            else
            {
                return new List<PackageCatalogReference>()
                {
                    ManagementDeploymentFactory.Instance.GetPackageManager().GetPackageCatalogByName(source)
                        ?? throw new InvalidSourceException(source),
                };
            }
        }

        [DllImport("winrtact.dll", EntryPoint = "winrtact_Initialize", ExactSpelling = true, PreserveSig = true)]
        private static extern void InitializeUndockedRegFreeWinRT();
    }
}
