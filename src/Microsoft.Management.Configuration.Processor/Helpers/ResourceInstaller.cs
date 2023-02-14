// -----------------------------------------------------------------------------
// <copyright file="ResourceInstaller.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Helpers
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using System.Management.Automation;
    using System.Management.Automation.Runspaces;
    using Microsoft.Management.Configuration.Processor.Constants;
    using Microsoft.Management.Configuration.Processor.Exceptions;
    using Microsoft.Management.Configuration.Processor.Extensions;
    using static Microsoft.Management.Configuration.Processor.Constants.PowerShellConstants;

    /// <summary>
    /// Looks for the module of a DSC resource and installs it.
    /// </summary>
    internal class ResourceInstaller
    {
        private readonly Runspace runspace;
        private readonly ConfigurationUnitInternal unitInternal;

        /// <summary>
        /// Initializes a new instance of the <see cref="ResourceInstaller"/> class.
        /// </summary>
        /// <param name="runspace">Runspace.</param>
        /// <param name="unitInternal">Unit internal.</param>
        public ResourceInstaller(Runspace runspace, ConfigurationUnitInternal unitInternal)
        {
            this.runspace = runspace;
            this.unitInternal = unitInternal;
        }

        /// <summary>
        /// Finds a resources, saves it in a quarantine location, verifies it and install it.
        /// </summary>
        public void InstallResource()
        {
            PSObject? getDscResourceInfo = this.FindDscResource();

            if (getDscResourceInfo is not null)
            {
                // TODO: hook up policies and enable this and create the quarantine path, per set.
                // this.ValidateModule(getDscResourceInfo, Path.Combine(Path.GetTempPath(), Guid.NewGuid().ToString()));

                // Install module for now, when the validation module gets fully implemented
                // we can improve the performance by moving the modules file somewhere in the
                // PSModulePath directory. We need to either Install-Module or move it for DSC cmdlets
                // to find the resources as the quarantine path will not be in the PSModulePath of
                // this runspace.
                this.InstallModule(getDscResourceInfo);
            }
        }

        /// <summary>
        /// Calls Find-DscResource.
        /// </summary>
        /// <returns>The first result of Find-DscResource or null if no resource is found.</returns>
        internal PSObject? FindDscResource()
        {
            var parameters = new Dictionary<string, object>()
            {
                { Parameters.Name, this.unitInternal.Unit.UnitName },
            };

            if (this.unitInternal.Module is not null)
            {
                parameters.Add(Parameters.ModuleName, this.unitInternal.Module.Name);

                if (this.unitInternal.Module.Version is not null)
                {
                    parameters.Add(Parameters.MinimumVersion, this.unitInternal.Module.Version);
                }

                if (this.unitInternal.Module.MaximumVersion is not null)
                {
                    parameters.Add(Parameters.MaximumVersion, this.unitInternal.Module.MaximumVersion);
                }

                if (this.unitInternal.Module.RequiredVersion is not null)
                {
                    parameters.Add(Parameters.RequiredVersion, this.unitInternal.Module.RequiredVersion);
                }
            }

            string? repository = this.unitInternal.GetDirective(DirectiveConstants.Repository);
            if (!string.IsNullOrEmpty(repository))
            {
                parameters.Add(Parameters.Repository, repository);
            }

            using PowerShell pwsh = PowerShell.Create(this.runspace);

            // TODO: Implement prerelease directive.
            // The result is just a PSCustomObject with a type name of Microsoft.PowerShell.Commands.PSGetDscResourceInfo.
            var result = pwsh.AddCommand(Commands.FindDscResource)
                             .AddParameters(parameters)
                             .InvokeAndStopOnError();

            // When no module is passed and a resource is not found, this will return an empty list. If a module
            // is specified and no resource is found then it will fail earlier because of a Write-Error.
            if (result is null || result.Count == 0)
            {
                string message = $"Resource = {this.unitInternal.Unit.UnitName}";
                throw new FindDscResourceNotFoundException(message);
            }

            return result[0];
        }

        /// <summary>
        /// Validates the module. Calls Save-Module and verify the signature of the module.
        /// </summary>
        /// <param name="getDscResourceInfo">Result of Find-DscResource.</param>
        /// <param name="quarantinePath">Quarantine path.</param>
        internal void ValidateModule(PSObject getDscResourceInfo, string quarantinePath)
        {
            // First save the module in the quarantine location.
            using PowerShell pwsh = PowerShell.Create(this.runspace);

            _ = pwsh.AddCommand(Commands.SaveModule)
                    .AddParameter(Parameters.Path, quarantinePath)
                    .AddParameter(Parameters.InputObject, getDscResourceInfo)
                    .InvokeAndStopOnError();

            // Now validate the signatures for dll, psd1 and psm1 files.
            string savedModulePath = Path.Combine(
                quarantinePath,
                (string)getDscResourceInfo.Properties[Parameters.ModuleName].Value);
            string[] paths = new string[]
            {
                $"{savedModulePath}\\*.dll",
                $"{savedModulePath}\\*.psd1",
                $"{savedModulePath}\\*.psm1",
            };
            this.VerifySignature(paths);
        }

        /// <summary>
        /// Validates the signature by calling the result of Get-ChildItems paths -Recurse and pipe
        /// it to Get-AuthenticodeSignature.
        /// </summary>
        /// <param name="paths">Array of strings with the paths to inspect.</param>
        internal void VerifySignature(string[] paths)
        {
            using PowerShell pwsh = PowerShell.Create(this.runspace);

            var signatures = pwsh.AddCommand(Commands.GetChildItem)
                                 .AddParameter(Parameters.Path, paths)
                                 .AddParameter(Parameters.Recurse)
                                 .AddCommand(Commands.GetAuthenticodeSignature)
                                 .InvokeAndStopOnError<Signature>();

            foreach (var signature in signatures)
            {
                if (signature.Status != SignatureStatus.Valid)
                {
                    throw new InvalidOperationException($"{signature.Status} {signature.Path}");
                }
            }
        }

        /// <summary>
        /// Calls Install-Module taking the input object from the result of Find-DscResource.
        /// </summary>
        /// <param name="getDscResourceInfo">Result of Find-DscResource.</param>
        internal void InstallModule(PSObject getDscResourceInfo)
        {
            using PowerShell pwsh = PowerShell.Create(this.runspace);

            // If the repository is untrusted, it will fail with:
            //   Microsoft.PowerShell.Commands.WriteErrorException : Exception calling "ShouldContinue" with "5"
            //   argument(s): "A command that prompts the user failed because the host program or the command type
            //   does not support user interaction.
            // If its trusted, PowerShellGets adds the Force parameter to the call to PackageManager\Install-Package.
            // TODO: Once we have policies, we should remove Force. For hosted environments and depending
            // on the policy we will trust PSGallery when we create the Runspace or add Force here.
            _ = pwsh.AddCommand(Commands.InstallModule)
                    .AddParameter(Parameters.InputObject, getDscResourceInfo)
                    .AddParameter(Parameters.Force)
                    .InvokeAndStopOnError();
        }
    }
}
