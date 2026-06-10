// -----------------------------------------------------------------------------
// <copyright file="InProcAttribute.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Helpers
{
    using System;
    using Xunit.Sdk;

    /// <summary>
    /// Trait used to mark a test as only for the in proc scenario.
    /// </summary>
    [TraitDiscoverer(InProcDiscoverer.TypeName, Constants.AssemblyNameForTraits)]
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Method, AllowMultiple = false)]
    public class InProcAttribute : Attribute, ITraitAttribute
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="InProcAttribute"/> class.
        /// </summary>
        public InProcAttribute()
        {
        }
    }
}
