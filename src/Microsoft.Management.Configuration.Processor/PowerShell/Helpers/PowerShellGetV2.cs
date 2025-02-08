// -----------------------------------------------------------------------------
// <copyright file="PowerShellGetV2.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.PowerShell.Helpers
{
    using System.Collections.Generic;
    using System.Linq;
    using System.Management.Automation;
    using Microsoft.PowerShell.Commands;
    using static Microsoft.Management.Configuration.Processor.PowerShell.Constants.PowerShellConstants;
    using SemanticVersion = Microsoft.Management.Configuration.Processor.Helpers.SemanticVersion;

    /// <summary>
    /// PowerShellGet implementation for 2.2.5 .
    /// </summary>
    internal class PowerShellGetV2 : IPowerShellGet
    {
        private const string AllUsers = "AllUsers";

        /// <inheritdoc/>
        public PSObject? FindModule(
            PowerShell pwsh,
            string moduleName,
            SemanticVersion? semanticVersion,
            SemanticVersion? semanticMinVersion,
            SemanticVersion? semanticMaxVersion,
            string? repository,
            bool? allowPrerelease)
        {
            bool implicitAllowPrerelease = false;

            var parameters = new Dictionary<string, object>()
            {
                { Parameters.Name, moduleName },
            };

            if (semanticVersion != null)
            {
                implicitAllowPrerelease |= semanticVersion.IsPrerelease;
                parameters.Add(Parameters.RequiredVersion, semanticVersion.ToString());
            }

            if (semanticMinVersion != null)
            {
                implicitAllowPrerelease |= semanticMinVersion.IsPrerelease;
                parameters.Add(Parameters.MinimumVersion, semanticMinVersion.ToString());
            }

            if (semanticMaxVersion != null)
            {
                implicitAllowPrerelease |= semanticMaxVersion.IsPrerelease;
                parameters.Add(Parameters.MaximumVersion, semanticMaxVersion.ToString());
            }

            if (!string.IsNullOrEmpty(repository))
            {
                parameters.Add(Parameters.Repository, repository);
            }

            if (allowPrerelease.HasValue || implicitAllowPrerelease)
            {
                // If explicit allowPrerelease = false don't use implicit.
                bool allow = allowPrerelease ?? implicitAllowPrerelease;
                parameters.Add(Parameters.AllowPrerelease, allow);
            }

            pwsh.AddCommand(Commands.FindModule)
                .AddParameters(parameters);

            return pwsh.Invoke().FirstOrDefault();
        }

        /// <inheritdoc/>
        public PSObject? FindDscResource(
            PowerShell pwsh,
            string resourceName,
            string? moduleName,
            SemanticVersion? semanticVersion,
            SemanticVersion? semanticMinVersion,
            SemanticVersion? semanticMaxVersion,
            string? repository,
            bool? allowPrerelease)
        {
            var parameters = new Dictionary<string, object>()
            {
                { Parameters.Name, resourceName },
            };

            bool implicitAllowPrerelease = false;

            if (!string.IsNullOrEmpty(moduleName))
            {
                parameters.Add(Parameters.ModuleName, moduleName);
            }

            if (semanticVersion != null)
            {
                implicitAllowPrerelease |= semanticVersion.IsPrerelease;
                parameters.Add(Parameters.RequiredVersion, semanticVersion.ToString());
            }

            if (semanticMinVersion != null)
            {
                implicitAllowPrerelease |= semanticMinVersion.IsPrerelease;
                parameters.Add(Parameters.MinimumVersion, semanticMinVersion.ToString());
            }

            if (semanticMaxVersion != null)
            {
                implicitAllowPrerelease |= semanticMaxVersion.IsPrerelease;
                parameters.Add(Parameters.MaximumVersion, semanticMaxVersion.ToString());
            }

            if (!string.IsNullOrEmpty(repository))
            {
                parameters.Add(Parameters.Repository, repository);
            }

            if (allowPrerelease.HasValue || implicitAllowPrerelease)
            {
                // If explicit allowPrerelease = false don't use implicit.
                bool allow = allowPrerelease ?? implicitAllowPrerelease;
                parameters.Add(Parameters.AllowPrerelease, allow);
            }

            pwsh.AddCommand(Commands.FindDscResource)
                .AddParameters(parameters);

            // The result is just a PSCustomObject with a type name of Microsoft.PowerShell.Commands.PSGetDscResourceInfo.
            // When no module is passed and a resource is not found, this will return an empty list. If a module
            // is specified and no resource is found then it will fail earlier because of a Write-Error.
            return pwsh.Invoke().FirstOrDefault();
        }

        /// <inheritdoc/>
        public void SaveModule(
            PowerShell pwsh,
            ModuleSpecification moduleSpecification,
            string location)
        {
            var parameters = new Dictionary<string, object>()
            {
                { Parameters.Name, moduleSpecification.Name },
                { Parameters.Path, location },
            };

            if (moduleSpecification.Version is not null)
            {
                parameters.Add(Parameters.MinimumVersion, moduleSpecification.Version);
            }

            if (moduleSpecification.MaximumVersion is not null)
            {
                parameters.Add(Parameters.MaximumVersion, moduleSpecification.MaximumVersion);
            }

            if (moduleSpecification.RequiredVersion is not null)
            {
                parameters.Add(Parameters.RequiredVersion, moduleSpecification.RequiredVersion);
            }

            _ = pwsh.AddCommand(Commands.SaveModule)
                    .AddParameters(parameters)
                    .AddParameter(Parameters.Force)
                    .Invoke();
        }

        /// <inheritdoc/>
        public void SaveModule(
            PowerShell pwsh,
            PSObject inputObject,
            string location)
        {
            _ = pwsh.AddCommand(Commands.SaveModule)
                    .AddParameter(Parameters.Path, location)
                    .AddParameter(Parameters.InputObject, inputObject)
                    .AddParameter(Parameters.Force)
                    .Invoke();
        }

        /// <inheritdoc/>
        public void InstallModule(
            PowerShell pwsh,
            PSObject inputObject,
            bool allUsers)
        {
            var parameters = new Dictionary<string, object>()
            {
                { Parameters.InputObject, inputObject },
            };

            if (allUsers)
            {
                parameters.Add(Parameters.Scope, AllUsers);
            }

            // If the repository is untrusted, it will fail with:
            //   Microsoft.PowerShell.Commands.WriteErrorException : Exception calling "ShouldContinue" with "5"
            //   argument(s): "A command that prompts the user failed because the host program or the command type
            //   does not support user interaction.
            // If its trusted, PowerShellGets adds the Force parameter to the call to PackageManager\Install-Package.
            // TODO: Once we have policies, we should remove Force. For hosted environments and depending
            // on the policy we will trust PSGallery when we create the Runspace or add Force here.
            _ = pwsh.AddCommand(Commands.InstallModule)
                    .AddParameters(parameters)
                    .AddParameter(Parameters.Force)
                    .Invoke();
        }

        /// <inheritdoc/>
        public void InstallModule(
            PowerShell pwsh,
            ModuleSpecification moduleSpecification,
            bool allUsers)
        {
            var parameters = new Dictionary<string, object>()
            {
                { Parameters.Name, moduleSpecification.Name },
            };

            if (moduleSpecification.Version is not null)
            {
                parameters.Add(Parameters.MinimumVersion, moduleSpecification.Version);
            }

            if (moduleSpecification.MaximumVersion is not null)
            {
                parameters.Add(Parameters.MaximumVersion, moduleSpecification.MaximumVersion);
            }

            if (moduleSpecification.RequiredVersion is not null)
            {
                parameters.Add(Parameters.RequiredVersion, moduleSpecification.RequiredVersion);
            }

            if (allUsers)
            {
                parameters.Add(Parameters.Scope, AllUsers);
            }

            _ = pwsh.AddCommand(Commands.InstallModule)
                    .AddParameters(parameters)
                    .AddParameter(Parameters.Force)
                    .Invoke();
        }
    }
}
