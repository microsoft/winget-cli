// -----------------------------------------------------------------------------
// <copyright file="ManagementDeploymentCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Commands.Common
{
    using System;
    using System.Collections.Generic;
    using System.Management.Automation;
    using System.Threading.Tasks;
    using Microsoft.Management.Deployment;
    using Microsoft.WinGet.Client.Engine.Common;
    using Microsoft.WinGet.Client.Engine.Exceptions;
    using Microsoft.WinGet.Client.Engine.Helpers;

    /// <summary>
    /// This is the base class for all of the commands in this module that use the COM APIs.
    /// </summary>
    public abstract class ManagementDeploymentCommand : BaseCommand
    {
        static ManagementDeploymentCommand()
        {
            WinRTHelpers.Initialize();
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ManagementDeploymentCommand"/> class.
        /// </summary>
        /// <param name="psCmdlet">psCmdlet.</param>
        internal ManagementDeploymentCommand(PSCmdlet psCmdlet)
            : base(psCmdlet)
        {
#if POWERSHELL_WINDOWS
            if (Utilities.UsesInProcWinget)
            {
                throw new WindowsPowerShellNotSupported();
            }
#endif
        }

        /// <summary>
        /// Retrieves the specified source or all sources if <paramref name="source" /> is null.
        /// </summary>
        /// <returns>A list of <see cref="PackageCatalogReference" /> instances.</returns>
        /// <param name="source">The name of the source to retrieve. If null, then all sources are returned.</param>
        /// <exception cref="ArgumentException">The source does not exist.</exception>
        internal IReadOnlyList<PackageCatalogReference> GetPackageCatalogReferences(string? source)
        {
            if (string.IsNullOrEmpty(source))
            {
                return PackageManagerWrapper.Instance.GetPackageCatalogs();
            }
            else
            {
                return new List<PackageCatalogReference>()
                {
                    PackageManagerWrapper.Instance.GetPackageCatalogByName(source!)
                        ?? throw new InvalidSourceException(source!),
                };
            }
        }

        /// <summary>
        /// Executes the cmdlet. All cmdlets that uses the COM APIs and don't call async functions MUST use this method.
        /// The inproc COM API may deadlock on an STA thread.
        /// </summary>
        /// <typeparam name="TResult">The type of result of the cmdlet.</typeparam>
        /// <param name="func">Cmdlet function.</param>
        /// <returns>The result of the cmdlet.</returns>
        protected TResult Execute<TResult>(Func<TResult> func)
        {
            if (Utilities.UsesInProcWinget)
            {
                return this.RunOnMTA(func);
            }

            return func();
        }

        /// <summary>
        /// Executes the cmdlet in a different thread and waits for results.
        /// </summary>
        /// <typeparam name="TResult">The type of result of the cmdlet.</typeparam>
        /// <param name="func">Cmdlet function.</param>
        /// <returns>The result of the cmdlet.</returns>
        protected TResult Execute<TResult>(Func<Task<TResult>> func)
        {
            var runningTask = this.RunOnMTA(
                async () =>
                {
                    return await func();
                });

            this.Wait(runningTask);
            return runningTask.Result;
        }
    }
}
