// -----------------------------------------------------------------------------
// <copyright file="OutOfProcOnlyAttribute.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Helpers
{
    using System;
    using Xunit.Sdk;

    /// <summary>
    /// Trait used to mark a test as only for the out of proc scenario. These tests should not run for the in-proc scenario.
    /// </summary>
    [TraitDiscoverer(OutOfProcDiscoverer.TypeName, Constants.AssemblyNameForTraits)]
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Method, AllowMultiple = false)]
    public class OutOfProcOnlyAttribute : Attribute, ITraitAttribute
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="OutOfProcOnlyAttribute"/> class.
        /// </summary>
        public OutOfProcOnlyAttribute()
        {
        }
    }
}
