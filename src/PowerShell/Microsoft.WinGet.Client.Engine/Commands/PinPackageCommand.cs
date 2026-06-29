// -----------------------------------------------------------------------------
// <copyright file="PinPackageCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Commands
{
    using System.Collections.Generic;
    using System.Management.Automation;
    using System.Threading.Tasks;
    using Microsoft.Management.Deployment;
    using Microsoft.WinGet.Client.Engine.Commands.Common;
    using Microsoft.WinGet.Client.Engine.Exceptions;
    using Microsoft.WinGet.Client.Engine.Helpers;
    using Microsoft.WinGet.Client.Engine.PSObjects;
    using Microsoft.WinGet.Common.Command;

    /// <summary>
    /// Engine command for pin operations (get, add, remove).
    /// </summary>
    public sealed class PinPackageCommand : PackageCommand
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="PinPackageCommand"/> class.
        /// </summary>
        /// <param name="psCmdlet">Caller cmdlet.</param>
        /// <param name="psCatalogPackage">PSCatalogPackage (optional).</param>
        /// <param name="id">Package identifier.</param>
        /// <param name="name">Name of package.</param>
        /// <param name="moniker">Moniker of package.</param>
        /// <param name="source">Source to search. If null, all are searched.</param>
        /// <param name="query">Match against any field of a package.</param>
        public PinPackageCommand(
            PSCmdlet psCmdlet,
            PSCatalogPackage psCatalogPackage,
            string id,
            string name,
            string moniker,
            string source,
            string[] query)
            : base(psCmdlet)
        {
            if (psCatalogPackage != null)
            {
                this.CatalogPackage = psCatalogPackage;
            }

            this.Id = id;
            this.Name = name;
            this.Moniker = moniker;
            this.Source = source;
            this.Query = query;
        }

        /// <summary>
        /// Gets all pins.
        /// </summary>
        public void GetAll()
        {
            IReadOnlyList<PackagePin> pins = this.Execute(
                () => PackageManagerWrapper.Instance.GetAllPins());

            for (int i = 0; i < pins.Count; i++)
            {
                this.Write(StreamType.Object, new PSPackagePin(pins[i]));
            }
        }

        /// <summary>
        /// Gets pins for a specific package.
        /// </summary>
        /// <param name="psPackageFieldMatchOption">PSPackageFieldMatchOption string.</param>
        public void Get(string psPackageFieldMatchOption)
        {
            IReadOnlyList<PackagePin> pins = this.Execute(() =>
            {
                CatalogPackage package = this.FindCatalogPackage(
                    CompositeSearchBehavior.AllCatalogs,
                    PSEnumHelpers.ToPackageFieldMatchOption(psPackageFieldMatchOption));
                return PackageManagerWrapper.Instance.GetPins(package);
            });

            for (int i = 0; i < pins.Count; i++)
            {
                this.Write(StreamType.Object, new PSPackagePin(pins[i]));
            }
        }

        /// <summary>
        /// Adds a pin for a package.
        /// </summary>
        /// <param name="psPackageFieldMatchOption">PSPackageFieldMatchOption string.</param>
        /// <param name="pinType">Pin type string.</param>
        /// <param name="gatedVersion">Gated version range (for Gating pins).</param>
        /// <param name="pinInstalledPackage">Whether to pin the installed package.</param>
        /// <param name="force">Whether to force the pin.</param>
        /// <param name="note">Optional user note.</param>
        public void Add(
            string psPackageFieldMatchOption,
            string pinType,
            string gatedVersion,
            bool pinInstalledPackage,
            bool force,
            string note)
        {
            var result = this.Execute(
                async () => await this.GetPackageAndExecuteAsync(
                    CompositeSearchBehavior.AllCatalogs,
                    PSEnumHelpers.ToPackageFieldMatchOption(psPackageFieldMatchOption),
                    async (package, version) =>
                    {
                        var options = ManagementDeploymentFactory.Instance.CreatePinPackageOptions();
                        options.PinType = PSEnumHelpers.ToPackagePinType(pinType);
                        if (!string.IsNullOrEmpty(gatedVersion))
                        {
                            options.GatedVersion = gatedVersion;
                        }

                        options.PinInstalledPackage = pinInstalledPackage;
                        options.Force = force;
                        if (!string.IsNullOrEmpty(note))
                        {
                            options.Note = note;
                        }

                        return await Task.FromResult(PackageManagerWrapper.Instance.PinPackage(package, options));
                    }));

            if (result != null)
            {
                this.Write(StreamType.Object, new PSPinResult(result.Item1));
            }
        }

        /// <summary>
        /// Removes the pin for a package.
        /// </summary>
        /// <param name="psPackageFieldMatchOption">PSPackageFieldMatchOption string.</param>
        public void Remove(string psPackageFieldMatchOption)
        {
            var result = this.Execute(
                async () => await this.GetPackageAndExecuteAsync(
                    CompositeSearchBehavior.AllCatalogs,
                    PSEnumHelpers.ToPackageFieldMatchOption(psPackageFieldMatchOption),
                    async (package, version) =>
                    {
                        return await Task.FromResult(PackageManagerWrapper.Instance.UnpinPackage(package));
                    }));

            if (result != null)
            {
                this.Write(StreamType.Object, new PSPinResult(result.Item1));
            }
        }

        private CatalogPackage FindCatalogPackage(CompositeSearchBehavior behavior, PackageFieldMatchOption match)
        {
            if (this.CatalogPackage != null)
            {
                return this.CatalogPackage.CatalogPackageCOM;
            }

            IReadOnlyList<MatchResult> results = this.FindPackages(behavior, 0, match);
            if (results.Count == 0)
            {
                throw new NoPackageFoundException();
            }
            else if (results.Count == 1)
            {
                return results[0].CatalogPackage;
            }
            else
            {
                throw new VagueCriteriaException(results);
            }
        }
    }
}
