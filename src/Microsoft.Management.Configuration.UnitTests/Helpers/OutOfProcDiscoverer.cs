// -----------------------------------------------------------------------------
// <copyright file="OutOfProcDiscoverer.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Helpers
{
    using System.Collections.Generic;
    using Xunit.Abstractions;
    using Xunit.Sdk;

    /// <summary>
    /// Enables integration with xUnit trait system.
    /// </summary>
    internal class OutOfProcDiscoverer : ITraitDiscoverer
    {
        /// <summary>
        /// The type name for this discoverer.
        /// </summary>
        internal const string TypeName = Constants.NamespaceNameForTraits + ".OutOfProcDiscoverer";

        /// <summary>
        /// Gets the trait information for the OutOfProcAttribute.
        /// </summary>
        /// <param name="traitAttribute">The trait information.</param>
        /// <returns>Trait name/value pairs.</returns>
        public IEnumerable<KeyValuePair<string, string>> GetTraits(IAttributeInfo traitAttribute)
        {
            yield return new KeyValuePair<string, string>("Category", "OutOfProc");
        }
    }
}
